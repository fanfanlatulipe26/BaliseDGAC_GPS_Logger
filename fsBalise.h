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
struct pref{  // preferences sauvées en EEPROM
  char signature[6] = "balfs";  // balfs  signature d'une EEPROM balise. Ne pas toucher
  char password[9] ="";  //  Pa défaut le réseau créé est ouvert
  char local_ip[16] = "192.168.4.1";    // c'est de toutes faons la valeur par défaut pour une softAP"8.8.8.8";    // 255.255.255.255     15 caractères max
  char gateway[16] = "192.168.4.1" ; // "8.8.8.8";
  char subnet[16] = "255.255.255.0";
  int logAfter = 0 ;   // en mètre. 0= debut du log dès le fix, sinon après déplacement initial de n mètre
  bool logOn = true;
  bool logVitesse = false;
  bool logAltitude = false;
  bool logHeure = true;
};

bool fs_logEcrire(TinyGPSPlus &gps);
bool removeOldest();
bool loadFromSPIFFS(String path);
void sendChunkDebut ( bool avecTopMenu );
void handleCockpit_();
void listSPIFFS(String message);
void handleDelete();
void handleFormatage();
void gestionSpiff(String message);
void handleNotFound();
void checkFactoryReset();
void readPreferences();
void savePreferences();
void listPreferences();
void handleOptionLogProcess();
void handleOptionWifiProcess();
void handleOptionsPreferences();
void handleResetUsine();
void handleReset();
void fs_initServerOn( );
void displayOptionsSysteme(String sms);
void handleOptionsSysteme();
void handleOTA_();
void  fs_initServerOnOTA(ESP8266WebServer &server);
