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
*/
#include <ESP8266WebServer.h>
#include <TinyGPS++.h>

// voir pour adresse DNS 8.8.8.8 FS https://github.com/espressif/arduino-esp32/issues/1037
//  pour Android ??
//enum { traceNone, traceGPX, traceCSV };

struct pref { // preferences sauvées en EEPROM
  char signature[6] = "ablfs";  // balfs  signature d'une EEPROM balise. Ne pas toucher
  char password[9] = ""; //  Pa défaut le réseau créé est ouvert
  char local_ip[16] = "192.168.4.1";    // c'est de toutes facons la valeur par défaut pour une softAP"8.8.8.8";    // 255.255.255.255     15 caractères max
  char gateway[16] = "192.168.4.1" ; // "8.8.8.8";
  char subnet[16] = "255.255.255.0";
  int logAfter = 5 ;   // enregistrement d'un point si déplacement de 5m  (si <0, enregistrement full speed pour test ...)
  int baud = 38400;  // vitesse de transmission avec le GPS   38400 57600
  byte hz = 10;  //  taux de rafraichissement du GPS
  char formatTrace[4] = "csv";  //  csv gpx
  bool logNo = false;  // true: no log/trace;  false: on enregistre
  bool logVitesse = false;
  bool logAltitude = true;
  bool logHeure = true;
};
// type pour structure servant à stocker en binaire une ligne (un point) du log de trace: 20 octets par point
struct trackLigne_t { 
    float lat;  // double pas nécessaire ...
    float lng;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t centisecond;
    float altitude; // double pas ncessaire ...
    float speed;
  };
bool fs_ecrireTrace(TinyGPSPlus &gps);
bool removeOldest();
bool loadFromSPIFFS(String path);
void sendChunkDebut ( bool avecTopMenu );
void handleCockpit();
void listSPIFFS(String message);
void handleDelete();
void handleFormatage();
void gestionSpiff(String message);
void handleGestionSpiff();
void handleNotFound();
void checkFactoryReset();
void readPreferences();
void savePreferences();
void listPreferences();
void handleOptionLogProcess();
void handleOptionWifiProcess();
void handleOptionGPSProcess();
void handleOptionsPreferences();
void handleResetUsine();
void handleReset();
void fs_initServerOn( );
void displayOptionsSysteme(String sms);
void handleOptionsSysteme();
void handleOTA_();
void  fs_initServerOnOTA(ESP8266WebServer &server);
void logError(TinyGPSPlus &gps, float latLastLog,float lngLastLog, float segment);
void logRingBuffer(char ringBuffer[], int sizeBuffer, int indexBuffer);
