// BaliseDGAC_GPS_Logger   Balise avec enregistrement de traces
// 20/03/2021 v2
//  Choisir la configuration du logiciel balise dans le fichier fs_options.h
//    (pin utilisées pour le GPS, option GPS, etc ...)

/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*------------------------------------------------------------------------------
  02/2021
  Author: FS
  Platforms: ESP8266
  Language: C++/Arduino
  Basé sur :
  https://github.com/dev-fred/GPS_Tracker_ESP8266/tree/main/GPS_Tracker_ESP8266V1
  https://github.com/khancyr/droneID_FR
  https://github.com/f5soh/balise_esp32/blob/master/droneID_FR.h (version 1 https://discuss.ardupilot.org/t/open-source-french-drone-identification/56904/98 )
  https://github.com/f5soh/balise_esp32

  ------------------------------------------------------------------------------*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <EEPROM.h>
#include <LittleFS.h>
#include <DNSServer.h>
#include "droneID_FR.h"
#include "fsBalise.h"
#include "fs_options.h"
#include "fs_GPS.h"
#include "fs_filtreGpsAscii.h"

extern pref preferences;
extern char statistics[] PROGMEM ;
extern File traceFile;

const char prefixe_ssid[] = "BALISE"; // Enter SSID here
// déclaration de la variable qui va contenir le ssid complet = prefixe + MAC adresse
char ssid[32];

/**
   Numero de serie donnée par l'dresse MAC !
   Changer les XXX par vos references constructeurs, le trigramme est 000 pour les bricoleurs ...
*/

char drone_id[31] = "000XXX000000000000XXXXXXXXXXXX"; // les xxx seront remplacés par l'adresse MAC, sans les ":"
//                   012345678901234567890123456789
//                             11111111112222222222
extern "C" {
#include "user_interface.h"
  int wifi_send_pkt_freedom(uint8 *buf, int len, bool sys_seq);
}

droneIDFR drone_idfr;

// beacon frame definition
static constexpr uint16_t MAX_BEACON_SIZE = 40 + 32 + droneIDFR::FRAME_PAYLOAD_LEN_MAX; // default beaconPacket size + max ssid size + max drone id frame size

// beacon frame definition
uint8_t beaconPacket[MAX_BEACON_SIZE] = {
  /*  0 - 3  */ 0x80, 0x00, 0x00, 0x00, // Type/Subtype: managment beacon frame
  /*  4 - 9  */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination: broadcast
  /* 10 - 15 */ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source
  /* 16 - 21 */ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source

  // Fixed parameters
  /* 22 - 23 */ 0x00, 0x00, // Fragment & sequence number (will be done by the SDK)
  /* 24 - 31 */ 0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, // Timestamp
  /* 32 - 33 */ 0xe8, 0x03, // Interval: 0x64, 0x00 => every 100ms - 0xe8, 0x03 => every 1s
  /* 34 - 35 */ 0x21, 0x04, // capabilities Tnformation

  // Tagged parameters

  // SSID parameters
  /* 36 - 38 */ 0x03, 0x01, 0x06, // DS Parameter set, current channel 6 (= 0x06), // TODO: manually set it
  /* 39 - 40 */ 0x00, 0x20,       // 39-40: SSID parameter set, 0x20:maxlength:content
};


// Ensure the AP SSID is max 31 letters
// 31 lettres maxi selon l'api, 17 caractères de l'adresse mac, reste 15 pour ceux de la chaine du début moins le caractère de fin de chaine ça fait 14, 14+17=31
static_assert((sizeof(prefixe_ssid) / sizeof(*prefixe_ssid)) <= (14 + 1), "Prefix of AP SSID should be less than 14 letters");
// Vérification drone_id max 30
static_assert((sizeof(drone_id) / sizeof(*drone_id)) <= 31, "Drone ID should be less that 30 letters !"); // 30 lettres + null termination

// ========================================================== //

/* Put IP Address details-> smartphone */
const byte DNS_PORT = 53;
DNSServer dnsServer;
ESP8266WebServer server(80);
IPAddress local_ip; // +++++ FS https://github.com/espressif/arduino-esp32/issues/1037
IPAddress gateway;
IPAddress subnet;

boolean traceFileOpened = false;
boolean fileSystemFull = false;
boolean immobile = false;
long T0Immobile;
String messAlarm = "";
unsigned int timeLogError = 0;

unsigned int counter = 0;
unsigned int TRBcounter = 0;
unsigned int nb_sat = 0;
unsigned int SV = 0;
uint64_t gpsSec = 0;
uint64_t beaconSec = 0;
float VMAX = 0.0;
char CLonLat[32];
char CHomeLonLat[32];
char Cmd;
float altitude_ref = 0.0 ;
float Lat = 0.0 ;
float Lon = 0.0 ;
float HLng = 0.0;
float HLat = 0.0;
float latLastLog, lngLastLog ;  // FS ++++++ pour calcul déplacement depuis le dernier log
static uint64_t gpsMap = 0;
String Messages = "init";
unsigned  long _heures = 0;
unsigned  long _minutes = 0;
unsigned  long _secondes = 0;
const unsigned int limite_sat = 5;
const float limite_hdop = 2.0;
float _hdop = 0.0;
unsigned int _sat = 0;

//===========
#ifdef fs_STAT
// pour statistiques
unsigned long T0Beacon, T0Loop, T0SendPkt, T0Server;
int entreBeaconMin, entreBeaconMax, entreBeaconTotal, entreBeaconSousTotal;
int duree, dureeMinLog, dureeMaxLog, dureeTotaleLog, dureeSousTotalLog;
int segment, segmentMin, segmentMax, segmentTotal, segmentSousTotal;
int periodeLoop, perMinLoop, perMaxLoop, perTotaleLoop, perSousTotalLoop;
int dureeMinSendPkt, dureeMaxSendPkt, dureeTotaleSendPkt, dureeSousTotalSendPkt;
int dureeMinServer, dureeMaxServer, dureeTotaleServer, dureeSousTotalServer;
float entreBeaconMoyenneLocale, dureeMoyenneLocaleLog, segmentMoyenneLocale, perMoyenneLocaleLoop;
float dureeMoyenneLocaleSendPkt, dureeMoyenneLocaleServer;
int n0Failed, n0Passed;
unsigned int countLoop, countSendPkt, countServer, nbrLogError = 0;
#endif

// ou debug
unsigned int countLog = 0;
unsigned long T0Log, T1Log = 0;
char cGPS;
char ringBuffer[500];
int indexBuffer;
//===========
//buzz(4, 2500, 1000); // buzz sur pin 4 à 2500Hz
void buzz(int targetPin, long frequency, long length) {
#ifdef ENABLE_BUZZER
  long delayValue = 1000000 / frequency / 2;
  long numCycles = frequency * length / 1000;
  for (long i = 0; i < numCycles; i++)
  {
    digitalWrite(targetPin, HIGH);
    delayMicroseconds(delayValue);
    digitalWrite(targetPin, LOW);
    delayMicroseconds(delayValue);
  }
#endif
}


#define led_pin 2  //internal blue LED
int led;

void flip_Led() {
#ifdef EBABLE_LED
  //Flip internal LED
  if (led == HIGH) {
    digitalWrite(led_pin, led);
    led = LOW;
  } else {
    digitalWrite(led_pin, led);
    led = HIGH;
  }
#endif
}


SoftwareSerial softSerial(GPS_RX_PIN, GPS_TX_PIN);
TinyGPSPlus gps;

#define USE_SERIAL Serial

char buff[14][34];   // contient les valeurs envoyées àla page HTML cockpit
void handleReadValues() {   //    pour traiter la requette de mise à jour de la page HTML cockpit
  String mes = "";
  for (int i = 0; i <= 13; i++) {
    if (i != 0)  mes += ";";
    mes += (String)i + "$" + buff[i] ;
  }
  // ecraser VMAX (case 7) si une alarme.
  if (messAlarm != "") mes += ";7$" + messAlarm;
  server.send(200, "text/plain", mes);
}
#ifdef fs_STAT
void resetStatistics() {
  entreBeaconMin = 99999999; entreBeaconMax = 0; entreBeaconTotal = 0; entreBeaconSousTotal = 0; entreBeaconMoyenneLocale = 0.0;
  dureeMinLog = 99999999; dureeMaxLog = 0; dureeTotaleLog = 0; dureeSousTotalLog = 0; dureeMoyenneLocaleLog = 0.0;
  segmentMin = 99999999; segmentMax = 0; segmentTotal = 0; segmentSousTotal = 0; segmentMoyenneLocale = 0.0;
  perMinLoop = 99999999; perMaxLoop = 0; perTotaleLoop = 0; perSousTotalLoop = 0, perMoyenneLocaleLoop = 0.0;

  dureeMinSendPkt = 99999999; dureeMaxSendPkt = 0; dureeTotaleSendPkt = 0; dureeSousTotalSendPkt = 0; dureeMoyenneLocaleSendPkt = 0.0;
  dureeMinServer = 99999999; dureeMaxServer = 0; dureeTotaleServer = 0; dureeSousTotalServer = 0; dureeMoyenneLocaleServer = 0.0;

  countLoop = 0; countSendPkt = 0; countServer = 0;
  countLog = 0; nbrLogError = 0; TRBcounter = 0; T1Log = 0; T0Beacon = 0;
  n0Passed = gps.passedChecksum(); n0Failed = gps.failedChecksum();
  handleStat();
}
// Calcul pour statistiques min/max etc ...
// Tout est passer par référence !!  sauf T0
void calculerStat (boolean calculerTemps, int T0, int &minimum, int &maximum, float &moyenneLocaleEchantillons, int &totalEchantillons,
                   int &sousTotalEchantillons, unsigned int &count) {

  // calculerTemps  true:  T0 représente le temps début de la periode de mesure
  //       (utile pour stat de timming)
  // calculerTemps  false:  T0 represente la mesure elle même, dont on va calculer min/max etc ..
  //       (utile pour stat sur longeur de segment)
  long T1;
  int echantillon;
  if (T0 == 0 && calculerTemps) return;  // ignorer le premier car T0 est parfois mal initialisé
  T1 = millis();
  if (calculerTemps) echantillon = T1 - T0;
  else echantillon = T0;
  totalEchantillons += echantillon; minimum = min(minimum, echantillon); maximum = max(maximum, echantillon);
  sousTotalEchantillons += echantillon;
  count++;
  if (count % 20 == 0) {
    moyenneLocaleEchantillons = float(sousTotalEchantillons) / 20.0;
    sousTotalEchantillons = 0;
  }
}
// Generation reponse pour la page statistique
void handleReadStatistics() {
  char buf[350]; // 260 car env utlisés
  int deb = 0;
  deb += sprintf(&buf[deb], "0$%s Err GPS:%u<br>passedCheck:%u failedCheck:%u (%.0f%%);", messAlarm.c_str(), nbrLogError,
                 gps.passedChecksum() - n0Passed, gps.failedChecksum() - n0Failed ,
                 100 * float(gps.failedChecksum() - n0Failed) / float(gps.passedChecksum() - n0Passed)); //
  deb += sprintf(&buf[deb], "1$Nbr entrées trace: %u  ,Nbr Beacon: %u;", countLog, TRBcounter );
  deb += sprintf(&buf[deb], "2$%u;", dureeMinLog);
  deb += sprintf(&buf[deb], "3$%u;", dureeMaxLog);
  deb += sprintf(&buf[deb], "4$%.1f;", float(dureeTotaleLog) / countLog);
  deb += sprintf(&buf[deb], "5$%.1f;", dureeMoyenneLocaleLog);
  deb += sprintf(&buf[deb], "6$%u;", segmentMin);
  deb += sprintf(&buf[deb], "7$%u;", segmentMax);
  deb += sprintf(&buf[deb], "8$%.1f;", float(segmentTotal) / countLog);
  deb += sprintf(&buf[deb], "9$%.1f;", segmentMoyenneLocale);
  deb += sprintf(&buf[deb], "10$%u;", entreBeaconMin);
  deb += sprintf(&buf[deb], "11$%u;", entreBeaconMax);
  deb += sprintf(&buf[deb], "12$%.1f;", float(entreBeaconTotal) / TRBcounter);
  deb += sprintf(&buf[deb], "13$%.1f;", entreBeaconMoyenneLocale);
  deb += sprintf(&buf[deb], "14$%u;", perMinLoop);
  deb += sprintf(&buf[deb], "15$%u;", perMaxLoop);
  deb += sprintf(&buf[deb], "16$%.1f;", float(perTotaleLoop) / countLoop);
  deb += sprintf(&buf[deb], "17$%.1f;", perMoyenneLocaleLoop);
  deb += sprintf(&buf[deb], "18$%u;", dureeMinServer);
  deb += sprintf(&buf[deb], "19$%u;", dureeMaxServer);
  deb += sprintf(&buf[deb], "20$%.1f;", float(dureeTotaleServer) / countServer);
  deb += sprintf(&buf[deb], "21$%.1f;", dureeMoyenneLocaleServer);
  deb += sprintf(&buf[deb], "22$%u;", dureeMinSendPkt);
  deb += sprintf(&buf[deb], "23$%u;", dureeMaxSendPkt);
  deb += sprintf(&buf[deb], "24$%.1f;", float(dureeTotaleSendPkt) / countSendPkt);
  deb += sprintf(&buf[deb], "25$%.1f", dureeMoyenneLocaleSendPkt);  // pas de ; pour la dernière valeur
  //Serial.println(deb);
  // Serial.print("\t"); Serial.println(buf);
  server.send(200, "text/plain", buf);
}
void handleStat() {
  sendChunkDebut ( true );  // debut HTML style, avec topMenu
  server.sendContent_P(statistics);
  server.chunkedResponseFinalize();
}
#endif    // partie spécifique pour statistiques

void beginServer()
{
  server.begin();

  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  // dnsServer.setTTL(300);   // va
  Serial.print(F("Starting dnsServer for captive portal ... "));
  Serial.println(dnsServer.start(DNS_PORT, "*", local_ip) ? F("Ready") : F("Failed!"));
  Serial.print(F("Opening filesystem littleFS ... "));
  Serial.println(LittleFS.begin() ? F("Ready") : F("Failed!"));
  fs_initServerOn( );  // server.on(xxx   Spécifiques au log, gestion fichier,OTA,option
#ifdef fs_STAT
  server.on("/stat", handleStat);
  server.on("/readStatistics", handleReadStatistics);
  server.on("/statReset", resetStatistics);
#endif
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++
  server.on("/readValues", handleReadValues);
  Serial.println (F("HTTP server started"));
}

#ifndef  GPS_STANDARD
void SelectChannels()
{
  // CFG-GNSS packet GPS + Galileo + Glonas
  byte packet[] = {0xB5, 0x62, 0x06, 0x3E, 0x3C, 0x00, 0x00, 0x00, 0x20, 0x07, 0x00, 0x08, 0x10, 0x00,
                   0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x04,
                   0x08, 0x00, 0x01, 0x00, 0x01, 0x01, 0x03, 0x08, 0x10, 0x00, 0x00, 0x00, 0x01, 0x01,
                   0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x01, 0x05, 0x00, 0x03, 0x00, 0x00, 0x00,
                   0x01, 0x01, 0x06, 0x08, 0x0E, 0x00, 0x01, 0x00, 0x01, 0x01, 0x2E, 0x75
                  };
  sendPacket(packet, sizeof(packet));
}

void BaudRate9600()
{
  byte packet[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA2, 0xB5};
  sendPacket(packet, sizeof(packet));
}

void Rate500()
{
  byte packet[] = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xF4, 0x01, 0x01, 0x00, 0x01, 0x00, 0x0B, 0x77};
  sendPacket(packet, sizeof(packet));
}

// Send the packet specified to the receiver.
void sendPacket(byte * packet, byte len)
{
  for (byte i = 0; i < len; i++)
  {
    softSerial.write(packet[i]);
  }
}
#endif

void setup()
{

  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println();

  EEPROM.begin(512);
  checkFactoryReset();
  readPreferences();
  Serial.println(F("Prefs lues dans EEPROM:"));
  listPreferences();
  local_ip.fromString(preferences.local_ip);
  gateway.fromString(preferences.gateway);
  subnet.fromString(preferences.subnet);


#ifdef ENABLE_BUZZER
  pinMode(BUZZER_PIN_G, OUTPUT); // set a pin for buzzer output
  digitalWrite(BUZZER_PIN_G, LOW);
  pinMode(BUZZER_PIN_P, OUTPUT); // set a pin for buzzer output
  buzz(BUZZER_PIN_P, 2500, 100);
#endif

#ifdef ENABLE_LED
  //built in blue LED -> change d'état à chaque envoi de trame
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);
#endif
  //  for (uint8_t t = 4; t > 0; t--) {
  //    Serial.printf("[SETUP] BOOT WAIT %d...\r\n", t);
  //    Serial.flush();
  //    delay(1000);
  //  }

  //connection sur le terrain à un smartphone
  // start WiFi
  WiFi.mode(WIFI_AP);   // +++++++++++++++++++++++++++  FS
  //conversion de l'adresse mac:
  String temp = WiFi.macAddress();
  temp.replace(":", ""); //on récupère les12 caractères
  strcpy(&drone_id[18], temp.c_str()); // quel'onmet a la fin du drone_id
  Serial.print(F("Drone_id:")), Serial.println(drone_id);
  temp = WiFi.macAddress();
  //concat du prefixe et de l'adresse mac
  temp = String(prefixe_ssid) + "_" + temp;
  //transfert dans la variable globale ssid
  temp.toCharArray(ssid, 32);

  Serial.print(F("Setting soft-AP configuration ... "));
  Serial.println(WiFi.softAPConfig(local_ip, gateway, subnet) ? F("Ready") : F("Failed!"));
  Serial.print(F("Setting soft-AP ... "));
  // ssid, pwd, channel, hidden, max_cnx
  Serial.println(WiFi.softAP(ssid, preferences.password, 6, false, 4) ? F("Ready") : F("Failed!"));
  Serial.print(F("IP address for AP network "));
  Serial.print(ssid);
  Serial.print(F(" : "));
  Serial.println(WiFi.softAPIP());
  WiFi.setOutputPower(20.5); // max 20.5dBm

  softap_config current_config;
  wifi_softap_get_config(&current_config);

  current_config.beacon_interval = 1000;
  wifi_softap_set_config(&current_config);

  beginServer(); //lancement du server WEB

#ifdef GPS_STANDARD
  fs_initGPS();
  // softSerial.begin(9600);   // ++++++++++  FS : communication à 9600.
#else
  //--------------------------------------------- 57600 ->BAUDRATE 9600
  softSerial.begin(GPS_57600);
  delay(100); // Little delay before flushing.
  softSerial.flush();
  Serial.println("GPS BAUDRATE 9600");
  BaudRate9600();
  delay(100); // Little delay before flushing.
  softSerial.flush();
  softSerial.begin(GPS_9600);
  delay(100); // Little delay before flushing.
  softSerial.flush();
  //-------Config RATE = 500 ms
  Serial.println("Configure GPS RATE = 500 ms");
  Rate500();
  delay(100); // Little delay before flushing.
  softSerial.flush();
  //--------Config CHANNELS
  Serial.println("Configure GPS CHANNELS = GPS + Galileo + Glonas");
  SelectChannels();
  delay(100); // Little delay before flushing.
  softSerial.flush();
#endif
  drone_idfr.set_drone_id(drone_id);
  snprintf(buff[0], sizeof(buff[0]), "ID:%s", drone_id); // on aura tout de suite l'info
#ifdef fs_STAT
  resetStatistics();
#endif
  // pour debug points aberrants
  for (int i = 0; i < sizeof(ringBuffer); i++ ) ringBuffer[i] = ' ';
  indexBuffer = 0;
  Serial.println(F("Attente du fix & Co"));
  delay(1000);
}



void loop()
{
  delay(1);
#ifdef fs_STAT
  T0Server = millis();
#endif
  server.handleClient();
#ifdef fs_STAT
  calculerStat (true, T0Server, dureeMinServer, dureeMaxServer, dureeMoyenneLocaleServer, dureeTotaleServer,
                dureeSousTotalServer, countServer);
#endif
  dnsServer.processNextRequest();


  // Ici on lit les données qui arrivent du GPS et on les passe à la librairie TinyGPS++ pour les traiter
  while (softSerial.available()) {
    cGPS = softSerial.read();
    ringBuffer[indexBuffer++] = cGPS;
    indexBuffer %= sizeof(ringBuffer);
    // pour filtrer quelques erreurs et avoir une chance de faire un checksum error ..... Q&D !!
    if ( cGPS > 0x57 || !asciiFilter[cGPS] ) {
      //  Serial.print(char(cGPS));
      continue; // ignorer à partir des minuscules  a    etc ...
    }
    gps.encode(cGPS);
    // gps.encode(softSerial.read());
  }
  yield();
  // On traite le cas où le GPS a un problème
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("NO GPS"));
    strncpy(buff[8], "NO GPS", sizeof(buff[8]));
    delay(2000);  //pour ne pas encombrer Serial Monitor ...
    return;
  }
  boolean saveLocationIsUpdated = gps.location.isUpdated();
  // On traite le cas si la position GPS n'est pas valide
  if (!gps.location.isValid()) {
    if (millis() - gpsMap > 1000) {
      SV = gps.satellites.value();
      Serial.print("Waiting... SAT=");  Serial.println(SV);
      snprintf(buff[8], sizeof(buff[8]), "ATT SAT %u", SV);
      strncpy(buff[10], "ATTENTE", sizeof(buff[10]));
      buzz(BUZZER_PIN_P, 2500, 10);//tick
      //Flip internal LED
      flip_Led();
      gpsMap = millis();

    }
    return;
    // } else if (gps.location.age() > 3000) {
  } else if (gps.location.age() > 5000) {
    // Serial.println("NO SAT");
    strncpy(buff[8], "NO SAT", sizeof(buff[8]));
    return;
    // } else if (gps.location.isUpdated() && gps.altitude.isUpdated() && gps.course.isUpdated() && gps.speed.isUpdated()) {
    // FS FS ++++++++++++++++++++++++++++++++  OU à la place de ET
  } else if (gps.location.isUpdated() || gps.altitude.isUpdated() || gps.course.isUpdated() || gps.speed.isUpdated()) {   // FS +++++++++++
    saveLocationIsUpdated = gps.location.isUpdated();
    // On traite le cas où la position GPS est valide.
    // On renseigne le point de démarrage quand la précision est satisfaisante
    if ( gps.satellites.value() > nb_sat ) {//+ de satellites
      nb_sat = gps.satellites.value();
      //     Serial.print("NEW SAT=");  Serial.println(nb_sat);
      strncpy(buff[8], "+SAT", sizeof(buff[8]));
    }
    if ( gps.satellites.value() < nb_sat ) {//- de satellites
      nb_sat = gps.satellites.value();
      //      Serial.print("LOST SAT=");  Serial.println(nb_sat);
      strncpy(buff[8], "-SAT", sizeof(buff[8]));
    }

    if (!drone_idfr.has_home_set() && gps.satellites.value() > limite_sat && gps.hdop.hdop() < limite_hdop) {

      Serial.println(F("Setting Home Position"));
      HLat = gps.location.lat(); HLng = gps.location.lng();
      latLastLog = HLat; lngLastLog = HLng;  //   FS +++++++++++++++++++++++++++++++++++
      drone_idfr.set_home_position(HLat, HLng, gps.altitude.meters());
      altitude_ref = gps.altitude.meters();
      snprintf(buff[11], sizeof(buff[11]), "DLNG:%.4f", HLng);
      snprintf(buff[12], sizeof(buff[12]), "DLAT:%.4f", HLat);
      strncpy(buff[10], "DEPART", sizeof(buff[10]));
      VMAX = 0;
      buzz(BUZZER_PIN_P, 7000, 200);
      delay (50);
      buzz(BUZZER_PIN_P, 7000, 200);
      delay (50);
      buzz(BUZZER_PIN_P, 7000, 200);
    }

    // On actualise les données GPS de la librairie d'identification drone.
    drone_idfr.set_current_position(gps.location.lat(), gps.location.lng(), gps.altitude.meters());
    drone_idfr.set_heading(gps.course.deg());
    drone_idfr.set_ground_speed(gps.speed.mps());

    //**************************************************************************
    //Calcul VMAX et renseigne les datas de la page WEB
    if (VMAX < gps.speed.mps()) {
      VMAX = gps.speed.mps();
    }
    snprintf(buff[1], sizeof(buff[1]), "UTC:%d:%d:%d", gps.time.hour(), gps.time.minute(), gps.time.second());
    _sat = gps.satellites.value(); if (_sat < limite_sat) {
      snprintf(buff[2], sizeof(buff[2]), "--SAT:%u", _sat);
    } else {
      snprintf(buff[2], sizeof(buff[2]), "SAT:%u", _sat);
    }
    _hdop = gps.hdop.hdop(); if (_hdop > limite_hdop) {
      snprintf(buff[3], sizeof(buff[3]), "++HDOP:%.2f", _hdop);
    } else {
      snprintf(buff[3], sizeof(buff[3]), "HDOP:%.2f", _hdop);
    }
    snprintf(buff[4], sizeof(buff[4]), "LNG:%.4f", gps.location.lng());
    snprintf(buff[5], sizeof(buff[5]), "LAT:%.4f", gps.location.lat());
    snprintf(buff[6], sizeof(buff[6]), "ALT:%.2f", gps.altitude.meters() - altitude_ref);
    snprintf(buff[7], sizeof(buff[7]), "%s VMAX(km/h):%.2f", messAlarm.c_str(), float (VMAX * 3.6));

    //*************************************************************

  }


  /**
    On regarde s'il est temps d'envoyer la trame d'identification drone :
     - soit toutes les 3s,
     - soit si le drone s'est déplacé de 30m,
     - uniquement si la position Home est déjà définie,
     - et dans le cas où les données GPS sont nouvelles.
  */

  // ATTENTION: ne pas appeler 2 fois drone_idfr.time_to_send !!
  if (drone_idfr.has_home_set() && drone_idfr.time_to_send()) {
    float time_elapsed = (float(millis() - beaconSec) / 1000);
    beaconSec = millis();
    /*
      Serial.print(time_elapsed,1); Serial.print("s Send beacon: "); Serial.print(drone_idfr.has_pass_distance() ? "Distance" : "Time");
      Serial.print(" with ");  Serial.print(drone_idfr.get_distance_from_last_position_sent()); Serial.print("m Speed="); Serial.println(drone_idfr.get_ground_speed_kmh());
    */
    /**
        On commence par renseigner le ssid du wifi dans la trame
    */
    // write new SSID into beacon frame
    const size_t ssid_size = (sizeof(ssid) / sizeof(*ssid)) - 1; // remove trailling null termination
    beaconPacket[40] = ssid_size;  // set size
    memcpy(&beaconPacket[41], ssid, ssid_size); // set ssid
    const uint8_t header_size = 41 + ssid_size;  //TODO: remove 41 for a marker
    /**
        On génère la trame wifi avec l'identification
    */
    const uint8_t to_send = drone_idfr.generate_beacon_frame(beaconPacket, header_size);  // override the null termination

    /**
        On envoie la trame
    */
#ifdef fs_STAT
    T0SendPkt = millis();
#endif
    if (wifi_send_pkt_freedom(beaconPacket, to_send, 0) != 0)  //  0 sucess, -1 erreur
      Serial.println(F("--Err emission trame beacon"));

#ifdef fs_STAT
    calculerStat (true, T0SendPkt, dureeMinSendPkt, dureeMaxSendPkt, dureeMoyenneLocaleSendPkt, dureeTotaleSendPkt,
                  dureeSousTotalSendPkt, countSendPkt);
    //calcul de stat. Attention la fonction incrmente déjà le compteur
    calculerStat (true, T0Beacon, entreBeaconMin, entreBeaconMax, entreBeaconMoyenneLocale, entreBeaconTotal, entreBeaconSousTotal, TRBcounter);
    TRBcounter--;
    T0Beacon = millis();
#endif
    //Flip internal LED
    flip_Led();
    //incrementation compteur de trame de balise envoyé
    TRBcounter++;
    snprintf(buff[9], sizeof(buff[9]), "TRAME:%u", TRBcounter);

    _secondes = millis() / 1000; //convect millisecondes en secondes
    _minutes = _secondes / 60; //convertir secondes en minutes
    _heures = _minutes / 60; //convertir minutes en heures
    _secondes = _secondes - (_minutes * 60); // soustraire les secondes converties afin d'afficher 59 secondes max
    _minutes = _minutes - (_heures * 60); //soustraire les minutes converties afin d'afficher 59 minutes max

    snprintf(buff[13], sizeof(buff[13]), " %dmn:%ds", _minutes, _secondes );

    /**
        On reset la condition d'envoi
    */
    drone_idfr.set_last_send();
  }

  // on regarde si on doit enregistrer un point dans la trace
  if (drone_idfr.has_home_set() &&  !preferences.logNo && (saveLocationIsUpdated || preferences.logAfter < 0) && !fileSystemFull  ) {

    float segmentf = distanceSimple(gps.location.lat(), gps.location.lng(), latLastLog, lngLastLog);
    // rejet de points abberrants (problème de communication avec le GPS ...)
    if ( gps.location.lat() == 0 || gps.location.lng() == 0 || (preferences.logAfter > 0) && segmentf > 30 * preferences.logAfter) {
      // point GPS abberrant: enregistrer l'erreur et l'ignorer ...
      // N'enregistrer que une fois le message, en début de rafale .
      // if ( gps.time.value() != timeLogError) {  // on suppose que gps.time.value() n'est pas aussi faux que gps.location()...
      //   logRingBuffer( ringBuffer, sizeof(ringBuffer), indexBuffer);
      //   logError(gps, latLastLog, lngLastLog, segmentf);
      //  timeLogError = gps.time.value();
      // }
#ifdef fs_STAT
      nbrLogError++;
#endif
    } else {  // point GPS OK
      // fermer le fichier trace si on ne bouge plus: avion posé ?, on risque la coupure de courant
      if (gps.speed.kmph() <= 0.01) { // baddddddddddddddddddddddddd  ??????????????????    +++++++++++++++++
        if ( immobile && millis() - T0Immobile > 2000 && preferences.logAfter > 0 && traceFileOpened ) {
          //a l'arret depuis plus de 2000s.: fermer le fichier trace. Sauf si en mode test avec preferences.logAfter <= 0)
          traceFile.close();
          traceFileOpened = false;
          //  Serial.println(F("-----------close main"));
        } else if (!immobile) {
          immobile = true;
          T0Immobile = millis();
        }
      } else {
        immobile = false;
      }
      // Si preferences.logAfter > 0 :On stocke un point de trace si le nouveau segment et plus long que preferences.logAfter
      // Si preferences.logAfter < 0 : pour des tests. On ne tient plus compte de la longeur du segment, mais on stocke
      //   un point de trace toutes les abs(preferences.logAfter)  ms ( il faut au moins 70ms ??)
      if (((preferences.logAfter > 0) && (segmentf > preferences.logAfter)) || ((preferences.logAfter < 0) && (millis() - T1Log > abs (preferences.logAfter)))) {
        T0Log = millis();
        if ( !fs_ecrireTrace (gps)) {
          Serial.println(F("File system full ? "));
          fileSystemFull = true;
          messAlarm = "<span class='b2'>Mémoire pleine ?</span>";
        } else {
          // ne mettre a jour les stats que si ecriture est faite. (sinon le disque est plein ..)
          fileSystemFull = false;
          messAlarm = "";
#ifdef fs_STAT
          calculerStat (true, T0Log, dureeMinLog, dureeMaxLog, dureeMoyenneLocaleLog, dureeTotaleLog,
                        dureeSousTotalLog, countLog);
          countLog--;  // même compteur pour durée d'un log et nbr de segment
          calculerStat (false, segment, segmentMin, segmentMax, segmentMoyenneLocale, segmentTotal,
                        segmentSousTotal, countLog);
          countLog--;
#endif
          countLog++;
          if (countLog % 300 == 0) {
            //  Serial.println(F("-- Flush"));
            traceFile.flush(); // forcer la mise a jour tous les 100 points
          }
          latLastLog = gps.location.lat();
          lngLastLog = gps.location.lng();
          // stats sur une tour complet de la boucle
#ifdef fs_STAT
          calculerStat (true, T0Loop, perMinLoop, perMaxLoop, perMoyenneLocaleLoop, perTotaleLoop,
                        perSousTotalLoop, countLoop);
#endif
        }
        T1Log = millis();
      }
    }
  }  // test si il faut ecrire un point
#ifdef fs_STAT
  T0Loop = millis();
#endif
}
