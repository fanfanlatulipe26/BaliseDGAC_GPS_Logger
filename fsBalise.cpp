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
#include "fs_options.h"
#include <LittleFS.h>
#include <EEPROM.h>
extern ESP8266WebServer server;
struct pref preferences;
struct pref factoryPrefs;
//  ATTENTION: il faut conditionner la compilation du code, sinonle linker
//            va tout prendre, même si on veut une version mini sans OTA...



#ifdef fs_OTA
char pageOTA[] PROGMEM = R"=====(
<form class = "card" method='POST' id='upload_form' enctype='multipart/form-data'>
<input type='file' name='update' >
<input type='button' onclick = 'uploadFile()' style= 'background-color: red;' value='Update'>
</form>
<div>
<progress id="bar" class = "card" value="0" max="100"></progress>
</div>
<div id="res" class="card"></div>
<script>
function uploadFile() {
gel('res').innerHTML = "Attendre un message ERREUR ou OK !!";
var form = gel('upload_form');
var data = new FormData(form);
var xhr = new window.XMLHttpRequest();
xhr.upload.addEventListener('progress', h_pro, false);
xhr.upload.addEventListener('loadstart', h_loadstart, false);
xhr.upload.addEventListener('abort', h_abort, false);
xhr.upload.addEventListener('error', h_error, false);
xhr.upload.addEventListener('load', h_load, false);
xhr.upload.addEventListener('timeout', h_timeout, false);
xhr.upload.addEventListener('loadend', h_pro, false);
xhr.open('POST','/update');
xhr.send(data);
xhr.onreadystatechange = function () {
gel('bar').value = 0;
 if(xhr.readyState == 4 ) gel('res').innerHTML = xhr.responseText;
 };
}
function h_loadstart (evt) { console.log("h_loadstart");}
function h_abort (evt) { alert("abort");}
function h_error (evt) { alert("h_error");gel('res').innerHTML = "ERREUR !!";}
function h_load (evt) { console.log("load");}
function h_timeout (evt) { alert("h_timeout");}
function h_loadend (evt) { alert("h_loadend");}
 function h_pro (evt) {
 console.log ("pro");
  if (evt.lengthComputable) {
    var per = evt.loaded / evt.total;
    gel('bar').value = Math.round(per * 100) ;
  }
}
 </script>
)=====";
#endif 

char fs_style[] PROGMEM = R"=====(
<!doctype html>
<head>
<meta charset="utf-8">
<title>Balise</title>
<META HTTP-EQUIV="Pragma" CONTENT="no-cache">
<META HTTP-EQUIV="Expires" CONTENT="-1">
<style>
#map{height:1000px;}
.card{
padding:5px;
width:100%;
background: #1f98bd81;
border-radius: 10px;
color: #000;
margin:0px;
font-size: 4vw;
text-align: center;
   /* line-height: 0%; */
}
.gauche{
text-align: left;
}
.b1{
background-color: LimeGreen;
}  
.b2{
background-color: red; 
}

input,button{
font-size: 4vw;
border: 5px solid  black ;
border-radius: 10px;
}
table {
margin: auto;
}

table,td,th{
border: 4px solid lime;
border-collapse: collapse;
font-size: 4vw;
}
.nobord{
border: 0px;
}
input[type=checkbox] + label {
} 
input[type=checkbox]:checked + label {
  background-color: #4CAF50;
} 
</style>
<script>
function D1() {return confirm("Effacer ce fichier ?");}
function gel(id) {return document.getElementById(id);}
</script>
</head>

)=====";

char topMenu[] PROGMEM = R"=====(
<body>
<div class = "card">
<table class = 'nobord'>
  <tr>
    <td class = 'nobord'><button class='b1' onclick="document.location='/'"> Cockpit</button></td>
    <td class = 'nobord'><button class='b1' onclick="document.location='/spiff'"> Traces </button></td>
    <td class = 'nobord'><button class='b1' onclick="document.location='/optionsSysteme'"> Préférences / Système</button></td>
  </tr>
</table>
</div>
)=====";

char menuSysteme[] PROGMEM = R"=====(
  <div class = 'card'>
  <button class='b1' onclick="document.location='/OTA_'">MaJ OTA</button>

    <button class='b1' onclick="document.location='/resetUsine'">Reset Usine </button>
     <button class='b1' onclick="document.location='/reset'">Reset </button>
  <button class='b2' onclick = 'if(confirm("Voulez vous vraiment reformater le syst&egrave;me de fichiers\n et tout effacer ?")){document.location="/formatage_"}'>Formatage</button>
  </div>

)=====";

char pageOption[] PROGMEM = R"=====(
<div id="d1" class = "card , gauche" >
<form action="/optionLogProcess">
  <b>Préférences gestion de la trace</b><br>
 <input type="checkbox" id="logOn" name="logOn"  checked>
<label for="logOn">Activer la trace</label><br>  
  &nbsp; &nbsp; <label for="logAfter">    après un déplacement de </label> 
 <input type="text" id="logAfter" name="logAfter" pattern="[0-9]{1,}" size="1" required value="x" > mètre(s).<br>   
 <input type="checkbox" id="logVitesse" name="logVitesse" > 
  <label for="logVitesse">Enregistrer la vitesse</label><br>  
 <input type="checkbox" id="logAltitude" name="logAltitude" >
    <label for="logAltitude">Enregistrer  l'altitude</label><br> 
 <input type="checkbox" id="logHeure" name="logHeure" >
    <label for="logHeure">Enregistrer  l'heure</label> 
    <br><input type="submit" value="Submit">
</form>
  <hr>  <b>Préférences WiFi (attention !!)</b> 
 <form action="/optionWiFiProcess">
 <label for="password">Mot de passe</label>
 <input type="text" id="password" name="password" size ="8" minlength="8"  maxlength="8" value=""><br>
 <label for="local_ip">Adresse IP</label>
 <input type="text" id="local_ip" name="local_ip" size="14" maxlength="15" value="x.x.x.x"><br>
 <label for="gateway">Gateway</label> 
 <input type="text" id="gateway" name="gateway" size="14" maxlength="15" value="x.x.x.x"><br> 
 <label for="subnet">Sous réseau</label> 
 <input type="text" id="subnet" name="subnet" size="14" maxlength="15" value="x.x.x.x">  
<br><input type="submit" value="Submit">
</form>
</div>
  </body>
</html>
)=====";


char byeBey[] PROGMEM = R"=====(
<div id="d1" class = "card , gauche" >
La balise redémarre.   Bye Bye !!
</div>
  </body>
</html>
)=====";
char cockpit[] PROGMEM = R"=====(
<script>
setInterval(function() {
getData();
}, 3000); //3000mSeconds update rate
function getData() {
var xhttp = new XMLHttpRequest();
xhttp.onreadystatechange = function() {
// chaine reçue   numero$valeur;numero$valeur ...
if (this.readyState == 4 && this.status == 200) {
this.responseText.split(';').forEach((item)=>{
      var i = item.split('$')[0].trim();   // ??????????????????????????????????????
    var val = item.split('$')[1].replace(/"/g,'').trim();
  document.getElementById("Value"+i).innerHTML = val;
});
}
};
xhttp.open("GET", "readValues", true);
xhttp.send();
}
getData();
</script>
<div class="card">
<p><span id="Value0">0</span><p>
<p><span id="Value7">0</span><p>
<table border="1" style="line-height: 100%;">
<tr>
<td><span id="Value4">0</span></td>
<td><span id="Value5">0</span></td>
<td><span id="Value6">0</span></td>
</tr>
<tr>
<td><span id="Value8">0</span></td>
<td><span id="Value2">0</span></td>
<td><span id="Value3">0</span></td>
</tr>
<tr>
<td><span id="Value1">0</span></td>
<td><span id="Value9"></span></td>
<td><span id="Value13"></span></td>
</tr>
<tr>
<td><span id="Value10">ATTENTE</span></td>
<td><span id="Value11"></span></td>
<td><span id="Value12"></span></td>
</tr>
</table>
</div>
</body>
</html>
)=====";

void sendChunkDebut ( bool avecTopMenu ) {
  server.chunkedResponseModeStart(200, "text/html");
  server.sendContent_P(fs_style);
  if (avecTopMenu)server.sendContent_P(topMenu);
}
void handleCockpit_() {
  sendChunkDebut ( true );  // debut HTML style, avec topMenu
  server.sendContent_P(cockpit);
  server.chunkedResponseFinalize();
}



// Ecriture d'une ligne csv dnas le fichier de log
// Si le file system est plein, on efface les fichiers les plus anciens.
// Dans ce cas on prda une ligne de log
// Return:  true   ecriture OK; false sinon
//    (le file system est surement plein, avec un seul fichier
//
char logFileName[25] = "";  // en global , car utilisé aussi dans options.
bool fs_logEcrire(TinyGPSPlus &gps) {
  // return; // +++++++++++++++++++++++++++++++++++++++++++++ debug
  char timeBuffer[16];
  String logMessage;

  if (logFileName[0] == 0)  // on cree une seule fois le fichier dans un run
    sprintf(logFileName, "/%04u-%02u-%02u_%02u-%02u.csv", gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute() );
  bool isFileExist = LittleFS.exists(logFileName);
  File fileToAppend = LittleFS.open(logFileName, "a");
  if (!fileToAppend) {
    Serial.print(F("Error opening the file for appending ")); Serial.println(logFileName);
    return removeOldest();
  }
  if (!isFileExist) { //Header
    logMessage = "Latitude,Longitude";
    if (preferences.logHeure) logMessage += ",Heure";
    if (preferences.logVitesse) logMessage += ",Vitesse";
    if (preferences.logAltitude) logMessage += ",Altitude";
    int longeurEcrite =  fileToAppend.println(logMessage);
    Serial.print(F(" Création fichier ")); Serial.println(logFileName);
    if (longeurEcrite == 0) {
      Serial.println(F("Erreur écriture"));
      return removeOldest(); // on essaye de gagner de la place
    }
  }

  sprintf(timeBuffer, "%02u:%02u:%02u", gps.time.hour(), gps.time.minute(), gps.time.second());
  logMessage = String(gps.location.lat()) + "," + gps.location.lng();
  if (preferences.logHeure) logMessage += "," + String(timeBuffer);
  if (preferences.logVitesse) logMessage += "," + String(gps.speed.kmph());
  if (preferences.logAltitude) logMessage += "," + String(gps.altitude.meters());
  int longeurEcrite = fileToAppend.println(logMessage);
  if (longeurEcrite == 0) {
    Serial.println(F("Erreur écriture"));
    return removeOldest(); // on essaye de gagner de la place
  }
  fileToAppend.close();
  return true;
}

// Efface le fichier le plus ancien pour faire de la place
//  (sauf si il y a un seul fichier ....)
// Return true sion a fait de la place,  false sinon
bool removeOldest() {
  Serial.println(F("Entrée remove oldest"));
  String oldest = "zzzzz", youngest = "";
  Dir dir = LittleFS.openDir("/");
  int tailleTotale = 0;
  while (dir.next()) {
    String fileName = dir.fileName();
    Serial.println(fileName);
    if (fileName < oldest ) {
      oldest = fileName;
    }
    if (fileName > youngest ) {
      youngest = fileName;
    }
  }
  Serial.print(F("!")); Serial.print(oldest); Serial.print(F("!")); Serial.println(youngest);
  if (oldest != "" && oldest != youngest) {
    Serial.print(F("Cleanup. On efface: ")); Serial.println(oldest);
    if (LittleFS.remove(oldest)) {
      Serial.print(F(" On a efface: ")); Serial.println(oldest);
      return true; // on a pu gagner de la place
    }  
    return false;  // on a rien pu faire: erreur permanente
  }
}

void listSPIFFS(String message) {
  int totalSize = 0;
  int nbrFiles = 0, longest = 0;
  String oldest = "zzzzz", youngest = "";
  int idOldest = 0, idYoungest = 0, idLongest;
  String response = "<div class = 'card'><form action='/delete_'  method='post'> <table><tr><th colspan='4'>" + message + "</th></tr>";
  server.sendContent(response);
  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();

    //fs: File f = dir.openFile("r");
    File f = dir.openFile("r");
    if (!f) {
      response = "<tr><td>" + fileName + "</td><td> Open error !!</td><td><\td><td><\td>\n";
    }
    else {
      totalSize += f.size();
      nbrFiles++;
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
      response = "<tr><td id='" + String(nbrFiles) + "'>" + fileName + "</td><td id='L" + String(nbrFiles) + "'>" + (String)f.size() + "</td>\n";
      response += "<td><button class='b2' type='submit' onclick='return D1()' name ='delete' value='" + fileName + "'>Effacer</button></td>\n";
      response += "<td><a href='" + fileName + "' download='"+ fileName + "'><button class='b1' type='button'  >T&eacute;l&eacute;charger</button></a></td></tr>\n";
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
    response += "<br>Nombre de traces: " + String(nbrFiles) + "<br>Taille totale:" + String(totalSize);
    response += "<br>totalBytes: " + String(fs_info.totalBytes) + "<br>usedBytes: " + String(fs_info.usedBytes) ;
  }
  else {
    Serial.println(F("Erreur LittleFS.info"));
    response += "<br>Erreurs dans le système de fichier ";
  }
  response += "</div></body></html>";
  server.sendContent(response);
  return ;
}

bool loadFromSPIFFS(String path) {
  String dataType = "text/plain";
  File dataFile = LittleFS.open(path.c_str(), "r");
  if (!dataFile) return false;
  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
    Serial.println(F("Sent less data than expected"));
  }
  dataFile.close();
  return true;
}
void handleDelete() {
  String result = "Le fichier " + server.arg("delete");
  if (LittleFS.remove(server.arg("delete")))
    result += " a &eacute;t&eacute; effac&eacute;";
  else
    result += " n'existe pas.";
  Serial.print(F("delete de ")); Serial.println(server.arg("delete"));
  gestionSpiff(result);
}

void handleFormatage() {
  Serial.print(F("Formatage:"));
  bool formatted = LittleFS.format();
  if (formatted) {
    Serial.println(F("OK"));
    LittleFS.begin();
    gestionSpiff("Formatage r&eacute;ussi");
  } else {
    Serial.println(F("Erreur"));
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
  handleCockpit_(); //  sinon: acces au portail général
}

// Panic !: on ecrase la conf avec la conf par defaut
void checkFactoryReset() {
  Serial.println(F("Entrée checkFactoryReset"));
  pinMode(pinFactoryReset, INPUT_PULLUP);
  if (digitalRead(pinFactoryReset) == HIGH ) return;
  Serial.println(F("Factory reset !!"));
  savePreferences();

}

void readPreferences() {
  EEPROM.get(0, preferences);
  Serial.print("factoryPrefs.signature: "); Serial.println(factoryPrefs.signature);
  Serial.print("preferences.signature: "); Serial.println(preferences.signature);
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
  Serial.print(F("local_ip: ")); Serial.println(preferences.local_ip);
  Serial.print(F("gateway: ")); Serial.println(preferences.gateway);
  Serial.print(F("subnet: ")); Serial.println(preferences.subnet);
  Serial.print(F("logAfter: ")); Serial.println(preferences.logAfter);
  Serial.print(F("logOn: ")); Serial.println(preferences.logOn ? "TRUE" : "FALSE");
  Serial.print(F("logVitesse: ")); Serial.println(preferences.logVitesse ? "TRUE" : "FALSE");
  Serial.print(F("logAltitude: ")); Serial.println(preferences.logAltitude ? "TRUE" : "FALSE");
  Serial.print(F("logHeure: ")); Serial.println(preferences.logHeure ? "TRUE" : "FALSE");
}
void handleOptionLogProcess() {
  Serial.println(F("handleOptionLogProcess"));
  /*
    String   arg (const char *name)  String  arg (int i) String  argName (int i) int   args ()
    bool  hasArg (const char *name)
  */
  preferences.logAfter =  (int)std::strtol(server.arg("logAfter").c_str(), nullptr, 10);;   // en mètre. 0= debut du log dès le fix, sinon après déplacement initial de n mètre
  preferences.logOn = server.hasArg("logOn");
  preferences.logVitesse = server.hasArg("logVitesse");
  preferences.logAltitude = server.hasArg("logAltitude");
  preferences.logHeure = server.hasArg("logHeure");
  savePreferences();
  // effacer le fichier log en cours (si il existe ..)
  //le prochain sera recree avec le bon format
  Serial.print("On efface le fichier courant:"); Serial.println(logFileName);
  LittleFS.remove(logFileName);
  logFileName[0] ; // nom vide. On va le recreer
  displayOptionsSysteme("Préferences mise à jour.");
}
void handleOptionWifiProcess() {
  Serial.println(F("handleOptionWifiProcess"));
  // validation mini des adresses IP
  String message;
  IPAddress ipTest;
  bool errIp = (!ipTest.fromString(server.arg("local_ip"))) || (!ipTest.fromString(server.arg("gateway"))) || (!ipTest.fromString(server.arg("subnet")));
  if (errIp) {
    message = "Erreurs dans les adresses IP";
  }
  else {
    server.arg("password").toCharArray(preferences.password, 9); //   forcer à 8  ??     init ""  ????????????
    server.arg("local_ip").toCharArray(preferences.local_ip, 16); // La taille est limité a 15 par le HTML
    server.arg("gateway").toCharArray(preferences.gateway, 16);
    server.arg("subnet").toCharArray(preferences.subnet, 16);
    message = "Préferences mise à jour. La balise redémare";
  }
  savePreferences();
  displayOptionsSysteme(message);
  delay(1500);
  ESP.restart();
}

void handleOptionsPreferences() {
  server.sendContent_P(pageOption);
  String message = "<script>gel('password').value='" + String(preferences.password) + "'";
  message += ";gel('local_ip').value='" + String(preferences.local_ip) + "'";
  message += ";gel('gateway').value='" + String(preferences.gateway) + "'";
  message += ";gel('subnet').value='" + String(preferences.subnet) + "'";
  message += ";gel('logAfter').value=" + String(preferences.logAfter);
  message += ";gel('logOn').checked=" + String((preferences.logOn ? "true" : "false"));
  message += ";gel('logVitesse').checked=" + String( (preferences.logVitesse ? "true" : "false"));
  message += ";gel('logAltitude').checked=" + String( (preferences.logAltitude ? "true" : "false"));
  message += ";gel('logHeure').checked=" + String((preferences.logHeure ? "true" : "false" ));
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
  LittleFS.end();
  sendChunkDebut ( false );  // debut HTML style, sans  topMenu
  server.sendContent_P(byeBey);
  server.chunkedResponseFinalize();
  delay(1500);
  ESP.restart();
}

void fs_initServerOn( ) {
  server.on("/", handleCockpit_);
  server.on("/cockpit", handleCockpit_);
  server.on("/spiff", handleGestionSpiff);
  server.on("/delete_" , HTTP_POST, handleDelete);
  server.on("/formatage_", HTTP_GET, handleFormatage);  // danger mais un POST est diff a faire !!
  //server.on("/optionsPreferences", handleOptionsPreferences);
  server.on("/optionsSysteme", handleOptionsSysteme);
  server.on("/optionLogProcess", handleOptionLogProcess);
  server.on("/optionWiFiProcess", handleOptionWifiProcess);
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
