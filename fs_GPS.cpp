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
#include "fsBalise.h"
#include "fs_GPS.h"
#include "fs_options.h"

#if defined(ESP32)
#include <HardwareSerial.h>
extern HardwareSerial serialGPS;
#else
#include <SoftwareSerial.h>
extern SoftwareSerial serialGPS;
#endif

//  Changer la vitesse de la liasion serie avec le GPS (HardwareSerial Avec ESP32, SoftwareSerial avec ESP8266)
void setSerialGpsSpeed(uint32_t baud) {
  // Serial.printf_P(PSTR("setSerialGpsSpeed(%u)\n "), baud);
#if defined(ESP32)
  while (serialGPS.available()) {
    serialGPS.read();
  }
  serialGPS.flush();
  serialGPS.updateBaudRate(baud);
#else
  serialGPS.flush();
  serialGPS.end();
  serialGPS.begin(baud);
#endif
}

#ifdef GPS_quectel
void setGpsRate(uint32_t rate) {
  char buff [25];
  // 1hz  periode 1000ms,  10 hz : 100, 5hz: 200
  // (pour 9600bds, dans le choix des préférences on interdit > 6hz car des pb ....
  sprintf_P(buff, PSTR("$PMTK220,%u"), 1000 / rate);
  addChecksum(buff);
  delay(100);  // mis pour test refresh rate perdu pafois
  serialGPS.println(buff);
}
void limitNmeaSentences() {
  // limiter le trafic aux trames $GPGA et $GPRMC
#define PMTK314_API_SET_NMEA_OUTPUT_GGA_RMC "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
  // exclure  les trames GPTXT
#define PQTXT_DISABLE_NOSAVE "$PQTXT,W,0,0*22"
  serialGPS.println(F(PMTK314_API_SET_NMEA_OUTPUT_GGA_RMC));
  serialGPS.println(F(PQTXT_DISABLE_NOSAVE));
}
// initialise le GPS a la bonne vitesse / frequence rafraichissement.
// initialise la liaison serie gps
// limite les trames NMEA envoyées par le GPS
void fs_initGPS(uint32_t baud, uint32_t rate) {
  //This message is used to query the rate of position fixing activity.
#define PMTK400_API_Q_FIX_CTL  "$PMTK400*36"   // reponse par $PMTK500
  setGpsSpeed(baud);
  setSerialGpsSpeed(baud);
  limitNmeaSentences();
  setGpsRate(rate);
  //serialGPS.println("$PMTK285,4,100*38"); // pps always on, pulse de 100ms
  //serialGPS.println("$PMTK285,4,100*38"); // pps always on, pulse de 100ms
  //serialGPS.println("$PMTK285,4,100*38"); // pps always on, pulse de 100ms
  serialGPS.println(F(PMTK400_API_Q_FIX_CTL));
  echoGPS(1500);  // pour vérifier que la vitesse a bien été prise en compte ....
}

// initialise la vitesse de communication du GPS, et la ligne serialGPS
void setGpsSpeed(uint32_t baud) {
  char buff [25];
  // surtout utile en developement/test, lors de warm start.
  //  car après un power down/up le GPS est a 9600bds, maj 1hz
  // on ne sait pas a quelle vitesse le GPS communique actuellement.
  // On va lui envoyer la commande sur toutes les vitesses possibles ...
  // Semble très sensible au timing ...
  // int speedTable[] = {9600, 14400, 19200, 38400, 57600, 115200};
  int speedTable[] = {9600, 19200, 38400, 57600, 115200};
  // int speedTable[] = {115200, 57600, 38400, 19200, 9600};
  sprintf_P(buff, PSTR("$PMTK251,%u"), baud);
  addChecksum(buff);
  for ( int i = 0; i < sizeof (speedTable) / sizeof (speedTable[0]); i++) {
    setSerialGpsSpeed(speedTable[i]);
    limitNmeaSentences();
    // avant de changer la vitesse, forcer le rafraichissement à 1Hz, car si on est déja configuré
    //  avec un rafraichissement élevé et que l'on passe à 9600bds, il y a blocage et pas de changement de vitesse.??
    setGpsRate(1);
    serialGPS.println(buff); // parfois des commandes de changements de vitesse sont perdues ...
    serialGPS.println(buff);
    serialGPS.println(buff);
    delay(150); // des pb si pas ce delay ...
  }
}
// rajout du checksum sur commande pour GPS.
// Le buffer d'entrée doit avoir AU MOINS 3 caractères de plus que la commande
void addChecksum(char *buff) {
  char cs = 0;
  int i = 1;
  while (buff[i]) {
    cs ^= buff[i];
    i++;
  }
  sprintf_P(buff, PSTR("%s*%02X"), buff, cs);
}
//fin code GPS_quectel

#elif defined (GPS_ublox)
// initialise le GPS a la bonne vitesse / frequence rafraichissement.
// initialise la liaison serie gps
// limite les trames NMEA envoyées par le GPS
void fs_initGPS(uint32_t baud, uint32_t rate) {
  //This message is used to query the rate of position fixing activity.
  Serial.printf_P(PSTR("fs_initGPS %u  %u\n"), baud, rate);
  setGpsSpeed(baud);
  setSerialGpsSpeed(baud);
  limitNmeaSentences();
  setGpsRate(rate);
  echoGPS(1500);  // pour vérifier que la vitesse a bien été prise en compte ....
}

// initialise la vitesse de communication du GPS, et la ligne serialGPS
void setGpsSpeed(uint32_t baud) {
  char buff [35];
  // surtout utile en developement/test, lors de warm start.
  //  car après un power down/up le GPS est a 9600bds, maj 1hz
  // on ne sait pas a quelle vitesse le GPS communique actuellement.
  // On va lui envoyer la commande sur toutes les vitesses possibles ...
  // Semble très sensible au timing ...
  // int speedTable[] = {9600, 14400, 19200, 38400, 57600, 115200};
  int speedTable[] = {9600, 19200, 38400, 57600, 115200};
  // int speedTable[] = {115200, 57600, 38400, 19200, 9600};
  // $PUBX,41,portId,inProto,outProto,baudrate,autobauding*cs<CR><LF>
  //Example:
  //$PUBX,41,1,0007,0003,19200,0*25
  sprintf_P(buff, PSTR("$PUBX,41,1,0007,0003,%u,0"), baud);
  addChecksum(buff);
//  Serial.print(F("Commande vitesse:")); Serial.println(buff);
  for ( int i = 0; i < sizeof (speedTable) / sizeof (speedTable[0]); i++) {
    setSerialGpsSpeed(speedTable[i]);
    delay(1000);   //   semble eviter le disable de l'UART / frame error pendant 1s
    /*
      As of Protocol version 18+, the UART RX interface will be disabled when more than 100
      frame errors are detected during a one-second period. This can happen if the wrong
      baud rate is used or the UART RX pin is grounded. The error message appears when the
      UART RX interface is re-enabled at the end of the one-second period
      $GNTXT,01,01,01,More than 100 frame errors, UART RX was disabled*70
    */
    limitNmeaSentences();
    // avant de changer la vitesse, forcer le rafraichissement à 1Hz, car si on est déja configuré
    //  avec un rafraichissement élevé et que l'on passe à 9600bds, il y a blocage et pas de changement de vitesse.??
    setGpsRate(1);
    serialGPS.println(buff);
  }
}
void limitNmeaSentences() {
  // limiter le trafic aux trames $GPGA et $GPRMC
  serialGPS.println(F("$PUBX,40,GSV,0,0,0,0,0,0*59"));
  serialGPS.println(F("$PUBX,40,GSA,0,0,0,0,0,0*4E"));
  serialGPS.println(F("$PUBX,40,GLL,0,0,0,0,0,0*5C"));
  serialGPS.println(F("$PUBX,40,VTG,0,0,0,0,0,0*5E"));
}
// rajout du checksum sur commande pour GPS.
// Le buffer d'entrée doit avoir AU MOINS 3 caractères de plus que la commande
void addChecksum(char *buff) {
  char cs = 0;
  int i = 1;
  while (buff[i]) {
    cs ^= buff[i];
    i++;
  }
  sprintf_P(buff, PSTR("%s*%02X"), buff, cs);
}
// Envoie au GPS une commande pour changer de frequence de refresh
//  ubx procole seulement ...
void setGpsRate(uint32_t rate) {
  byte packet_rate[] = {0xB5, 0x62, 0x06, 0x08, // UBX-CFG-RATE Navigation/measurement rate settings
                        0x06, 0x00, // length
                        0xF4, 0x01, // measRate. Ici 0xF4 0x01 = 500ms 2hz
                        0x01, 0x00, // navRate
                        //                      0x01, 0x00, // timeRef . Ici 1 =GPS time
                        0x00, 0x00, // timeRef . Ici 0 =UTC time
                        0x0B, 0x77  //
                       };
//  Serial.printf_P(PSTR("setGpsRate(%u) "), rate);
  uint32_t period = 1000.0 / rate;
//  Serial.printf_P(PSTR("  period:%u (%X):\n"), period);
  packet_rate[6] = period;
  packet_rate[7] = period >> 8;
  crc_ublox(packet_rate, sizeof(packet_rate));
  delay(200) ; //
  sendPacket(packet_rate, sizeof(packet_rate));
  delay(200) ; //
  sendPacket(packet_rate, sizeof(packet_rate));  //
  delay(200) ; //
}
// calcul checksum^pour protocole UBX. Le message est mis à jour.
void crc_ublox( byte *msg, uint32_t size)
{
  uint32_t s = size;
  uint8_t CK_A = 0, CK_B = 0;
  for ( uint8_t i = 2; i < (s - 2) ; i++)
  {
    CK_A = CK_A + msg[i];
    CK_B = CK_B + CK_A;
  }
  msg[s - 2] = CK_A;
  msg[s - 1] = CK_B;
}
// Send the packet specified to the receiver.
void sendPacket(byte * packet, byte len) {
  for (byte i = 0; i < len; i++)
  {
    serialGPS.write(packet[i]);
  }
  delay(200);
}

// fin code GPS_ublox
#else
void fs_initGPS(uint32_t baud, uint32_t rate) {
  Serial.println(F("++ GPS déjà configuré: pas de reconfiguration possible"));
  echoGPS(1500);  // pour vérifier que la vitesse a bien été prise en compte ....
}
#endif

void echoGPS(int duree) {
  long T0;
  char car;
  Serial.println(F("--Début echo GPS"));
  T0 = millis();
  while (millis() < T0 + duree) {
    while (serialGPS.available()) {
      car = serialGPS.read();
      //   Serial.print(car,HEX);
      Serial.print(char(car));
    }
  }
  Serial.println(F("\n--Fin echo GPS"));
}
// Calcul simple, rapide de distance entre 2 points GPS voisins
// calcul distance en metre
float distanceSimple(float lat1, float lon1, float lat2, float lon2) {
  /*
    distance = sqrt(dx * dx + dy * dy)
    with distance: Distance in km
    dx = 111.3 * cos(lat) * (lon1 - lon2)
    lat = (lat1 + lat2) / 2 * 0.01745
    dy = 111.3 * (lat1 - lat2)
    lat1, lat2, lon1, lon2: Latitude, Longitude in degrees
  */
  float lUn = 20003931.5 / 180.0; // longeur 1 degre méridien
  float lat = (lat1 + lat2) / 2 * 0.01745;
  float dx = lUn * cos(lat) * (lon1 - lon2);
  float dy = lUn * (lat1 - lat2);
  return sqrt(dx * dx + dy * dy);
}
