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
#include "fs_main.h"
//#ifdef fs_RECEPTEUR
#include "fs_recepteur.h"
//#endif
#include "fs_pagePROGMEM.h"
#include <LittleFS.h>
#include <EEPROM.h>
#ifdef ESP32
#include <Update.h>
extern fs_WebServer server;
#else
extern ESP8266WebServer server;
#endif
extern boolean traceFileOpened;
extern bool fileSystemFull;
extern String messAlarm;

extern float VmaxSegment, AltMaxSegment;
extern unsigned long TimeShutdown;
extern boolean countdownRunning;
#ifdef fs_STAT
extern statBloc_t  statNotFound , statCockpit ;
#endif
extern TinyGPSPlus gps;

struct pref preferences;
struct pref factoryPrefs;

void sendChunkDebut ( bool avecTopMenu ) {
  server.chunkedResponseModeStart(200, "text/html");
  server.sendContent_P(fs_style);
  if (avecTopMenu)server.sendContent_P(topMenu);
}
void handleCockpit() {
  //  reset du timeout pour arreter le wifi
  TimeShutdown = millis() + preferences.timeoutWifi * 1000;
  countdownRunning = true;
#ifdef fs_STAT
  statCockpit.T0 = millis();
  handleCockpitNotFound();
  calculerStat(true, &statCockpit);
#else
  handleCockpitNotFound();
#endif
}
void handleCockpitNotFound() {
  //Serial.print("handleCockpitNotFound "); Serial.print (server.hostHeader()); Serial.print("  ");Serial.println (server.uri());
  sendChunkDebut ( true );  // debut HTML style, avec topMenu
  server.sendContent_P(cockpit);
  server.sendContent_P(cockpit_fin);
  server.chunkedResponseFinalize();
}
void handleNotFound() {
  // Serial.printf_P(PSTR("------------handleNotFound:%s  milli:%u\n"), server.uri().c_str(), millis());
  if (loadFromSPIFFS(server.uri())) return;  // permet de telecharger un fichier par appel direct
#ifdef fs_STAT
  statNotFound.T0 = millis();
  handleCockpitNotFound(); //  sinon: acces au portail général
  calculerStat(true, &statNotFound);
#else
  handleCockpitNotFound(); //  sinon: acces au portail général
#endif
}

void logRingBuffer(char ringBuffer[], int sizeBuffer, int indexBuffer) {
  // char logFileName[] = "logRingBuf.txt";
  char logFileName[] = "/errorlog.txt";
  File fileToAppend = LittleFS.open(logFileName, "a");
  fileToAppend.println(F("\n====================="));
  fileToAppend.printf_P(PSTR("%u bauds,%uHz\n"), preferences.baud, preferences.hz);
  fileToAppend.println(indexBuffer);
  fileToAppend.write((uint8_t*)ringBuffer[indexBuffer], sizeBuffer - indexBuffer);
  fileToAppend.write((uint8_t*)ringBuffer, indexBuffer);
  fileToAppend.println();
  fileToAppend.close();
  yield();
}

// Problème détecté avec la lecture du GPS. On enregistre des infos dans un log d'erreur.
void logError(TinyGPSPlus &gps, float latLastLog, float lngLastLog , float segment) {
  char logFileName[] = "/errorlog.txt";
  bool isFileExist = LittleFS.exists(logFileName);
  File fileToAppend = LittleFS.open(logFileName, "a");
  //  if (!isFileExist) { //Header
  fileToAppend.println(F("\nlocation.isValid,lat,lng,latLastLog,lngLastLog,segment,speed_km,alt_m,"
                         "sat,hdop,year,month,day,hour,min,sec,centi,failedChecksum,passedChecksum\n"));
  // }
  fileToAppend.printf_P(PSTR("%s"), gps.location.isValid() ? "T" : "F");
  fileToAppend.printf_P(PSTR(",%.6f,%.6f,%.6f,%.6f"), gps.location.lat(), gps.location.lng(), latLastLog, lngLastLog);
  fileToAppend.printf_P(PSTR(",%.0f,%.2f,%.2f"), segment, gps.speed.kmph(), gps.altitude.meters());
  fileToAppend.printf_P(PSTR(",%u,%i,%u,%u,%u"), gps.satellites.value(), gps.hdop.value(), gps.date.year(), gps.date.month(), gps.date.day());
  fileToAppend.printf_P(PSTR(",%u,%u,%u,%u,%u,%u\n"), gps.time.hour(), gps.time.minute(), gps.time.second(), gps.time.centisecond(),
                        gps.failedChecksum(), gps.passedChecksum());
  fileToAppend.close();
}

// Ecriture d'une ligne dans le trace file. En général le fichier est deja ouvert en mode append
// Return:  true   ecriture OK; false sinon
//    (le file system est surement plein, avec un seul fichier
//
char traceFileName[25] = "";  // en global , car utilisé aussi dans  telechargement.
File traceFile; // en global. Besoin dans le main et pour download
bool fs_ecrireTrace(TinyGPSPlus &gps) {

  int longueurEcriture;
  if (!traceFileOpened) {
    if (traceFileName[0] == 0) { // on cree une seule fois le nom fichier dans un run
      sprintf_P(traceFileName, PSTR("/%04u-%02u-%02u_%02u-%02u"), gps.date.year(), gps.date.month(), gps.date.day(),
                gps.time.hour(), gps.time.minute());
      // Serial.printf_P(PSTR("date.isValid: %s location.isValid: %s\n"), gps.date.isValid() ? "yes" : "no", gps.location.isValid() ? "yes" : "no");
    }
    if (!LittleFS.exists(traceFileName)) {
      Serial.print(F("Création du fichier trace ")); Serial.println(traceFileName);
      traceFile = LittleFS.open(traceFileName, "a");
      Serial.printf_P(PSTR(" open append:%s\n"), traceFile ? "OK" : "FAIL");  //+++++++++++++++++++++++++++++++++++++++++
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
  trackLigne.VmaxSegment = VmaxSegment; // vitesse max vue entre ce point et le point précédent en km/h
  trackLigne.AltMaxSegment = AltMaxSegment;
  VmaxSegment = 0.0;
  AltMaxSegment = 0.0;
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
#ifdef ESP32
  File dir = LittleFS.open("/");
#else
  Dir dir = LittleFS.openDir("/");
#endif
  int tailleTotale = 0;

#ifdef ESP32
  File f = dir.openNextFile();
  while (f) {
    //    String fileName = f.name();
#else
  while (dir.next()) {
    File f = dir.openFile("r");
    //    String fileName = String("/") + f.name();
#endif
    String fileName = f.name();
    if (fileName.charAt(0) != '/') fileName = "/" + fileName;
    // Ici le fileName est /xyz...  (ESP32 ou 8266)
    nbrFiles++;
    if (fileName < oldest ) {
      oldest = fileName;
    }
    if (fileName > youngest ) {
      youngest = fileName;
    }
#ifdef ESP32
    f = dir.openNextFile();
#else
    f.close();
#endif
  }
#ifdef ESP32
  dir.close();
#endif
  Serial.printf_P(PSTR("oldest:%s\n"), oldest.c_str());
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
  String response = "<script>window.setInterval(function(){window.location.replace('/spiff');},30000);</script>\n";  //  refresh 30 seconds.
  response += "<div class='card'><form action='/delete_' method='post'><table><tr><th colspan='4'>" + messAlarm + "  " + message + "</th></tr>\n";
  server.sendContent(response);
  // Adaptation ESP8266 / ESP32 pour liste directory vu à partir ligne 185 dans
  //https://github.com/lorol/ESPAsyncWebServer/blob/master/src/SPIFFSEditor.cpp#L185
  traceFile.flush(); // mettre à jour les infos sur fichir et fs
#ifdef ESP32
  File dir = LittleFS.open("/");
#else
  Dir dir = LittleFS.openDir("/");
#endif

#ifdef ESP32
  File f = dir.openNextFile();
  while (f) {
    //  String fileName = f.name();
#else
  while (dir.next()) {
    //    String fileName = dir.fileName();
    File f = dir.openFile("r");
    // String fileName = String("/") + f.name();
#endif
    String fileName = f.name();
    if (fileName.charAt(0) != '/') fileName = "/" + fileName;
    // Ici le fileName est /xyz...  (ESP32 ou 8266)
    if (!f) {
      response = "<tr><td>" + fileName + "</td><td>Open error !! </td><td><\td><td><\td>\n";
    }
    else {
      totalSize += f.size();
      nbrFiles++;
      if (fileName.substring(0, 2) == "/2" ) { // ne prendre en compte que les fichiers de trace pour oldest/younger
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

      // on génère explicitement un download="kkjhjk.zzz" (sans le / de tête) car sinon cela ne marche pas bien sur certains systèmes
      response = "<tr><td><button id='" + String(nbrFiles) + "' class = 'b0' type='submit' name='filename' formaction='/fileInfo_'";
      response += " value='" + fileName + "'>" + fileName + "</button></td>";
      response += "<td id='L" + String(nbrFiles) + "'>" + (String)f.size() + "</td>\n";
      response += "<td><button class='b2' type='submit' onclick='return D1()' name='delete' value='" + fileName + "'>Effacer</button></td>\n";
      response += "<td><a href='" + fileName + "' download='" + fileName.substring(1)  +
                  //     ((fileName.charAt(0) == '2' ) ? ("." + String(preferences.formatTrace)) : "" ) +
                  // Les fichier de trace commencent tous par l'année /2xxx .   (/ rajouté pour ESP32 ...)
                  ((fileName.charAt(1) == '2' ) ? ("." + String(preferences.formatTrace)) : "" ) +  // fichier trace /2021-
                  "'><button class='b1' type='button'>T&eacute;l&eacute;charger </button></a></td></tr>\n";
      response += "</tr>";
    }
    server.sendContent(response);
#ifdef ESP32
    f = dir.openNextFile();
#else
    f.close();
#endif
  }
#ifdef ESP32
  dir.close();
#endif
  response = "</table></form>";
  if (nbrFiles != 0) {
    response += "<script>gel('" + String(idYoungest) + "').style.backgroundColor='SpringGreen';"\
                + "gel('" + String(idOldest) + "').style.backgroundColor='Orange';"\
                + "gel('L" + String(idLongest) + "').style.backgroundColor='Orange';</script>";
  }

#ifdef ESP32
  response += "<br>Nombre de traces: " + String(nbrFiles) + "<br>Taille totale: " + String(totalSize);
  response += "<br>Espace disponible: " + String(LittleFS.totalBytes()) + "<br>Espace utilisé: " + String(LittleFS.usedBytes()) ;
#else
  FSInfo fs_info;
  if (LittleFS.info(fs_info)) {
    size_t totalBytes;
    size_t usedBytes;
    response += "<br>Nombre de traces: " + String(nbrFiles) + "<br>Taille totale: " + String(totalSize);
    response += "<br>Espace disponible: " + String(fs_info.totalBytes) + "<br>Espace utilisé: " + String(fs_info.usedBytes) ;
  }
  else {
    response += "<br>Erreurs dans le système de fichier ";
  }
#endif
  response += "</div></body></html> ";
  server.sendContent(response);
  return ;
}

// Garder uniquement les preferences.nbrMaxTraces dernieres traces
void nettoyageTraces() {
  int nbrFiles = 0;
#ifdef ESP32
  File dir = LittleFS.open("/");
  File f = dir.openNextFile();
  while (f) {
    nbrFiles++;
    f = dir.openNextFile();
  }
  dir.close();
#else
  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    nbrFiles++;
  }
#endif
  for (int i = nbrFiles; i > preferences.nbrMaxTraces; i--) {
    removeOldest();
  }
}

bool loadFromSPIFFS(String path) {
  // Path est du type server.uri(), donc commence par /
  // Les fichiers de trace commencent tous par l'année /2xxx et font l'objet d'un traitement particuliers.
  // Un autre fichier peut exister errorlog.txt qui lui sera directement téléchargé.
  // Les autres demandes sont a tous les coups des "notFound"
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
    if (strcmp(traceFileName, path.c_str()) == 0) {
      Serial.println(F("Fermeture fichier trace en cours"));
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
      if (preferences.logHeure) logMessage += "Heure,";
      logMessage += "Latitude,Longitude";
      if (preferences.logVitesse) logMessage += ",Vitesse,Vitesse_max_segment";
      if (preferences.logAltitude) logMessage += ",Altitude,Altitude_max_segment";
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
        //  12:30:31,-90.00000,-180.00000,1000.00,1000.00,99999.99,99999.99
        //  123456789012345678901234567890123456789012345678901234567890123
        //  Soit un total de 63 + 2 (cr/lf) + 1 = 65 caractères par point CSV max
        if (sizeof(buf) - deb < 70 ) {
          // Serial.printf("Ecriture buffer download %i\n", deb);
          server.sendContent((const char*)buf, deb);
          deb = 0;
          buf[0] = 0; // chaine vide
        }
        if (preferences.logHeure) deb += sprintf_P(&buf[deb], PSTR("%02u:%02u:%02u.%02u,"), trackLigne.hour, trackLigne.minute, trackLigne.second, trackLigne.centisecond);
        deb += sprintf_P(&buf[deb], PSTR("%.6f,%.6f"), trackLigne.lat, trackLigne.lng);
        if (preferences.logVitesse) {
          deb += sprintf_P(&buf[deb], PSTR(",%.2f"), trackLigne.speed);
          deb += sprintf_P(&buf[deb], PSTR(",%.2f"), trackLigne.VmaxSegment);
        }
        if (preferences.logAltitude) {
          deb += sprintf_P(&buf[deb], PSTR(",%.2f"), trackLigne.altitude);
          deb += sprintf_P(&buf[deb], PSTR(",%.2f"), trackLigne.AltMaxSegment);
        }
        deb += sprintf_P(&buf[deb], PSTR("\r\n"));
      }
      else { // generation GPX
        // <trkpt lat=" - 90.20169" lon=" - 180.67096"><ele>99999.40</ele><time>2002-02-10T16:56:22.00Z</time></trkpt>
        // 123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345
        // 103 + 2(cr/lf) + 1     106 caractères max pour une entrée
        if (sizeof(buf) - deb < 120 ) {
         // Serial.printf_P(PSTR("Ecriture buffer download %i\n"), deb);
          server.sendContent((const char*)buf, deb);
          deb = 0;
          buf[0] = 0;
        }
        // speed n'existe pas en GPX 1.1: géré par les boutons de sélection du format dans les preferences.
        //    <time> + ou - obligatoire pour OpenStreetMap
        deb += sprintf_P(&buf[deb], PSTR("<trkpt lat=\"%.6f\" lon=\"%.6f\">"), trackLigne.lat, trackLigne.lng);
        if (preferences.logAltitude) deb += sprintf_P(&buf[deb], PSTR("<ele>%.2f</ele>"), trackLigne.altitude);
        //                         +++++++++++++++++++++   attention ecriture des centiseconde ??
        //<time>2002-02-10T21:01:29.250Z</time> Conforms to ISO 8601 specification for date/time representation.
        if (preferences.logHeure) {
          // On a que l'heure dans le fichier binaire . YYYY/MM/DD sont connus par le nom du fichier 2021-03-03_16-56
          deb += sprintf_P(&buf[deb], PSTR("<time>"));
          dateDuFichier.toCharArray(&buf[deb], 11);  // 10 carac + le null
          deb += 10;
          deb += sprintf_P(&buf[deb], PSTR("%T%02u:%02u:%02u.%02uZ</time>"),
                           trackLigne.hour, trackLigne.minute, trackLigne.second, trackLigne.centisecond);
        }
        deb += sprintf_P(&buf[deb], PSTR("</trkpt>\r\n"));
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
      deb += sprintf_P(&buf[deb], PSTR("</trkseg></trk></gpx>"));
    }
    server.sendContent((const char*)buf, deb);
    server.chunkedResponseFinalize();
    dataFile.close();
    return true;
  }
}

void handleDelete() {
  // Effacement du fichier trace courant ?? : forcer sa recréation etc ...
  if (strcmp(traceFileName, server.arg("delete").c_str()) == 0) {
    Serial.println(F("Fermeture fichier trace en cours"));
    traceFile.close();
    traceFileOpened = false;
    traceFileName[0] = 0;
  }
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
// generation d'une page info sur un fichier.//
// Recherche de vitesse et altitude max sur les segments
void handleFileInfo() {
  char buf[400];
  int deb ;
  deb = 0;
  Serial.print(F("handleFileInfo fichier:")); Serial.println(server.arg("filename"));
  trackLigne_t trackLigne, trackLigneFirst, trackLigneVmax, trackLigneAltMax;
  trackLigneVmax.VmaxSegment = -1.0;
  trackLigneAltMax.AltMaxSegment = -1.0;
  int nbrPts = 0;
  // des pb si on utilise le fichier trace courant ???
  if (strcmp(traceFileName, server.arg("filename").c_str()) == 0) {
    Serial.println(F("Fermeture fichier trace en cours"));
    traceFile.close();
    traceFileOpened = false;
  }
  File dataFile = LittleFS.open(server.arg("filename"), "r");
  if (!dataFile) {
    Serial.print(F("Err open handleFileInfo "));
    Serial.println(server.arg("filename"));
  }
  else {
    while (dataFile.available()) {
      dataFile.read((uint8_t *)&trackLigne, sizeof(trackLigne));
      if (nbrPts == 0) trackLigneFirst = trackLigne;
      if (trackLigne.VmaxSegment > trackLigneVmax.VmaxSegment)
        trackLigneVmax = trackLigne;
      if (trackLigne.AltMaxSegment > trackLigneAltMax.AltMaxSegment )
        trackLigneAltMax = trackLigne;
      nbrPts++;
    }
    dataFile.close();
  }
  uint32_t Tdeb = trackLigneFirst.hour * 360000 + trackLigneFirst.minute * 6000 + trackLigneFirst.second * 100 + trackLigneFirst.centisecond;
  uint32_t Tfin = trackLigne.hour * 360000 + trackLigne.minute * 6000 + trackLigne.second * 100 + trackLigne.centisecond;
  uint32_t duree = Tfin - Tdeb;
  Serial.printf("%u %u %u\n", Tdeb, Tfin, duree);
  unsigned int hh, mm, ss, cc;
  hh = duree / 360000;
  mm = (duree - hh * 360000) / 6000;
  ss = (duree - hh * 360000 - mm * 6000) / 100;
  cc = (duree - hh * 360000 - mm * 6000 - ss * 100);
  Serial.printf("%u:%u:%u.%u\n", hh, mm, ss, cc);

  server.chunkedResponseModeStart(200, "text/html");
  server.sendContent_P(fs_style);
  deb += sprintf_P(&buf[deb], PSTR("<body><div id='d1' class='card gauche' >Fichier %s<br>Nombre de points %u"),
                   server.arg("filename").c_str(), nbrPts);
  deb += sprintf_P(&buf[deb], PSTR("<br>Début de trace à %02d:%02d:%02d.%02d"), trackLigneFirst.hour, trackLigneFirst.minute,
                   trackLigneFirst.second, trackLigneFirst.centisecond);
  deb += sprintf_P(&buf[deb], PSTR("<br>Fin de trace à %02d:%02d:%02d.%02d"), trackLigne.hour, trackLigne.minute,
                   trackLigne.second, trackLigne.centisecond);
  deb += sprintf_P(&buf[deb], PSTR("<br>Durée %02d h %02d min %02d.%02d s"), hh, mm, ss, cc);
  deb += sprintf_P(&buf[deb], PSTR("<br>Vmax % .2f km/h à %02d:%02d:%02d.%02d"), trackLigneVmax.VmaxSegment, trackLigneVmax.hour, trackLigneVmax.minute,
                   trackLigneVmax.second, trackLigneVmax.centisecond);
  deb += sprintf_P(&buf[deb], PSTR("<br>Hmax % .2f m à %02d:%02d:%02d.%02d"), trackLigneAltMax.AltMaxSegment, trackLigneAltMax.hour, trackLigneAltMax.minute,
                   trackLigneAltMax.second, trackLigneAltMax.centisecond);
  deb += sprintf_P(&buf[deb], PSTR("</div></body></html>"));
  server.sendContent( buf );
  server.chunkedResponseFinalize();
  // Serial.println(deb);
}
void handleFormatage() {
  traceFile.close();
  bool formatted = LittleFS.format();
  if (formatted) {
    Serial.print(F("Format: OK. LittleFS.begin: "));
    LittleFS.end();
#ifdef ESP32
    Serial.println(LittleFS.begin(true) ? F("Ready") : F("Failed!"));  // true = format if fail
#else
    Serial.println(LittleFS.begin() ? F("Ready") : F("Failed!"));  // pour ESP8266 format if fail par defaut
#endif
    traceFileName[0] = 0; // forcer la recréation éventuelle du trace file
    traceFileOpened = false;
    fileSystemFull = false;
    messAlarm = "";
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
  countdownRunning = false;
  gestionSpiff("Liste des traces (téléchargement " + String(preferences.formatTrace) + ")");
}

// Panic !: on ecrase la conf avec la conf par defaut
void checkFactoryReset() {
#ifdef pinFactoryReset
  pinMode(pinFactoryReset, INPUT_PULLUP);
  if (digitalRead(pinFactoryReset) == HIGH ) return;
  savePreferences();
  // attendre un vrai rédémmarage. Pour eviter un court circuit si pinFactoryReset est aussi utilisée plus tard comme sortie
  // (cas de la pin 2 sur ESP01/8266 ....)
  while (true) {
    delay(1000);
  };
#endif
}

void readPreferences() {
  EEPROM.get(0, preferences);
  if ( strcmp(factoryPrefs.signature, preferences.signature) != 0) {
    Serial.print(F("  EEPROM corrompue ? Initialisation configuration 'usine'"));
    preferences = factoryPrefs;
    savePreferences();
  }
}

void savePreferences() {

  dbgHeap("debut");
  EEPROM.put(0, preferences);
  EEPROM.commit();
  dbgHeap("fin");
}

void listPreferences() {
  Serial.print(F("password : ")); Serial.println(preferences.password);
  Serial.print(F("ssid_AP : ")); Serial.println(preferences.ssid_AP);
  Serial.print(F("logOn : ")); Serial.println(preferences.logOn ? "TRUE" : "FALSE");
  Serial.print(F("logToujours : ")); Serial.println(preferences.logToujours ? "TRUE" : "FALSE");
  Serial.print(F("logAfter : ")); Serial.println(preferences.logAfter);
  Serial.print(F("formatTrace : ")); Serial.println(preferences.formatTrace);
  Serial.print(F("logVitesse : ")); Serial.println(preferences.logVitesse ? "TRUE" : "FALSE");
  Serial.print(F("logAltitude : ")); Serial.println(preferences.logAltitude ? "TRUE" : "FALSE");
  Serial.print(F("logHeure : ")); Serial.println(preferences.logHeure ? "TRUE" : "FALSE");
  Serial.print(F("nbrMaxTraces : ")); Serial.println(preferences.nbrMaxTraces);
  Serial.print(F("baud : ")); Serial.println(preferences.baud);
  Serial.print(F("hz : ")); Serial.println(preferences.hz);
  Serial.print(F("arretWifi : ")); Serial.println(preferences.arretWifi ? "TRUE" : "FALSE");
  Serial.print(F("timeoutWifi : ")); Serial.println(preferences.timeoutWifi);
  Serial.print(F("basseConso : ")); Serial.println(preferences.basseConso ? "TRUE" : "FALSE");
}
void handleOptionLogProcess() {
  //Serial.println(F("handleOptionLogProcess"));
  /*
    String   arg (const char *name)  String  arg (int i) String  argName (int i) int   args ()
    bool  hasArg (const char *name)
  */

  preferences.logAfter =  (int)std::strtol(server.arg("logAfter").c_str(), nullptr, 10);
  //Si "unit" est m: log d'un point trace après un déplacement de n mètre.
  //Si "unit" est ms: log d'un point trace chaque n millis seconde. La veleur de n est stocké en négatif ( historique ...!)
  //    100 ms minimum (10hz ...)

  if (server.arg("unit") == "ms")
  {
    if (preferences.logAfter < 100) preferences.logAfter = -100;
    else  preferences.logAfter = -preferences.logAfter;
  }
  server.arg("formatTrace").toCharArray(preferences.formatTrace, 4);
  preferences.logOn = server.arg("logOn") == "Oui";
  preferences.logToujours = server.arg("logToujours") == "Oui";
  preferences.logVitesse = server.hasArg("logVitesse");
  preferences.logAltitude = server.hasArg("logAltitude");
  preferences.logHeure = server.hasArg("logHeure");
  preferences.nbrMaxTraces = (int)std::strtol(server.arg("nbrMaxTraces").c_str(), nullptr, 10);
  savePreferences();
  listPreferences();  // ++++++++++++++++++++++++++++++++++++++++++++++++++
  displayOptionsSysteme("Préferences mises à jour.");
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
  displayOptionsSysteme("Préferences mises à jour.");
  if (restartGps)  fs_initGPS(preferences.baud, preferences.hz);
}

void handleOptionPointAccesProcess() {
  preferences.arretWifi = (server.arg("arretWifi") == "Oui");
  if (server.hasArg("timeoutWifi"))
    preferences.timeoutWifi = (int)std::strtol(server.arg("timeoutWifi").c_str(), nullptr, 10);
  preferences.basseConso = (server.arg("conso") == "Oui");
  server.arg("password").toCharArray(preferences.password, 9); //   forcer à 8  ??     init ""  ????????????
  server.arg("ssid_AP").toCharArray(preferences.ssid_AP, 33); //    init ""
  savePreferences();
  listPreferences();  // ++++++++++++++++++++++++++++++++++++++++++++++++++
  displayOptionsSysteme("Préferences mises à jour.");
}

void handleOptionsPreferences() {
  server.sendContent_P(pageOption);
  String message = "<script>gel('password').value = '" + String(preferences.password) + "'";
  message += "; gel('ssid_AP').value = '" + String(preferences.ssid_AP) + "'";
  //  message += "; gel('local_ip').value = '" + String(preferences.local_ip) + "'";
  //  message += "; gel('gateway').value = '" + String(preferences.gateway) + "'";
  //  message += "; gel('subnet').value = '" + String(preferences.subnet) + "'";
  message += "; gel('logAfter').value = " + String(abs(preferences.logAfter));
  if (preferences.logAfter <= 0)
    message += "; gel('millis').selected = 'true'";
  else
    message += "; gel('metre').selected = 'true'";
  message += "; gel('" + String((preferences.logOn ? "ouiLog" : "nonLog"))  + "').checked = true";
  message += "; gel('" + String((preferences.logToujours ? "ouiToujours" : "nonToujours"))  + "').checked = true";
  if (strcmp(preferences.formatTrace, "gpx") == 0) message += "; gel('logGPX').checked = true";
  else message += "; gel('logCSV').checked = true";
  message += "; gel('logVitesse').checked = " + String((preferences.logVitesse ? "true" : "false"));
  message += "; gel('logAltitude').checked = " + String((preferences.logAltitude ? "true" : "false"));
  message += "; gel('logHeure').checked = " + String((preferences.logHeure ? "true" : "false" ));
  message += "; gel('nbrMaxTraces').value = " + String(preferences.nbrMaxTraces);
  message += "; gel('B" + String(preferences.baud) + "').selected = 'true'";
  message += "; gel('F" + String(preferences.hz) + "').selected = 'true'";
  message += "; gel('" + String((preferences.arretWifi ? "ouiWifi" : "nonWifi"))  + "').checked = true";
  message += "; gel('timeoutWifi').value = " + String(preferences.timeoutWifi);
  message += "; gel('" + String((preferences.basseConso ? "ouiConso" : "nonConso"))  + "').checked = true";
  message += "; </script ></body></html> ";
  server.sendContent(message);
  server.chunkedResponseFinalize();
}
void handleGiveMeTime() {
  // repousser le timeout arret wifide 2mn
  Serial.println(F("Entrée handleGiveMeTime"));
  TimeShutdown += 2 * 60 * 1000;
  server.send(204); // OK, no content
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
  server.sendContent_P(byeBye);
  server.chunkedResponseFinalize();
  delay(1500);
  ESP.restart();
}

void handleFavicon() {
  Serial.print(F("Favicon ")); Serial.println(sizeof(favicon));
  server.send_P(200, "image/x-icon", favicon, sizeof(favicon));
  //  server.send_P(200, "image/x-icon", favicon);
}
// pour captive portal Android. Voir https://gitlab.com/defcronyke/wifi-captive-portal-esp-idf
void handle_generate_204() {
  Serial.print(F("generate_204 ")); Serial.print (server.hostHeader()); Serial.print("  ");
  Serial.println (server.uri());
  // on fait une redirection. Cela change url dans la barre adresse du navigateur. Plus joli !!
  server.sendHeader("Location", String("http://majoliebalise.fr"), true);
  server.send ( 302, "text/plain", "Found");
}
void fs_initServerOn( ) {
  // Pour portail captif. Voir infopour Android  https://gitlab.com/defcronyke/wifi-captive-portal-esp-idf
  server.on("/", handleCockpitNotFound);
  server.on("/favicon.ico", handleFavicon);
  server.on("/redirect",  handle_generate_204); // Pour windows  www.msftconnecttest.com//redirect
  server.on("/generate_204", handle_generate_204);  // pour Android connectivitycheck.gstatic.com/generate_204
  server.onNotFound(handleNotFound);
  //
  server.on("/cockpit", handleCockpit);
  server.on("/readValues", handleReadValues);
  server.on("/giveMeTime", handleGiveMeTime);
  server.on("/razVMaxHMAx", handleRazVMaxHMAx);
  server.on("/spiff", handleGestionSpiff);
  server.on("/delete_" , HTTP_POST, handleDelete);
  server.on("/fileInfo_" , HTTP_POST, handleFileInfo);
  server.on("/formatage_", HTTP_GET, handleFormatage);  // danger mais un POST est diff a faire !!
  server.on("/optionsSysteme", handleOptionsSysteme);
  server.on("/optionLogProcess", handleOptionLogProcess);
  server.on("/optionGPSProcess", handleOptionGPSProcess);
  server.on("/optionPointAccesProcess", handleOptionPointAccesProcess);
  server.on("/resetUsine", handleResetUsine);
  server.on("/reset", handleReset);
#ifdef fs_STAT
  server.on("/stat", handleStat);
  server.on("/readStatistics", handleReadStatistics);
  server.on("/statReset", handleResetStatistics);
#endif
#ifdef fs_RECEPTEUR
  server.on("/recepteur", handleRecepteur);
  server.on("/recepteurRefresh", handleRecepteurRefresh);
  server.on("/recepteurDetail", handleRecepteurDetail);
#endif

#ifdef fs_OTA
  fs_initServerOnOTA(server); // server.on spécicifiques à OTA
#endif
}

void displayOptionsSysteme(String sms) {
  sendChunkDebut ( true );  // debut HTML style, avec topMenu
  server.sendContent_P(menuSysteme);
  server.sendContent("<div class='card gauche'>" + sms + "</div>");
  handleOptionsPreferences();
  server.chunkedResponseFinalize();
}

void handleOptionsSysteme() {
  countdownRunning = false;
  displayOptionsSysteme("");
}

#ifdef fs_OTA
void handleOTA_() {
  sendChunkDebut ( true );  // debut HTML style, avec topMenu
  server.sendContent_P(pageOTA);
  server.chunkedResponseFinalize();
}
#ifdef ESP32
void  fs_initServerOnOTA(fs_WebServer &server) {
#else
void  fs_initServerOnOTA(ESP8266WebServer &server) {
#endif
  server.on("/OTA_", HTTP_GET, handleOTA_);
  server.on("/update", HTTP_POST, [&server]() {
    server.sendHeader("Connection", "close");
    //  server.send(200, "text/plain", ((Update.hasError()) ? "ERREUR !! " : "OK.")+"La balise redémarre. Bye Bye !!<br> (je reviens peut être dans 10s ...)";
    server.send(200, "text/plain", String(Update.hasError() ? "ERREUR !! " : "OK. ") +
                "La balise redémarre. Bye Bye !!<br> (je reviens peut être dans 10s ...)");
    delay(1000);
    ESP.restart();
  }, [&server]() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.setDebugOutput(true);
      //WiFiUDP::stopAll(); // ?????????????????????????++++++++++++++++++++  FS
      traceFile.close();
      LittleFS.end();
      Serial.printf_P(PSTR("Update avec: % s\n"), upload.filename.c_str());
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
        Serial.printf_P(PSTR("Update Success: % u\nRebooting...\n"), upload.totalSize);
      } else {
        Update.printError(Serial);
      }
      Serial.setDebugOutput(false);
    }
    yield();
  });
}
#endif
// Calcul pour statistiques min/max etc ...
// Tout est passer par référence !!  sauf T0 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void calculerStat (boolean calculerTemps, statBloc_t * bl) {
  // calculerTemps  true:  bl.T0 représente le temps début de la periode de mesure
  //       (utile pour stat de timming)
  // calculerTemps  false:  bl.T0 represente la mesure elle même, dont on va calculer min/max etc ..
  //       (utile pour stat sur longeur de segment)
  long T1;
  int echantillon;
  if (bl->T0 == 0 && calculerTemps) return;  // ignorer le premier car T0 est parfois mal initialisé
  T1 = millis();
  if (calculerTemps) echantillon = T1 - bl->T0;
  else echantillon = bl->T0;
  bl->total += echantillon; bl->min = min(bl->min, echantillon); bl->max = max(bl->max, echantillon);
  bl->sousTotal += echantillon;
  bl->count++;
  if (bl->count % 20 == 0) {
    bl->moyenneLocale = float(bl->sousTotal) / 20.0;
    bl->sousTotal = 0;
  }
}
void razStatBloc(statBloc_t* bl) {
  bl->min = 99999999;
  bl->max = 0;
  bl->total = 0;
  bl->count = 0;
  bl->sousTotal = 0;
  bl->T0 = 0;
  bl->moyenneLocale = 0.0;
}
int writeStatBloc(char buf[], int size, statBloc_t* bl) {
  int s = 0;
  if ( size <= 0 ) Serial.println(F("writeStatBloc. buffer trop petit. "));
  else
    //génére une chaine du genre     statxy$min$max$moyenne1$moyenne2;
    //  Pour affichage dans la page HTML de statistiques
    s = snprintf_P(buf, size, PSTR(" % s$ % u$ % u$ % .1f$ % .1f; "), bl->nom, bl->min, bl->max, float(bl->total) / bl->count, bl->moyenneLocale);
  if (s >= size ) Serial.println(F("writeStatBloc. buffer trop petit. "));
  return s;
}
