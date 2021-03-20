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
#include "fsBalise.h"
#include "fs_GPS.h"
#include "fs_options.h"
#include "fs_pagePROGMEM.h"
#include <LittleFS.h>
#include <EEPROM.h>
extern ESP8266WebServer server;
extern boolean traceFileOpened;
extern bool fileSystemFull;
extern String messAlarm;

extern TinyGPSPlus gps;

struct pref preferences;
struct pref factoryPrefs;

void sendChunkDebut ( bool avecTopMenu ) {
  server.chunkedResponseModeStart(200, "text/html");
  server.sendContent_P(fs_style);
  if (avecTopMenu)server.sendContent_P(topMenu);
}
void handleCockpit() {
  sendChunkDebut ( true );  // debut HTML style, avec topMenu
  server.sendContent_P(cockpit);
  server.chunkedResponseFinalize();
}

// Ecriture d'une ligne csv ou GPX dans le fichier de log.
// Attention: le fichier GPX a une trace non fermer. Le compléter lors  d'un download
// Si le file system est plein, on efface les fichiers les plus anciens.
// Dans ce cas on perdra une ligne de log
// Return:  true   ecriture OK; false sinon
//    (le file system est surement plein, avec un seul fichier
//

void logRingBuffer(char ringBuffer[], int sizeBuffer, int indexBuffer) {
  // char logFileName[] = "logRingBuf.txt";
  char logFileName[] = "errorlog.txt";
  File fileToAppend = LittleFS.open(logFileName, "a");
  fileToAppend.println("\n=====================");
  fileToAppend.printf("%u bauds,%uHz\n", preferences.baud, preferences.hz);
  fileToAppend.println(indexBuffer);
  fileToAppend.write(ringBuffer + indexBuffer, sizeBuffer - indexBuffer);
  fileToAppend.write(ringBuffer, indexBuffer);
  fileToAppend.println();
  fileToAppend.close();
  yield();
}

// Problème détecté avec la lecture du GPS. On enregistre des infos dans un log d'erreur.
void logError(TinyGPSPlus &gps, float latLastLog, float lngLastLog , float segment) {
  char logFileName[] = "errorlog.txt";
  bool isFileExist = LittleFS.exists(logFileName);
  File fileToAppend = LittleFS.open(logFileName, "a");
  //  if (!isFileExist) { //Header
  fileToAppend.println("\nlocation.isValid,lat,lng,latLastLog,lngLastLog,segment,speed_km,alt_m,"
                       "sat,hdop,year,month,day,hour,min,sec,centi,failedChecksum,passedChecksum\n");
  // }
  fileToAppend.printf("%s", gps.location.isValid() ? "T" : "F");
  fileToAppend.printf(",%.6f,%.6f,%.6f,%.6f", gps.location.lat(), gps.location.lng(), latLastLog, lngLastLog);
  fileToAppend.printf(",%.0f,%.2f,%.2f", segment, gps.speed.kmph(), gps.altitude.meters());
  fileToAppend.printf(",%u,%i,%u,%u,%u", gps.satellites.value(), gps.hdop.value(), gps.date.year(), gps.date.month(), gps.date.day());
  fileToAppend.printf(",%u,%u,%u,%u,%u,%u\n", gps.time.hour(), gps.time.minute(), gps.time.second(), gps.time.centisecond(),
                      gps.failedChecksum(), gps.passedChecksum());
  fileToAppend.close();
}

// Ecriture d'une ligne dans le trace file. En général le fichier est deja ouvert en mode append
char traceFileName[25] = "";  // en global , car utilisé aussi dans  telechargement.
File traceFile; // en global. Besoin dans le main et pour diwnload
bool fs_ecrireTrace(TinyGPSPlus &gps) {
  char timeBuffer[16];
  String logMessage;
  int longueurEcriture;
  if (!traceFileOpened) {
    if (traceFileName[0] == 0)  // on cree une seule fois le fichier dans un run
      sprintf(traceFileName, "%04u-%02u-%02u_%02u-%02u", gps.date.year(), gps.date.month(), gps.date.day(),
              gps.time.hour(), gps.time.minute());
    if (!LittleFS.exists(traceFileName)) {
      Serial.print(F("Création du fichier trace ")); Serial.println(traceFileName);
      traceFile = LittleFS.open(traceFileName, "a");
      traceFile.close();  // le creer et forcer son enregistrement
    }
    traceFile = LittleFS.open(traceFileName, "a");
    if (!traceFile) {
      Serial.print(F("Error opening the file for appending ")); Serial.println(traceFileName);
      traceFile.close(); // just in case  ;-)
      return removeOldest();
    }
    traceFileOpened = true;
  }
  // structure servant à stocker en binaire une ligne (un point) du log de trace
  trackLigne_t trackLigne;;
  trackLigne.lat = gps.location.lat();
  trackLigne.lng = gps.location.lng();
  trackLigne.hour = gps.time.hour(); // Hour (0-23) (u8)
  trackLigne.minute = gps.time.minute(); // Minute (0-59) (u8)
  trackLigne.second = gps.time.second(); // Second (0-59) (u8)
  trackLigne.centisecond = gps.time.centisecond(); // 100ths of a second (0-99) (u8)
  trackLigne.altitude = gps.altitude.meters();
  trackLigne.speed = gps.speed.kmph();
  longueurEcriture = traceFile.write((uint8_t *)&trackLigne, sizeof(trackLigne));
  if (longueurEcriture != sizeof(trackLigne)) {
    Serial.println(F("Erreur longueur écriture."));
    traceFile.close();
    traceFileOpened = false;
    return removeOldest(); // on essaye de gagner de la place
  }
  return true;
}

// Efface le fichier le plus ancien pour faire de la place
//  (sauf si il y a moins de 4 fichiers ...)
// Return true si on a fait de la place,  false sinon
bool removeOldest() {
  int nbrFiles = 0;
  String oldest = "zzzzz", youngest = "";
  Dir dir = LittleFS.openDir("/");
  int tailleTotale = 0;
  while (dir.next()) {
    String fileName = dir.fileName();
    nbrFiles++;
    if (fileName < oldest ) {
      oldest = fileName;
    }
    if (fileName > youngest ) {
      youngest = fileName;
    }
  }
  if (  nbrFiles > 4) {
    messAlarm = ""; // on va faire de la place dans la mémoire (?) .Enlever message alarme
    fileSystemFull = false;
    return LittleFS.remove(oldest);
  }
  return false ;  // on a rien pu faire: erreur permanente ou moins de 4 fichiers
}

void listSPIFFS(String message) {
  int totalSize = 0;
  int nbrFiles = 0, longest = 0;
  String oldest = "zzzzz", youngest = "";
  int idOldest = 0, idYoungest = 0, idLongest;
  String response = "<script>window.setInterval(function(){window.location.replace('/spiff');},30000);</script>";  //  refresh 30 seconds.
  response += "<div class='card'><form action='/delete_' method='post'><table><tr><th colspan='4'>" + messAlarm + "  " + message + "</th></tr>";
  server.sendContent(response);
  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    //fs: File f = dir.openFile("r");
    File f = dir.openFile("r");
    if (!f) {
      response = "<tr><td>" + fileName + "</td><td>Open error !! </td><td><\td><td><\td>\n";
    }
    else {
      totalSize += f.size();
      nbrFiles++;
      if (fileName.substring(0, 1) == "2" ) {  // ne prendre en compte que les fichier de trace pour oldet/youger
        if (fileName < oldest ) {
          oldest = fileName;
          idOldest = nbrFiles;
        }
        if (fileName > youngest ) {
          youngest = fileName;
          idYoungest = nbrFiles;
        }
        if (f.size() > longest) {
          idLongest = nbrFiles;
          longest = f.size();
        }
      }
      // on génère explicitement un download="kkjhjk.zzz" car sinon cela ne marche pas bien sur certains systèmes
      response = "<tr><td id='" + String(nbrFiles) + "'>" + fileName + "</td><td id='L" + String(nbrFiles) + "'>" + (String)f.size() + "</td>\n";
      response += "<td><button class='b2' type='submit' onclick='return D1()' name='delete' value='" + fileName + "'>Effacer</button></td>\n";
      response += "<td><a href='" + fileName + "' download='" + fileName  +
                  ((fileName.charAt(0) == '2' ) ? ("." + String(preferences.formatTrace)) : "" ) +
                  "'><button class='b1' type='button'>T&eacute;l&eacute;charger </button></a></td></tr>\n";
    }
    server.sendContent(response);
  }
  response = "</table></form>";
  if (nbrFiles != 0) {
    response += "<script>gel('" + String(idYoungest) + "').style.backgroundColor='SpringGreen';"\
                + "gel('" + String(idOldest) + "').style.backgroundColor='Orange';"\
                + "gel('L" + String(idLongest) + "').style.backgroundColor='Orange';</script>";
  }

  FSInfo fs_info;
  if (LittleFS.info(fs_info)) {
    size_t totalBytes;
    size_t usedBytes;
    response += "<br>Nombre de traces: " + String(nbrFiles) + "<br>Taille totale: " + String(totalSize);
    response += "<br>totalBytes: " + String(fs_info.totalBytes) + "<br>usedBytes: " + String(fs_info.usedBytes) ;
  }
  else {
    response += "<br>Erreurs dans le système de fichier ";
  }
  response += "</div></body></html> ";
  server.sendContent(response);
  return ;
}

bool loadFromSPIFFS(String path) {
  // Path est du type server.uri(), donc commence par /
  // Les fichier de trace commence tous par l'année /2xxx et font l'objet d'un traitement particuliers.
  // Un autre fichier peut exister errorlog.txt qui lui sera directement téléchargé.
  // Les autre demande sont a tous les coup des "notFound"
  // Les autres (fichiers log erreur sont simplement directement téléchargés.
  if (path.substring(0, 2) == "/e" ) {
    File dataFile = LittleFS.open(path.c_str(), "r");
    if (!dataFile) return false;
    server.streamFile(dataFile, "text/plain") ;
    dataFile.close();
    return true;
  } else if (path.substring(0, 2) != "/2" ) {
    return false;
  } else {

    // download du fichier "path". On va générer un ficheer .csv ou .gpx suivant les option,
    // a partir du log binaire
    // structure servant à stocker en binaire une ligne (un point) du log de trace
    trackLigne_t trackLigne;
    //   /yyyy-mm-dd
    //   01234678901
    // dateDuFichier va servir à construire le champ date/heure du fichier GPX
    String dateDuFichier = path.substring(1, 11); // on oublit le / du debut de l'uri yyyy-mm-dd
    // Serial.print("Date | ");Serial.print(dateDuFichier);Serial.println(" | ");
    // Si on veut charger le fichier trace en cours, le fermer
    //Serial.printf("; % s; % s; \n"  , traceFileName, dateDuFichier.c_str());
    String ss = path.substring(1);
    if (strcmp(traceFileName, ss.c_str()) == 0) {
      Serial.println("Fermeture fichier trace en cours");
      traceFile.close();
      traceFileOpened = false;
    }

    File dataFile = LittleFS.open(path.c_str(), "r");
    if (!dataFile) return false;
    Serial.print (F("download fichier ")); Serial.println(path);
    server.chunkedResponseModeStart(200, "text/plain");
    // creer le header
    if (strcmp(preferences.formatTrace, "csv") == 0) {
      String logMessage;
      logMessage = "Latitude, Longitude";
      if (preferences.logHeure) logMessage += ", Heure";
      if (preferences.logVitesse) logMessage += ", Vitesse";
      if (preferences.logAltitude) logMessage += ", Altitude";
      logMessage += "\n";
      server.sendContent(logMessage);
    }
    else {
      server.sendContent_P(headerGPX);
    }
    char buf[1024];
    int deb ;
    deb = 0;
    while (dataFile.available()) {
      dataFile.read((uint8_t *)&trackLigne, sizeof(trackLigne));
      if (strcmp(preferences.formatTrace, "csv") == 0) { //generation CSV
        //  -90.00000,-180.00000,12:30:31,1000.00,99999.99
        //  12345678901234567890123456789012345678901234567890
        //  Soit un total de 46 + 2 (cr/lf) + 1 = 49 caractères par point CSV max
        if (sizeof(buf) - deb < 51 ) {
          server.sendContent((const char*)buf, deb);
          deb = 0;
          buf[0] = 0; // chaine vide
        }
        deb += sprintf(&buf[deb], " % .6f, % .6f", trackLigne.lat, trackLigne.lng);   //
        if (preferences.logHeure) deb += sprintf(&buf[deb], ", % 02u: % 02u: % 02u. % 02u", trackLigne.hour, trackLigne.minute, trackLigne.second, trackLigne.centisecond);
        if (preferences.logVitesse) deb += sprintf(&buf[deb], ", % .2f", trackLigne.speed);
        if (preferences.logAltitude) deb += sprintf(&buf[deb], ", % .2f", trackLigne.altitude);
        deb += sprintf(&buf[deb], "\r\n");
      }
      else { // generation GPX
        // <trkpt lat=" - 90.20169" lon=" - 180.67096"><ele>99999.40</ele><time>2002-02-10T16:56:22.00Z</time></trkpt>
        // 123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345
        // 103 + 2(cr/lf) + 1     106 caractères max pour une entrée
        if (sizeof(buf) - deb < 108 ) {
          server.sendContent((const char*)buf, deb);
          deb = 0;
          buf[0] = 0;
        }
        // speed n'existe pas en GPX 1.1: géré par les boutons de sélection du format dans les preferences.
        //    <time> + ou - obligatoire pour OpenStreetMap
        deb += sprintf(&buf[deb], "<trkpt lat=\"%.6f\" lon=\"%.6f\">", trackLigne.lat, trackLigne.lng);
        if (preferences.logAltitude) deb += sprintf(&buf[deb], "<ele>%.2f</ele>", trackLigne.altitude);
        //                         +++++++++++++++++++++   attention ecriture des centiseconde ??
        //<time>2002-02-10T21:01:29.250Z</time> Conforms to ISO 8601 specification for date/time representation.
        if (preferences.logHeure) {
          // On a que l'heure dans le fichier binaire . YYYY/MM/DD sont connus par le nom du fichier 2021-03-03_16-56
          deb += sprintf(&buf[deb], "<time>");
          dateDuFichier.toCharArray(&buf[deb], 11);  // 10 carac + le null
          deb += 10;
          deb += sprintf(&buf[deb], "%T%02u:%02u:%02u.%02uZ</time>",
                         trackLigne.hour, trackLigne.minute, trackLigne.second, trackLigne.centisecond);
        }
        deb += sprintf(&buf[deb], "</trkpt>\r\n");
      }
    }
    // fin fichier. vider le bufer et fermer le GPX
    if (strcmp(preferences.formatTrace, "gpx") == 0) {
      // Bien terminer le fichier GPX
      // </trkseg></trk></gpx>
      // 12345678901234567890123
      //   21 + 1 + 2 (cr/lf) = 24 carca
      if (sizeof(buf) - deb < 26 ) {
        server.sendContent((const char*)buf, deb);
        deb = 0;
        buf[0] = 0;
      }
      deb += sprintf(&buf[deb], "</trkseg></trk></gpx>");
    }
    server.sendContent((const char*)buf, deb);
    server.chunkedResponseFinalize();
    dataFile.close();
    return true;
  }
}

void handleDelete() {
  String result = "Le fichier " + server.arg("delete");
  if (LittleFS.remove(server.arg("delete"))) {
    result += " a &eacute;t&eacute; effac&eacute;";
    fileSystemFull = false;
    messAlarm = ""; // on va faire de la place dans la mémoire (?) .Enlever message alarme
  }
  else
    result += " n'existe pas.";
  Serial.print(F("delete de ")); Serial.println(server.arg("delete"));
  gestionSpiff(result);
}

void handleFormatage() {
  bool formatted = LittleFS.format();
  if (formatted) {
    LittleFS.begin();
    traceFileName[0] = 0; // forcer la recréation éventuelle du trace file
    traceFileOpened = false;
    gestionSpiff("Formatage r&eacute;ussi");
  } else {
    gestionSpiff("Erreur de formatage");
  }
}
void gestionSpiff(String message) {
  sendChunkDebut ( true );  // debut HTML style, avec topMenu
  listSPIFFS(message);
  server.chunkedResponseFinalize();

}
void handleGestionSpiff() {
  gestionSpiff("Liste des traces");
}

void handleNotFound() {
  if (loadFromSPIFFS(server.uri())) return;  // permet de telecharger un fichier par appel direct
  handleCockpit(); //  sinon: acces au portail général
}

// Panic !: on ecrase la conf avec la conf par defaut
void checkFactoryReset() {
  pinMode(pinFactoryReset, INPUT_PULLUP);
  if (digitalRead(pinFactoryReset) == HIGH ) return;
  savePreferences();
}

void readPreferences() {
  EEPROM.get(0, preferences);
  if ( strcmp(factoryPrefs.signature, preferences.signature) != 0) {
    Serial.print(F(" Ecriture configuration usine"));
    preferences = factoryPrefs;
    savePreferences();
  }
}

void savePreferences() {
  EEPROM.put(0, preferences);
  EEPROM.commit();
}

void listPreferences() {
  Serial.print(F("\npassword: ")); Serial.println(preferences.password);
  // Serial.print(F("local_ip: ")); Serial.println(preferences.local_ip);
  // Serial.print(F("gateway: ")); Serial.println(preferences.gateway);
  // Serial.print(F("subnet: ")); Serial.println(preferences.subnet);
  Serial.print(F("logNo: ")); Serial.println(preferences.logNo ? "TRUE" : "FALSE");
  Serial.print(F("logAfter: ")); Serial.println(preferences.logAfter);
  Serial.print(F("formatTrace: ")); Serial.println(preferences.formatTrace);
  Serial.print(F("logVitesse: ")); Serial.println(preferences.logVitesse ? "TRUE" : "FALSE");
  Serial.print(F("logAltitude: ")); Serial.println(preferences.logAltitude ? "TRUE" : "FALSE");
  Serial.print(F("logHeure: ")); Serial.println(preferences.logHeure ? "TRUE" : "FALSE");
  Serial.print(F("baud: ")); Serial.println(preferences.baud);
  Serial.print(F("hz: ")); Serial.println(preferences.hz);

}
void handleOptionLogProcess() {
  Serial.println(F("handleOptionLogProcess"));
  /*
    String   arg (const char *name)  String  arg (int i) String  argName (int i) int   args ()
    bool  hasArg (const char *name)
  */

  preferences.logAfter =  (int)std::strtol(server.arg("logAfter").c_str(), nullptr, 10);;   // n en mètre. Log trace après un déplacemnt de n mètre.  Si <0: log pleine vitesse !!!! pour test
  server.arg("formatTrace").toCharArray(preferences.formatTrace, 4);
  preferences.logNo = server.hasArg("logNo");
  preferences.logVitesse = server.hasArg("logVitesse");
  preferences.logAltitude = server.hasArg("logAltitude");
  preferences.logHeure = server.hasArg("logHeure");
  savePreferences();
  listPreferences();  // ++++++++++++++++++++++++++++++++++++++++++++++++++
  displayOptionsSysteme("Préferences mise à jour.");
}

void handleOptionGPSProcess() {
  bool restartGps = false;
  int bds = (int)std::strtol(server.arg("baud").c_str(), nullptr, 10);
  if (bds != preferences.baud) {
    restartGps = true;
    preferences.baud = bds;
  }
  int hz =  (int)std::strtol(server.arg("hz").c_str(), nullptr, 10);
  // pour 9600bds, limited le rafrachissement à 5hz sinon boom...
  if (bds == 9600 && hz > 5) hz = 5;
  if (hz != preferences.hz ) {
    restartGps = true;
    preferences.hz = hz;
  }
  savePreferences();
  listPreferences();  // ++++++++++++++++++++++++++++++++++++++++++++++++++
  displayOptionsSysteme("Préferences mise à jour.");
  if (restartGps) fs_initGPS();
}

void handleOptionWifiProcess() {
  Serial.println(F("handleOptionWifiProcess"));
  // validation mini des adresses IP
  String message;
  //  IPAddress ipTest;
  // bool errIp = (!ipTest.fromString(server.arg("local_ip"))) || (!ipTest.fromString(server.arg("gateway"))) || (!ipTest.fromString(server.arg("subnet")));
  // if (errIp) {
  //    message = "Erreurs dans les adresses IP";
  //  }
  //  else {
  server.arg("password").toCharArray(preferences.password, 9); //   forcer à 8  ??     init ""  ????????????
  //   server.arg("local_ip").toCharArray(preferences.local_ip, 16); // La taille est limité a 15 par le HTML
  //   server.arg("gateway").toCharArray(preferences.gateway, 16);
  //    server.arg("subnet").toCharArray(preferences.subnet, 16);
  message = "Préferences mise à jour. La balise redémare";
  //}
  savePreferences();
  displayOptionsSysteme(message);
  delay(1500);
  ESP.restart();
}

void handleOptionsPreferences() {
  server.sendContent_P(pageOption);
  String message = "<script>gel('password').value='" + String(preferences.password) + "'";
  //  message += ";gel('local_ip').value='" + String(preferences.local_ip) + "'";
  //  message += ";gel('gateway').value='" + String(preferences.gateway) + "'";
  //  message += ";gel('subnet').value='" + String(preferences.subnet) + "'";
  message += ";gel('logAfter').value=" + String(preferences.logAfter);
  message += ";gel('logNo').checked=" + String((preferences.logNo ? "true" : "false"));
  if (strcmp(preferences.formatTrace, "gpx") == 0) message += ";gel('logGPX').checked=true";
  else message += ";gel('logCSV').checked=true";
  message += ";gel('logVitesse').checked=" + String((preferences.logVitesse ? "true" : "false"));
  message += ";gel('logAltitude').checked=" + String((preferences.logAltitude ? "true" : "false"));
  message += ";gel('logHeure').checked=" + String((preferences.logHeure ? "true" : "false" ));
  message += ";gel('B" + String(preferences.baud) + "').selected='true'";
  message += ";gel('F" + String(preferences.hz) + "').selected ='true'";
  //  document.getElementById("orange").selected = "true";
  message += ";</script>";
  server.sendContent(message);
  server.chunkedResponseFinalize();
}

void handleResetUsine() {
  preferences = factoryPrefs;
  savePreferences();
  handleReset();
}
void handleReset() {
  traceFile.close();
  LittleFS.end();
  sendChunkDebut ( false );  // debut HTML style, sans  topMenu
  server.sendContent_P(byeBey);
  server.chunkedResponseFinalize();
  delay(1500);
  ESP.restart();
}

void fs_initServerOn( ) {
  server.on("/", handleCockpit);
  server.on("/cockpit", handleCockpit);
  server.on("/spiff", handleGestionSpiff);
  server.on("/delete_" , HTTP_POST, handleDelete);
  server.on("/formatage_", HTTP_GET, handleFormatage);  // danger mais un POST est diff a faire !!
  //server.on("/optionsPreferences", handleOptionsPreferences);
  server.on("/optionsSysteme", handleOptionsSysteme);
  server.on("/optionLogProcess", handleOptionLogProcess);
  server.on("/optionWiFiProcess", handleOptionWifiProcess);
  server.on("/optionGPSProcess", handleOptionGPSProcess);
  server.on("/resetUsine", handleResetUsine);
  server.on("/reset", handleReset);
  server.onNotFound(handleNotFound);

#ifdef fs_OTA
  fs_initServerOnOTA(server); // server.on spécicifiques à OTA
#endif
}

void displayOptionsSysteme(String sms) {
  sendChunkDebut ( true );  // debut HTML style, avec topMenu
  server.sendContent_P(menuSysteme);
  server.sendContent("<div class = 'card , gauche'>" + sms + "</div>");
  handleOptionsPreferences();
  server.chunkedResponseFinalize();
}

void handleOptionsSysteme() {
  displayOptionsSysteme("");
}

#ifdef fs_OTA
void handleOTA_() {
  sendChunkDebut ( true );  // debut HTML style, avec topMenu
  server.sendContent_P(pageOTA);
  server.chunkedResponseFinalize();
}
void  fs_initServerOnOTA(ESP8266WebServer &server) {
  server.on("/OTA_", HTTP_GET, handleOTA_);
  server.on("/update", HTTP_POST, [&server]() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "ERREUR !!" : "OK. restart... ");
    delay(1000);
    ESP.restart();
  }, [&server]() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.setDebugOutput(true);
      //WiFiUDP::stopAll(); // ?????????????????????????++++++++++++++++++++  FS
      traceFile.close();
      LittleFS.end();
      Serial.printf("Update avec: %s\n", upload.filename.c_str());
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Serial.println(F("Ongoing ..."));  // FS +++++++++++++++++++++++++++++++++++++++++++++++++
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
      Serial.setDebugOutput(false);
    }
    yield();
  });
}
#endif
