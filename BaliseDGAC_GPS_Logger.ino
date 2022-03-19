
// BaliseDGAC_GPS_Logger   Balise avec enregistrement de traces et récepteur balise
// 03/2022 v3 b1
//  Choisir la configuration du logiciel balise dans le fichier fs_options.h
//    (pins utilisées pour le GPS, option GPS, etc ...)

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
  03/2022   02/2021
  Author: FS
  Platforms: ESP8266 / ESP32 / ESP32-C3
  Language: C++/Arduino
  Basé sur :
  https://github.com/dev-fred/GPS_Tracker_ESP8266
  https://github.com/khancyr/droneID_FR
  https://github.com/f5soh/balise_esp32/blob/master/droneID_FR.h (version 1 https://discuss.ardupilot.org/t/open-source-french-drone-identification/56904/98 )
  https://github.com/f5soh/balise_esp32
  https://www.tranquille-informatique.fr/modelisme/divers/balise-dgac-signalement-electronique-a-distance-drone-aeromodelisme.html

  ------------------------------------------------------------------------------*/

#ifdef ESP32
#pragma message "Compilation pour ESP32 !"
#include <WiFi.h>
#include "fs_WebServer.h"
#elif defined(ESP8266)
#pragma message "Compilation pour ESP8266 !"
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#else
#error "Il faut utiliser une carte ESP32 ou ESP8266"
#endif

#include <LittleFS.h>
#include <TinyGPS++.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include "droneID_FR.h"
#include "fsBalise.h"
#include "fs_options.h"
#include "fs_GPS.h"
#include "fs_main.h"
//#ifdef fs_RECEPTEUR
#include "fs_recepteur.h"
//#endif
#include "fs_pagePROGMEM.h"

#if defined(ESP32)
#include <HardwareSerial.h>
#pragma message "Utilisation de HardwareSerial !"
#else
#include <SoftwareSerial.h>
#pragma message "Utilisation de SotfwareSerial !"
#endif

extern pref preferences;
//extern const char statistics[] PROGMEM ;
extern File traceFile;

const char prefixe_ssid[] = "BALISE"; // Enter SSID here
// déclaration de la variable qui va contenir le ssid complet = prefixe + MAC adresse
char ssid[33];

//double codeinfo = 0;  // FS :   pourquoi double ????????     ++++++++++++++++++++++++++++++
double codeinfo = 1;  // FS :   pourquoi double ????????     ++++++++++++++++++++++++++++++

/*
  codeinfo
  1   GPS non détecté, aucune donnée GPS reçue

  2   Positionnement du GPS non valide (fix en cours?)
  3   Fix du GPS perdu...
  4   La précision du GPS n'est pas bonne
    Nb de satellites (doit être supérieur à 3): " + String(gps.satellites.value()));
  5   La précision du GPS n'est pas bonne
    Précision 2D (doit être inférieure à 5.0): " + String(gps.hdop.hdop()));
  6   La précision du GPS n'est pas bonne
    Altitude non récupérée: " + String(gps.altitude.meters()));
  7   stockage positionde départ
  9   ça roule. Pas d'erreur
*/
// dès que la position home a été mise, on passe à true et on y reste
bool has_set_home = false;
double home_alt = 0;
//les positions précédentes:
double lat_prev = 0;
double lon_prev = 0;
int16_t alt_prev = 0;
int16_t dir_prev = 0;
double vit_prev = 0; // en m/s

boolean saveLocationIsUpdated;
/**
   Numero de serie donnée par l'dresse MAC !
   Changer les XXX par vos references constructeurs, le trigramme est 000 pour les bricoleurs ...
*/
char drone_id[31] = "000FSB000000000000YYYYYYYYYYYY"; // les YY seront remplacés par l'adresse MAC, sans les ":"
//                   012345678901234567890123456789
//                             11111111112222222222
#ifdef ESP32
extern "C"
{
#include "esp_wifi.h"
  esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);
  esp_err_t esp_wifi_internal_set_fix_rate(wifi_interface_t ifx, bool en, wifi_phy_rate_t rate);
}
#else
extern "C" {
#include "user_interface.h"
  int wifi_send_pkt_freedom(uint8 *buf, int len, bool sys_seq);
}
#endif

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
#ifdef ESP32
//WebServer server(80);
fs_WebServer server(80);
#else
ESP8266WebServer server(80);
#endif
IPAddress local_ip; // +++++ FS https://github.com/espressif/arduino-esp32/issues/1037
IPAddress gateway;
IPAddress subnet;

boolean modeRecepteur = false;  // false: mode balise avec emmision de trames; true: mode reception de trames beacon
boolean traceFileOpened = false;
boolean fileSystemFull = false;
boolean immobile = false;
long T0Immobile;
String messAlarm = "";
unsigned int timeLogError = 0;

unsigned long TimeShutdown;
boolean countdownRunning = true;
boolean shutdown = false;


unsigned int TRBcounter = 0;  // nbr de trames nvoyées

float vitesse = 0.0;  // vitesse en m/s, qui sera lissée, pour décision fermer fichier et arret wifi
float VmaxCockpit = 0.0, AltmaxCockpit = 0.0;
float VmaxSegment = 0.0, AltMaxSegment = 0.0 ;// vitesse (kmh)/alt max vue entre 2 points de trace enregistré

float latLastLog, lngLastLog ;  // FS ++++++ pour calcul déplacement depuis le dernier log
//===========
#ifdef fs_STAT
// pour statistiques
unsigned long T0Beacon;
statBloc_t statBeacon = {"Période Beacon"}, statSendPkt = {"Durée Sendpkt"}, statServer = {"Durée Server handle"},
           statLog = {"Durée ecr trace"}, statSegment = {"Long segment"}, statLoop = {"Période loop"} ;
statBloc_t statReveiller = {"Durée reveiller"}, statEndormir = {"Durée endormir"};
statBloc_t  statNotFound = {" notFound" }, statCockpit = {"cockpit"};

int n0Failed, n0Passed;
unsigned int nbrLogError = 0;
#endif

// ou debug
unsigned int countLog = 0;
unsigned long T1Log = 0;
char cGPS;
//===========


#if defined(ESP32)
HardwareSerial serialGPS(1);
#else
SoftwareSerial serialGPS(GPS_RX_PIN, GPS_TX_PIN);
#endif
TinyGPSPlus gps;

#ifdef fs_STAT
void razStatistics() {
  razStatBloc(&statBeacon); razStatBloc(&statSendPkt); razStatBloc(&statServer);
  razStatBloc(&statLog); razStatBloc(&statSegment); razStatBloc(&statLoop);
  razStatBloc(&statReveiller); razStatBloc(&statEndormir);
  razStatBloc(&statNotFound); razStatBloc(&statCockpit);
  nbrLogError = 0;  T1Log = 0;
  n0Passed = gps.passedChecksum(); n0Failed = gps.failedChecksum();
}
void handleResetStatistics() {
  razStatistics();  // raz des compteurs & Co
  handleStat(); // renvoyer le html
}

void handleReadStatistics() {
  dbgHeap("deb");
  char buf[350]; // 290 car env utlisés
  int deb = 0;
  // Pour remplir la page HTML de stats. 2eme champs (délimité par les ;) vont dans les elements Value1 et Value2
  //  Les autres remplissent la table construite dynamiquement
  // libellé1$min$max$moyenne1$moyenne2;libellé2$min$max$moyenne1$moyenne2
  int gpsFailed = gps.failedChecksum();
  int gpsPass =  gps.passedChecksum();
  deb += snprintf_P(&buf[deb], sizeof(buf) - deb, PSTR("%s Err GPS:%u<br>passedCheck:%u failedCheck:%u (%.1f%%);"), messAlarm.c_str(), nbrLogError,
                    gpsPass - n0Passed, gpsFailed - n0Failed ,
                    100 * float(gpsFailed - n0Failed) / float(gpsFailed - n0Failed + gpsPass - n0Passed));
  deb += snprintf_P(&buf[deb], sizeof(buf) - deb, PSTR("Nbr entrées trace: %u  ,Nbr Beacon: %u;"),
                    countLog, TRBcounter);
  deb += writeStatBloc(&buf[deb], sizeof(buf) - deb, &statLog);
  deb += writeStatBloc(&buf[deb], sizeof(buf) - deb, &statSegment);
  deb += writeStatBloc(&buf[deb], sizeof(buf) - deb, &statBeacon);
  deb += writeStatBloc(&buf[deb], sizeof(buf) - deb, &statLoop);
  deb += writeStatBloc(&buf[deb], sizeof(buf) - deb, &statServer);
  // deb += writeStatBloc(&buf[deb], sizeof(buf) - deb, &statSendPkt);
  deb += writeStatBloc(&buf[deb], sizeof(buf) - deb, &statNotFound);
  //  deb += writeStatBloc(&buf[deb], sizeof(buf) - deb, &statCockpit);

  // effacer le dernier ";".
  deb--;
  buf[deb] = 0;
  // Serial.printf_P(PSTR("-----handleReadStatistics: %u  %i/%i  %s\n "), millis(), deb, sizeof(buf), buf);
  server.send(200, "text/plain", buf);
  dbgHeap("fin");
}
void handleStat() {
  countdownRunning = false;
  sendChunkDebut ( true );  // debut HTML style, avec topMenu
  server.sendContent_P(statistics);
  server.chunkedResponseFinalize();
}
#endif    // partie spécifique pour statistiques

// Generation reponse requette pour mise à jour de la page HTML cockpit
// Format: suite de                N°_de_champ$valeur;
void handleReadValues() {
  const char *messageCodeinfo[] = {"GPS", "pos/date", "perdu", "sat", "hdop", "alt", "", "", ""};
  char buf[400]; // 350 car env utlisés
  int deb = 0;
  countdownRunning = true; //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  deb += sprintf_P(&buf[deb], PSTR("1$Balise v%s;"), versionSoft);
  deb += sprintf_P(&buf[deb], PSTR("2$ID: %s;"), drone_id);
  if (messAlarm != "")  deb += sprintf_P(&buf[deb], PSTR("3$%s;"), messAlarm.c_str());
  else {
    deb += sprintf_P(&buf[deb], PSTR("3$Mode basse consommation "));
    if (!preferences.arretWifi)
      deb += sprintf_P(&buf[deb], PSTR("impossible.;"));
    else
      deb += sprintf_P(&buf[deb], PSTR("%s programmé.;"), preferences.basseConso  ? "" : " non");
  }
  if (!preferences.arretWifi)
    deb += sprintf_P(&buf[deb], PSTR("5$<span class='b2'>Arrêt du Wifi non programmé.</span>;")); // pas de ; pour la dernière valeur
  else
    deb += sprintf_P(&buf[deb], PSTR("5$<span class=%s du Wifi dans %is</span>;"), has_set_home ? "'b1'> Arrêt" : "'b3'> Après le fix GPS, arrêt" , int((int(TimeShutdown) - int(millis())) / 1000)); // pas de ; pour la dernière valeur
  deb += sprintf_P(&buf[deb], PSTR("7$Lng:%.6f;"), lon_prev);
  deb += sprintf_P(&buf[deb], PSTR("8$Lat:%.6f;"), lat_prev);
  deb += sprintf_P(&buf[deb], PSTR("9$%02d:%02d:%02d.%02d;"), gps.time.hour(), gps.time.minute(), gps.time.second(),
                   gps.time.centisecond());
  deb += sprintf_P(&buf[deb], PSTR("10$H(Max):%i(%i);"), int(alt_prev - home_alt), int(AltmaxCockpit - home_alt));  // hauteur par rapport au sol
  deb += sprintf_P(&buf[deb], PSTR("11$V(Max):%.2f(%.2f)<br>km/h;"), vit_prev * 3.6, VmaxCockpit); // vitesse en km/h
  deb += sprintf_P(&buf[deb], PSTR("12$Cap:%i;"), dir_prev);
  deb += sprintf_P(&buf[deb], PSTR("13$Trames:%u;"), TRBcounter);
  deb += sprintf_P(&buf[deb], PSTR("14$Pts de trace:%u;"), countLog);
  deb += sprintf_P(&buf[deb], PSTR("15$Code:%u;"), int(codeinfo));

  deb += sprintf_P(&buf[deb], PSTR("16$<span class=%s(%s)</span>;"), codeinfo <= 6 ? "'b2'>Att. Fix" : "'b1'>Fix OK" ,
                   messageCodeinfo[int(codeinfo) - 1]);
  deb += sprintf_P(&buf[deb], PSTR("17$Sat:%u;"), gps.satellites.value());
  deb += sprintf_P(&buf[deb], PSTR("18$Hdop:%.2f"), gps.hdop.hdop());

  // Serial.printf_P(PSTR("-----handleReadValues. millis=%u   TimeShutdown=%u    deb=%i/%i\n"), millis(), TimeShutdown, deb, sizeof(buf));
  server.send(200, "text/plain", buf);
}
void handleRazVMaxHMAx()
{
  Serial.println(F("handleRazVMaxHMAx"));
  VmaxCockpit = 0.0;
  AltmaxCockpit = 0.0;
  server.send(204); // OK, no content
}
void beginServer()
{
  server.begin();
  Serial.println (F("HTTP server started"));
  fs_initServerOn( );  // server.on(xxx   Spécifiques au log, gestion fichier,OTA,option
  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  // dnsServer.setTTL(300);   // va
  Serial.print(F("Starting dnsServer for captive portal ... "));
  Serial.println(dnsServer.start(DNS_PORT, "*", local_ip) ? F("Ready") : F("Failed!"));
  Serial.print(F("Opening filesystem littleFS ... "));

#ifdef ESP32
  Serial.println(LittleFS.begin(true) ? F("Ready") : F("Failed!"));  // true = format if fail
#else
  Serial.println(LittleFS.begin() ? F("Ready") : F("Failed!"));  // pour ESP8266 format if fail par defaut
#endif
  nettoyageTraces(); // ne garder qu'un nombre limité de traces
}


uint32_t sleep_time_in_ms = 10000;
void  Reveiller_Balise(void)
{
  if (shutdown && preferences.basseConso) {
#ifndef ESP32
    WiFi.forceSleepWake();  // ESP8266
    WiFi.mode(WIFI_STA);
    wifi_set_channel(6);
#else
    WiFi.mode(WIFI_STA);
    esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);  // ESP32
#endif
  }
  delay(0);
  return;
}

void  Endormir_Balise()
{
  if (shutdown && preferences.basseConso) {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
#ifndef ESP32
    WiFi.forceSleepBegin();
#endif
    delay(0);
  }
  return;
}

void dumpTrame(uint8_t* beaconPacket, uint16_t to_send)
{
  for (int i = 0; i <= to_send; i++) {
    Serial.printf("%1i ", ( i / 100));
  }
  Serial.println();
  for (int i = 0; i <= to_send; i++) {
    Serial.printf("%1i ", ( i / 10) % 10);
  }
  Serial.println();
  for (int i = 0; i <= to_send; i++) {
    Serial.printf("%1i ", i % 10);
  }
  Serial.println();
  for (int i = 0; i <= to_send; i++) {
    Serial.printf_P(PSTR("%02X"), beaconPacket[i]);
  }
  Serial.println();
  for (int i = 0; i <= to_send; i++) {
    Serial.print(isPrintable( beaconPacket[i]) ?  char(beaconPacket[i]) : '.') ; Serial.print(" ");
  }
  Serial.println();
}

static void EnvoiTrame(double lat_actu, double lon_actu, int16_t alt_actu, int16_t dir_actu, double vit_actu)
{
  //on définit le code info dans la trame:
  drone_idfr.set_codeinfo(codeinfo);
  //on envoie les données à la biblio pour formattage:
  drone_idfr.set_current_position(lat_actu, lon_actu, alt_actu);
  drone_idfr.set_heading(dir_actu);
  drone_idfr.set_ground_speed(vit_actu);
  drone_idfr.set_heigth(alt_actu - home_alt);

  /**
    On regarde s'il est temps d'envoyer la trame d'identification drone: soit toutes les 3s soit si le drones s'est déplacé de 30m en moins de 3s.
     - soit toutes les 3s,
     - soit si le drone s'est déplacé de 30m en moins de 30s soit 10m/s ou 36km/h,
     - uniquement si la position Home est déjà définie,
     - et dans le cas où les données GPS sont nouvelles.
  */

  // on envoie une trame dès qu'il est temps d'en envoyer une
  if (drone_idfr.time_to_send())
  {
#ifdef pinLed
    // Tant que le fix n'est pas fait, flash du led avec la periode des trame.
    // Quand le fix est fiat, un flash rapide pour chaque trame.
    // Le flash est un peu plus long:brillant en mode économie d'énergie car il y a un certain delay pour endormir/reveiller le wifi
    if (codeinfo == 9)digitalWrite(pinLed, HIGH);
    else digitalWrite(pinLed, TRBcounter % 2);
#endif
    //On commence par renseigner le ssid du wifi dans la trame
    // write new SSID into beacon frame
    // const size_t ssid_size = (sizeof(ssid) / sizeof(*ssid)) - 1; // remove trailling null termination
    size_t ssid_size = strlen(ssid);
    beaconPacket[40] = ssid_size;  // set size
    memcpy(&beaconPacket[41], ssid, ssid_size); // set ssid
    const uint8_t header_size = 41 + ssid_size;  //TODO: remove 41 for a marker

    //On génère la trame wifi avec l'identification
    const uint8_t to_send = drone_idfr.generate_beacon_frame(beaconPacket, header_size);  // override the null termination
    //allume wifi
#ifdef fs_STAT
    statReveiller.T0 = millis();
#endif
    Reveiller_Balise();
    //envoi trame
    unsigned long TT = millis();
#ifdef fs_STAT
    calculerStat (true,  &statReveiller);
    statSendPkt.T0 = millis();
#endif
    //dump une seule fois d'une trame Beacon ...
    if (millis() < 9000) {
      Serial.println("Trame typique:");
      dumpTrame(beaconPacket, to_send);
    }

#ifdef ESP32
    if (shutdown) {
      ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_STA, beaconPacket, to_send, true));
    }
    else  {
      ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_AP, beaconPacket, to_send, true));  // interface mode AP dans mon cas
    }
#else
    //  ESP_ERROR_CHECK( wifi_send_pkt_freedom(beaconPacket, to_send, true));
    wifi_send_pkt_freedom(beaconPacket, to_send, true);
#endif

#ifdef fs_STAT
    calculerStat (true,  &statSendPkt);
    statEndormir.T0 = millis();
#endif
    TRBcounter++;
    //éteindre wifi
    Endormir_Balise();
    //On reset la condition d'envoi
    drone_idfr.set_last_send();
#ifdef fs_STAT
    calculerStat (true,  &statEndormir);
    calculerStat (true, &statBeacon);
    statBeacon.T0 = millis();
    if (statSendPkt.count % 20 == 0) {
      Serial.printf_P(PSTR("%i\tReveiller :%i  %i  %f"), statReveiller.count, statReveiller.min, statReveiller.max, statReveiller.moyenneLocale);
      Serial.printf_P(PSTR("\tSendpkt :%i  %i  %f"), statSendPkt.min, statSendPkt.max, statSendPkt.moyenneLocale);
      Serial.printf_P(PSTR("\tEndormir :%i  %i  %f\n"), statEndormir.min, statEndormir.max, statEndormir.moyenneLocale);
      dbgHeap("fin");
     
    }
#endif
#ifdef pinLed
    if (codeinfo == 9) digitalWrite(pinLed, LOW);
#endif
  }
}

void setup()
{
#ifdef pinLed
  pinMode(pinLed, OUTPUT);
#endif
  Serial.begin(115200);
  //delay(2000);
  Serial.print(F("\n\nBalise v")); Serial.print(versionSoft);
  Serial.println(F("  " __DATE__ " " __TIME__));
  Serial.print(F("GPS: "));
#ifdef GPS_quectel
  Serial.println(F("Quectel"));
#elif defined(GPS_ublox)
  Serial.println(F("Ublox"));
#else
  Serial.println(F("non défini"));
#endif
  Serial.printf_P(PSTR("Pin %i sur RX du GPS\nPin %i sur TX du GPS\n"), GPS_TX_PIN, GPS_RX_PIN);

  EEPROM.begin(512);
  checkFactoryReset();
  readPreferences();
  Serial.println(F("Prefs lues dans EEPROM:"));
  listPreferences();
  // init liaison GPS. Cette vitesse sera changée en fait lors de la configuration du GPS
#if defined(ESP32)
  serialGPS.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN );
#else
  serialGPS.begin(9600);
  if (!serialGPS) { // If the object did not initialize, then its configuration is invalid
    Serial.println(F("Invalid SoftwareSerial pin configuration, check config"));
    while (1) { // Don't continue with invalid configuration
      delay (1000);
    }
  }
#endif

  local_ip.fromString(preferences.local_ip);
  gateway.fromString(preferences.gateway);
  subnet.fromString(preferences.subnet);

  //connection sur le terrain à un smartphone
  // start WiFi
  WiFi.mode(WIFI_AP);
  //conversion de l'adresse mac:
  String temp = WiFi.macAddress();
  temp.replace(":", ""); //on récupère les 12 caractères
  strcpy(&drone_id[18], temp.c_str()); // que on met à la fin du drone_id
  Serial.print(F("Drone_id:")), Serial.println(drone_id);
  temp = WiFi.macAddress();
  //concat du prefixe et de l'adresse mac
  temp = String(prefixe_ssid) + "_" + temp;
  //transfert dans la variable globale ssid
  temp.toCharArray(ssid, 32);
  if (strlen(preferences.ssid_AP) != 0 ) strcpy (ssid, preferences.ssid_AP);
  else strcpy (preferences.ssid_AP, ssid);
  Serial.print(F("Setting soft-AP configuration ... "));
  Serial.println(WiFi.softAPConfig(local_ip, gateway, subnet) ? F("Ready") : F("Failed!"));
  Serial.print(F("Setting soft-AP ... "));
  // ssid, pwd, channel, hidden, max_cnx
  Serial.println(WiFi.softAP(ssid, preferences.password, 6, false, 4) ? F("Ready") : F("Failed!"));  // 4 clients peris
  // Serial.println(WiFi.softAP(ssid, preferences.password, 6, false, 1) ? F("Ready") : F("Failed!")); // 1 client permis
  Serial.print(F("IP address for AP network "));
  Serial.print(ssid);
  Serial.print(F(" : "));
  Serial.println(WiFi.softAPIP());

  beginServer(); //lancement du server WEB, DNS, file system
  delay(1200); // le GPS QUECTEL met environ 1.1s pour être pret après un power up et ne reçoit pas les commandes avant. Ubloxd : environ 600ms
  fs_initGPS(preferences.baud, preferences.hz);  // init du GPS (vitesse, refresh) et serialGPS
  drone_idfr.set_drone_id(drone_id);

#ifdef fs_STAT
  razStatistics();
#endif
  Serial.println(F("Attente du fix & Co"));
}



void loop()
{
#ifdef fs_RECEPTEUR
  if (modeRecepteur)
  {
    server.handleClient();
    dnsServer.processNextRequest();
    loopRecepteur();
    return;// doit relancer loop
  }
#endif
  //  Gestion du shutdown du Point Acces Wifi
  if (!has_set_home || !countdownRunning) TimeShutdown = millis() + preferences.timeoutWifi * 1000;
  if  ( preferences.arretWifi && !shutdown && (millis() > TimeShutdown && countdownRunning  ||  vitesse > 2)) { // 2m/S = 7.2km/h
    Serial.printf_P(PSTR("!!!!!!!  Shutdown. millis:%u  TimeShutdown:%u  vitesse:%i\n"), millis(), TimeShutdown, vitesse);
    //   Serial.printf_P(PSTR("WiFi.softAPdisconnect: %s\n"), WiFi.softAPdisconnect(true) ? F("OK") : F("Failed!"));
    Serial.printf_P(PSTR("WiFi.mode: %s\n"), WiFi.mode(WIFI_STA) ? F("OK") : F("Failed!"));
    // la balise
#ifdef ESP32
    esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);  // ESP32
#else
    wifi_set_channel(6);
#endif
#ifdef STAT
    razStatistics();
#endif
    delay(100);
    shutdown = true;
  } else if (!shutdown) {
    // AP toujours actif
#ifdef fs_STAT
    statServer.T0 = millis();
#endif
    server.handleClient();
#ifdef fs_STAT
    calculerStat (true, &statServer);
#endif
    dnsServer.processNextRequest();
  }

  // Ici on lit les données qui arrivent du GPS et on les passe à la librairie TinyGPS++ pour les traiter
  while (serialGPS.available()) {
    cGPS = serialGPS.read();
    gps.encode(cGPS);
  }
  yield();
  saveLocationIsUpdated = false;
  vit_prev = 0; // restera à 0 en cas de problème avec le GPS. Sionon sera mis à jour.
  // et on commence par filtrer tous les cas possibles d'erreur
  // On traite le cas où le GPS a un problème
  if (millis() > 5000 && gps.charsProcessed() < 10)
    codeinfo = 1;
  // On traite le cas si la position GPS n'est pas valide
  else if (!gps.location.isValid() || !gps.date.isValid() )
    codeinfo = 2;
  else if (gps.location.age() > 3000)
    codeinfo = 3;
  // test si précision ok, sinon on remonte au début afin de ne pas encore fixer la home position
  else if (gps.satellites.value() < 4)
    // La précision du GPS n'est pas bonne
    // Nb de satellites (doit être supérieur à 3): " + String(gps.satellites.value()));
    codeinfo = 4;
  else if (gps.hdop.hdop() > 5)
    //La précision du GPS n'est pas bonne
    //Précision 2D (doit être inférieure à 5.0): " + String(gps.hdop.hdop()));
    codeinfo = 5;
  else if (gps.altitude.meters() == 0)
    //La précision du GPS n'est pas bonne
    //Altitude non récupérée: " + String(gps.altitude.meters()));
    codeinfo = 6;
  else { // ici on n'est plus en erreur. location/date/age/satellites/hdop/altitude sont OK.
    saveLocationIsUpdated = gps.location.isUpdated();
    if (!has_set_home)
      // On renseigne une première et unique fois le point de démarrage dès que la précision est satisfaisante
    {
      Serial.println(F("Stockage de la position de départ"));
      home_alt = gps.altitude.meters();
      //on fixe la position home:
      drone_idfr.set_home_position(gps.location.lat(), gps.location.lng(), home_alt);
      has_set_home = true;
      latLastLog = gps.location.lat();
      lngLastLog = gps.location.lng();
      //on envoie une trame
      codeinfo = 7;
    } else
      codeinfo = 9;

    // si ici, pas d'erreur donc:
    //  on a un codeinfo 7 si on vient de definir la home position, ou 9 en fonctionnement normal
    //on stocke les dernières positions
    lat_prev = gps.location.lat();
    lon_prev = gps.location.lng();
    alt_prev = gps.altitude.meters();
    dir_prev = gps.course.deg();
    vit_prev = gps.speed.mps();  // metre /sec
    vitesse = vitesse * 0.8 + vit_prev * 0.2; // un peu de lissage
    //vitesse = vit_prev;
    VmaxCockpit = max(VmaxCockpit, float(vit_prev * 3.7));
    AltmaxCockpit = max(AltmaxCockpit, float(alt_prev));
    VmaxSegment = max(VmaxSegment, float(gps.speed.kmph()));
    // AltMaxSegment = max(AltMaxSegment, float(alt_prev));
    AltMaxSegment = max(AltMaxSegment, float(gps.altitude.meters())); // alt_prev est un entier double...
  }
  // Tout est bon si codeinfo =9 ou 7. Sinon envoyer une trame avec les coordonnées &Co précédentes
  // (sauf pour la vitesse qui a été mise à 0 au début, pour ne pas avoir de problème avec l'estimation de distance parcourue
  //  dans has_pass_distance()  )
  //
  EnvoiTrame(lat_prev, lon_prev, alt_prev, dir_prev, vit_prev);  // envoie éventuel. Autres conditions testées dans la fonction
  // on regarde si on doit enregistrer un point dans la trace
  //  IL faut au moins avoir eu un fix, des nouvelles données.


  if (  preferences.logOn && drone_idfr.has_home_set() && !fileSystemFull  ) {
    float segmentf = distanceSimple(gps.location.lat(), gps.location.lng(), latLastLog, lngLastLog);
    // Si le fix existe :rejet de points abberrants (problème de communication avec le GPS ...). Surtout pour ESP8266
    if (drone_idfr.has_home_set() && ( gps.location.lat() == 0 || gps.location.lng() == 0 || (preferences.logAfter > 0) && segmentf > 30 * preferences.logAfter)) {
      // point GPS aberrant: enregistrer l'erreur et l'ignorer ...
#ifdef fs_STAT
      nbrLogError++;
      //   Serial.printf_P(PSTR("%f  %f  %f  %f  %f\n"), segmentf, gps.location.lat(), gps.location.lng(), latLastLog, lngLastLog);
#endif
    } else {  // point GPS OK
      // fermer le fichier trace si on ne bouge plus: avion posé ?, on risque la coupure de courant
      if (vitesse <= 0.1) { // ???  0.36km/h  . En fait Default speed threshold: 0.4 m/s
        //      if ( immobile && millis() - T0Immobile > 2000 && preferences.logAfter > 0 && traceFileOpened ) { // fermer si mode distance uniquement
        if ( immobile && millis() - T0Immobile > 2000  && traceFileOpened ) { // fermer dans tous les cas
          //a l'arret depuis plus de 2000s.: fermer le fichier trace.
          traceFile.close();
          traceFileOpened = false;
          // Serial.print(F("-----------close tracefile vitesse:")); Serial.println(vitesse);
        } else if (!immobile) {  // début immobilité
          immobile = true;
          T0Immobile = millis();
        }
      } else {  // fin immobilité
        immobile = false;
      }
      // Si preferences.logAfter > 0 :On stocke un point de trace si le nouveau segment et plus long que preferences.logAfter
      // Si preferences.logAfter < 0 :On ne tient plus compte de la longeur du segment, mais on stocke
      //   un point de trace toutes les abs(preferences.logAfter)  ms ( il faut au moins 70ms ??)
      //   et uniquementsi on s'est un peu déplacé depuis le dernier log ou si l'option logToujours est true.
      if (((preferences.logAfter > 0) && (segmentf > preferences.logAfter))
          || ((preferences.logAfter < 0) && (millis() - T1Log > abs (preferences.logAfter)  )
              && (segmentf > 1.0 || preferences.logToujours) ) ) {
        // Serial.print(F("Demande ecr trace. segmentf: ")); Serial.println(segmentf);
        T1Log = millis();
#ifdef fs_STAT
        statLog.T0 = millis();
#endif
        if ( !fs_ecrireTrace (gps)) {
          Serial.println(F("File system full ? "));
          fileSystemFull = true;
          messAlarm = "<span class='b2'>Mémoire pleine ?</span>";
        } else {
          // ne mettre a jour les stats que si ecriture est faite. (sinon le disque est plein ..)
          fileSystemFull = false;
          messAlarm = "";
#ifdef fs_STAT
          calculerStat (true, &statLog);
          statSegment.T0 = segmentf;
          calculerStat (false, &statSegment);
#endif
          countLog++;

          if (countLog % 300 == 0) {
            traceFile.flush(); // forcer la mise a jour tous les n points
          }
        }
        latLastLog = gps.location.lat();
        lngLastLog = gps.location.lng();
      }
    }
  }  // test si il faut ecrire un point
#ifdef fs_STAT
  // stats sur une tour complet de la boucle
  calculerStat (true, &statLoop);
  statLoop.T0 = millis();
#endif
}
