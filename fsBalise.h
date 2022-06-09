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
#ifdef ESP32
#include "fs_WebServer.h"
#else
#include <ESP8266WebServer.h>
#endif

#include <TinyGPS++.h>

const char versionSoft[] = "3.0";
#define DEBUG_HEAP false
#ifndef ESP32
#define dbgHeap(mes) \
  do { if (DEBUG_HEAP) Serial.printf_P(PSTR("%s %s():%d  FreeContStack: %5d  free: %5d - max: %5d - frag: %3d%% \n "),\
                                         mes, __func__, __LINE__,ESP.getFreeContStack(),\
                                         ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(),\
                                         ESP.getHeapFragmentation());} while (0)
#else
#define dbgHeap(mes) \
  do { if (DEBUG_HEAP) Serial.printf_P(PSTR("%s %s():%d free: %5d \n "),\
                                         mes, __func__, __LINE__,\
                                         ESP.getFreeHeap());} while (0)
#endif

// voir pour adresse DNS 8.8.8.8 FS https://github.com/espressif/arduino-esp32/issues/1037
//  pour Android ??
//enum { traceNone, traceGPX, traceCSV };
struct pref { // preferences sauvées en EEPROM
  char signature[6] = "ddd";  // signature d'une EEPROM balise. A changer si on change les valeurs par defaut ou le format des references
 //  char signature[6] = "ablfs";  // balfs  signature d'une EEPROM balise. Ne pas toucher
  char password[9] = ""; //  Par défaut le réseau créé est ouvert
  char ssid_AP[33] = ""; // Par défaut le ssid du point d'accés crée sera basé sur l'adresse MAC

  char local_ip[16] = "124.213.16.29"; // vu dans  https://gitlab.com/defcronyke/wifi-captive-portal-esp-idf
  char gateway[16] = "124.213.16.29" ; 
  char subnet[16] = "255.0.0.0";
  
  //char local_ip[16] = "192.168.4.1";    // c'est de toutes facons la valeur par défaut pour une softAP"8.8.8.8";    // 255.255.255.255     15 caractères max
  //char gateway[16] = "192.168.4.1" ; // "8.8.8.8";
  //char subnet[16] = "255.255.255.0";
  int timeoutWifi = 45;  // delais deconnection AP wifi
  int logAfter = 5 ;   // enregistrement d'un point si déplacement de 5m  (si <0, enregistrement après n millis sec)
  int baud = 9600;  // vitesse de transmission avec le GPS   19200 ((OK avec ESP8266), 38400 
  int nbrMaxTraces = 10; // nobre maximal de traces à conserver
  byte hz = 1;  //  taux de rafraichissement du GPS
  char formatTrace[4] = "csv";  //  csv gpx
  bool logOn = true;  // true: on enregistre; false: pas de trace enregistrée
  bool logToujours= true;  // enregistrerla trace même si on ne se déplace pas
  bool logVitesse = false;
  bool logAltitude = true;
  bool logHeure = true;
  bool arretWifi = true;  // false: ne pas arreter le point d'accès Wifi en vol;  true: arreter le point d'accès
  bool basseConso = false;  // true: couper le wifi entre 2 trames.
};
// type pour structure servant à stocker en binaire une ligne (un point) du log de trace: 20 octets par point
struct trackLigne_t {
  float lat;  // double pas nécessaire ...
  float lng;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t centisecond;
  float altitude; // double pas necessaire ...
  float speed;
  float VmaxSegment; // vitesse max vue entre ce point et le point précédent
  float AltMaxSegment;
};

// type pour un bloc d'info de statistiques
struct statBloc_t {
  char nom[25];
  int min, max, total, sousTotal, count;
  long T0;
  float moyenneLocale;
};

bool fs_ecrireTrace(TinyGPSPlus &gps);
bool removeOldest();
bool loadFromSPIFFS(String path);
void sendChunkDebut ( bool avecTopMenu );
void handleCockpit();
void handleGiveMeTime();
void handleRazVMaxHMAx();
void handleCockpitNotFound();
void listSPIFFS(String message);
void handleDelete();
void handleFileInfo();
void handleFormatage();
void gestionSpiff(String message);
void handleGestionSpiff();
void handleNotFound();
void checkFactoryReset();
void readPreferences();
void savePreferences();
void listPreferences();
void handleOptionLogProcess();
void handleOptionGPSProcess();
void handleOptionPointAccesProcess();
void handleOptionsPreferences();
void handleResetUsine();
void handleReset();
void handleFavicon();
void handle_generate_204();
void fs_initServerOn( );
void displayOptionsSysteme(String sms);
void handleOptionsSysteme();
void handleOTA_();
void handleResetStatistics();
void razStatistics();
void nettoyageTraces();
void razStatBloc(statBloc_t* bl);
void calculerStat (boolean calculerTemps, statBloc_t * bl);
int writeStatBloc(char buf[], int size, statBloc_t* bl);
#ifdef ESP32
void  fs_initServerOnOTA(fs_WebServer &server);
#else
void  fs_initServerOnOTA(ESP8266WebServer &server);
#endif
void logError(TinyGPSPlus &gps, float latLastLog, float lngLastLog, float segment);
void logRingBuffer(char ringBuffer[], int sizeBuffer, int indexBuffer);
