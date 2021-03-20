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
#include "fs_GPS.h"
#include "fsBalise.h"
#include <SoftwareSerial.h>
extern SoftwareSerial softSerial;
extern pref preferences;
#define PMTK314_API_SET_NMEA_OUTPUT_GGA_RMC "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
#define PQTXT_DISABLE_NOSAVE "$PQTXT,W,0,0*22"
#define PMTK220_SET_POS_FIX_1HZ "$PMTK220,1000*1F" // 1 Hz
#define PMTK400_API_Q_FIX_CTL  "$PMTK400*36"   // reponse par $PMTK500

void fs_initGPS() {
  char buff [25];
  // softSerial.begin(38400);
  // return;    // à déommenter si le GPS a une configuration satisfaisante au cold start.
  setGpsSpeed(preferences.baud);
  // limiter le trafic aux trames $GPGA et $GPRMC
  softSerial.println(PMTK314_API_SET_NMEA_OUTPUT_GGA_RMC);
  // exclure  les trames GPTXT
  softSerial.println(PQTXT_DISABLE_NOSAVE);
  // 1hz  periode 1000ms,  10 hz : 100, 5hz: 200
  // (pour 9600bds, dans le choix des préférences on interdit > 6hz car des pb ....
  sprintf(buff, "$PMTK220,%u", 1000 / preferences.hz);
  addChecksum(buff);
  softSerial.println(buff);
  softSerial.println(PMTK400_API_Q_FIX_CTL);  // pour debug et verifier le 10hz.
  echoGPS(1000);  // pour debug
}
// initialise la vitesse de communication du GPS, et la ligne softserial
void setGpsSpeed(int baud) {
  char buff [25];
  // surtout utile en developement/test, lors de warm start.
  //  car après un power down/up le GPS est a 9600bds, maj 1hz
  // on ne sait pas a quelle vitesse le GPS communique actuellement.
  // On va lui envoyer la commande sur toutes les vitesses possibles ...
  // Semble très sensible au timing ...
  // int speedTable[] = {9600, 14400, 19200, 38400, 57600, 115200};
  int speedTable[] = {9600, 19200, 38400, 57600, 115200};
  sprintf(buff, "$PMTK251,%u", baud);
  addChecksum(buff);
  for ( int i = 0; i < sizeof (speedTable) / sizeof (speedTable[0]); i++) {
    // Serial.print("   vitesse:"); Serial.println(speedTable[i]);
    delay(300);  // obligatoire. 300 ? ou moins ?? ++++++++++++++++++++++
    softSerial.flush();
    delay(100);
    softSerial.end();
    delay(100);
    softSerial.begin(speedTable[i]);
    delay(150); // obligatoire ++++++++++++++++++++++++++++++
    // avant de changer la vitesse, forcer le rafraichissement à 1Hz, car si on est déja configuré
    //  avec un rafraichissement élevé et que l'on passe à 9600bds, il y a blocage et pas de changement de vitesse.
    softSerial.println(PMTK220_SET_POS_FIX_1HZ);   // touchy à 9600bds ...
    softSerial.println(PMTK220_SET_POS_FIX_1HZ);
    softSerial.println(buff); // parfois des commandes de changements de vitesse sont perdues ...
    softSerial.println(buff);
    softSerial.println(buff);
  }
  delay(200);
  softSerial.flush();
  softSerial.end();
  Serial.print("+++ softSerial initialisé à "); Serial.print(baud);
  softSerial.begin(baud);
  delay(100);
  Serial.print("\t"); Serial.println(softSerial.baudRate());
  delay(200);
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
  sprintf(buff, "%s*%02X", buff, cs);
}

// echo du GPS pendant "duree" milli
void echoGPS(int duree) {
  long T0;
  byte car;
  T0 = millis();
  while (millis() < T0 + duree) {
    while (softSerial.available()) {
      car = softSerial.read();
      Serial.print(char(car));
      // gps.encode(car);
    }
  }
  Serial.println();
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
