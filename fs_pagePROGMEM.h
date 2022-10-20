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
#ifndef FS_PAGEPROGMEM_H
#define FS_PAGEPROGMEM_H
#include "fs_options.h"

//  ATTENTION: il faut conditionner la compilation du code, sinon le linker
//            va tout prendre, même si on veut une version mini sans OTA...

#ifdef fs_OTA
const char pageOTA[] PROGMEM = R"=====(
<form class="card" method='POST' id='upload_form' enctype='multipart/form-data'>
<input type='file' name='update' >
<input type='button' onclick='uploadFile()' style='background-color:red;' value='Update'>
</form>
<div class="card">
<progress id="bar" class="card" value="0" max="100"></progress>
</div>
<div id="res" class="card"></div>
<script>
function uploadFile() {
gel('res').innerHTML = "Attendre un message ERREUR ou OK !!";
var form=gel('upload_form');
var data=new FormData(form);
var xhr=new window.XMLHttpRequest();
xhr.upload.addEventListener('progress',h_pro,false);
xhr.upload.addEventListener('loadstart',h_loadstart,false);
xhr.upload.addEventListener('abort',h_abort,false);
xhr.upload.addEventListener('error',h_error,false);
xhr.upload.addEventListener('load',h_load,false);
xhr.upload.addEventListener('timeout',h_timeout,false);
xhr.upload.addEventListener('loadend',h_pro,false);
xhr.open('POST','/update');
xhr.send(data);
xhr.onreadystatechange=function(){
gel('bar').value=0;
if(xhr.readyState==4){gel('res').innerHTML=xhr.responseText;
setTimeout(function () {
window.location.href = '/cockpit'; 
}, 9000); //will call the function after 9 secs.
}
};
}
function h_loadstart(evt){console.log("h_loadstart");}
function h_abort(evt){alert("abort");}
function h_error(evt){alert("h_error");gel('res').innerHTML ="ERREUR !!";}
function h_load(evt){console.log("load");}
function h_timeout(evt){alert("h_timeout");}
function h_loadend(evt){alert("h_loadend");}
function h_pro(evt){
console.log("pro");
if (evt.lengthComputable){
var per=evt.loaded/evt.total;
gel('bar').value=Math.round(per*100) ;
}
}
</script>
)=====";
#endif 

const char fs_style[] PROGMEM = R"=====(
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
background-color: lightblue;
/*background-color:blue;*/
border-radius:10px;
margin:0px;
font-size:4vw;
text-align:center;
/* line-height: 0%; */
}
.gauche{
text-align:left;
}

.b0{
background-color:transparent;
} 

.b1{
background-color:LimeGreen;
}  
.b2{
background-color:red; 
}
.b3{
background-color:salmon; 
}
.box {
font-size:inherit;
font-family:inherit;
}
input,button{
font-size:inherit;
font-family:inherit;
border:5px solid  black ;
border-radius:10px;
}
table{
margin:auto;
}
table,td,th{
border:4px solid lime;
border-collapse:collapse;
font-size:inherit;
}
td.redb{
border:4px solid red;
}
.nobord{
border:0px;
}
input[type=checkbox]+label{
} 
input[type=checkbox]:checked+label{
background-color:#4CAF50;
} 

input[type=radio].checkedRed:checked+label{
background-color:red;
} 
input[type=radio]:checked+label{
background-color:#4CAF50;
}
input[type=submit]{
background-color:DarkGray;
}

</style>
<script>
function D1(){return confirm("Effacer ce fichier ?");}
function gel(id){return document.getElementById(id);}
</script>
</head>
)=====";
// location.replace('/recepteur')"   location.assign('/recepteur')"

const char topMenu[] PROGMEM = R"=====(
<body>
<div class ="card">
<table class ='nobord'>
<tr>
<td class ='nobord'><button class='b1' onclick="location.replace('/cockpit')">Cockpit</button></td>
<td class ='nobord'><button class='b1' onclick="location.replace('/spiff')">Traces</button></td>
<td class ='nobord'><button class='b1' onclick="location.replace('/optionsSysteme')">Préférences</button></td>
)====="
#ifdef fs_RECEPTEUR
R"=====(
<td class ='nobord'><button class='b1' onclick="window.history.pushState({'nom':'toto'}, 'il faut reset', '/reset');location.assign('/recepteur')">Récepteur</button></td>
)====="
#endif
R"=====(
</tr>
</table>
</div>
)=====";

const char menuSysteme[] PROGMEM = R"=====(
<div class ='card'>
)====="
#ifdef fs_OTA
R"=====(
<button class='b1' onclick="document.location='/OTA_'">MaJ OTA</button>
)====="
#endif
R"=====(
<button class='b1' onclick="document.location='/reset'">Reset</button>
<button class='b2' onclick='if(confirm("Voulez vous vraiment reformater le syst&egrave;me de fichiers\n et tout effacer ?")){document.location="/formatage_"}'>Formatage</button>
</div>
)=====";

const char pageOption[] PROGMEM = R"=====(
<div id="d1" class="card gauche">
<form action="/optionLogProcess">
<b>Gestion de la trace</b><input style="float: right;" type="submit" value="Envoyer">
<p>
Enregistrement de traces?
<input type="radio" id="ouiLog" name="logOn" value="Oui">
<label for="ouiLog">Oui</label> 
<input type="radio" class = "checkedRed" id="nonLog" name="logOn" value="Non">
<label for="nonLog">Non</label><br> 
<label for="logAfter">Distance entre 2 points </label> 
<input type="text" id="logAfter" name="logAfter" pattern="[0-9]{1,}" size="4" required value="x">
<select class ="box" id="unit" name="unit">
<option id ="metre" value="m">mètres</option>
<option id ="millis" value="ms">millis sec</option>
</select>
<br>
&nbsp;&nbsp;Enregistrer si la balise est immobile?
<input type="radio" id="ouiToujours" name="logToujours" value="Oui">
<label for="ouiToujours">Oui</label> 
<input type="radio"  id="nonToujours" name="logToujours" value="Non">
<label for="nonToujours">Non</label>
<br>   
Format de téléchargement 
<input type="radio" id="logCSV" name="formatTrace" value="csv">
<label for="logCSV">CSV</label> 
<input type="radio" id="logGPX" name="formatTrace" value="gpx" onclick="gel('logVitesse').checked=false">
<label for="logGPX">GPX</label><br> 
&nbsp;&nbsp;Inclure:&nbsp;&nbsp;&nbsp;<input type="checkbox" id="logVitesse" name="logVitesse" onclick="gel('logGPX').checked=false;gel('logCSV').checked=true;"> 
<label for="logVitesse">la vitesse</label>  
&nbsp;&nbsp;&nbsp;<input type="checkbox" id="logAltitude" name="logAltitude">
<label for="logAltitude">l'altitude</label>
&nbsp;&nbsp;&nbsp;<input type="checkbox" id="logHeure" name="logHeure">
<label for="logHeure">l'heure</label> <br>
<label for="nbrMaxTraces">Nombre maximal de traces à conserver:</label> 
<input type="number" id="nbrMaxTraces" name="nbrMaxTraces" style="width:3em;" min="5"  required>
</form>
<form action="/optionGPSProcess">
<hr><b>Gestion du GPS</b><input style="float: right;" type="submit" value="Envoyer">
<p> 
<label for="baud">Vitesse:</label>
<select class ="box" id="baud" name="baud">
<option id ="B9600" value="9600">9600bds</option>
<option id ="B19200" value="19200">19200bds</option>
<option id ="B38400" value="38400">38400bds</option>
<option id ="B57600" value="57600">57600bds</option>
</select>&nbsp;&nbsp;&nbsp;
<label for="hz">Rafraîchissement:</label>
<select class="box"  id="hz" name="hz">
<option id ="F1" value="1">1hz</option>
<option id ="F3" value="3">3hz</option>
<option id ="F5" value="5">5hz</option>
<option id ="F7" value="7">7hz</option>
<option id ="F10" value="10">10hz</option>
</select>
</form>
<form action="/optionPointAccesProcess">
<hr><b>Gestion du point d'accès Wifi</b><input style="float: right;" type="submit" value="Envoyer">
<p>
Couper le pt. d'accès en mouvement?
<input type="radio" id="ouiWifi" name="arretWifi" value="Oui">
<label for="ouiWifi">Oui</label> 
<input type="radio" class = "checkedRed" id="nonWifi" name="arretWifi" value="Non">
<label for="nonWifi">Non</label><br> 
<label for="timeoutWifi">&nbsp;&nbsp;&nbsp;&nbsp;Coupure après un délai de </label> 
<input type="number" id="timeoutWifi" name="timeoutWifi" style="width:3em;" min="30" max="300" required>secondes.<br>
Basse consommation après coupure?
<input type="radio" id="ouiConso" name="conso" value="Oui">
<label for="ouiConso">Oui</label> 
<input type="radio" class = "checkedRed" id="nonConso" name="conso" value="Non">
<label for="nonConso">Non</label><br> 
<p>
<label for="password">Mot de passe</label>
<input type="text" id="password" name="password" size ="8" minlength="8"  maxlength="8" value="">
<br>
<label for="ssid_AP">Nom du réseau</label>
<input type="text" id="ssid_AP" name="ssid_AP" pattern="[0-9a-zA-Z:_-]*" size ="35" minlength="1"  maxlength="32" value="">
</form>
)====="
#ifdef repondeurGSM
R"=====(
<form action="/optionSMSCommand">
<hr><b>Gestion de l'envoi d'un SMS de localisation</b><input style="float: right;" type="submit" value="Envoyer">
<p>
<label for="SMSCommand">Mot de passe</label>
<input type="text" id="SMSCommand" name="SMSCommand" size ="8" maxlength="8" value="">
 (pour <span id="myPhoneNumber">zzz</span> )
<br>
</form>
)====="
#endif
R"=====(
<form action="/resetUsine">
<hr><input style="float: right;" type="submit" value="Restauration des valeurs par défaut">
<p>&nbsp
</form>
</div>
)=====";


const char byeBye[] PROGMEM = R"=====(
<body onLoad="setTimeout( setTimeout(function(){ document.location.href='/cockpit'; }, 5000))">
<div id="d1" class="card gauche" >
La balise redémarre.  Bye Bye !!<br> (je reviens peut être dans 5s ...)
</div></body></html>
)=====";

const char cockpit[] PROGMEM = R"=====(
<script>
setInterval(function() {
getData();
},3027);//3000mSeconds update rate 
function getData(){
var xhttp=new XMLHttpRequest();
xhttp.onreadystatechange=function() {
// chaine reçue numero$valeur;numero$valeur ...
if (this.readyState==4&&this.status==200){
this.responseText.split(';').forEach((item)=>{
var i=item.split('$')[0].trim();
var val=item.split('$')[1].replace(/"/g,'').trim();
document.getElementById("Value"+i).innerHTML=val;
});
}
};
xhttp.open("GET","readValues",true);
xhttp.send();
}
getData();
</script>
<div class="card">
<span id="Value1">Bal</span>
<br><span id="Value2">ID</span>
<br><span id="Value3">Conso</span><br>
<table border="1" style="line-height: 100%;">
<tr>
<td class ='redb' colspan="2"><span id="Value5">arret wifi ..</span></td>
<td><span ><button class='b1' onclick="document.location='/giveMeTime'">+2min SVP !</button></span></td>
</tr>
<tr>
<td><span id="Value7"></span></td>
<td><span id="Value8"></span></td>
<td><span id="Value9"></span></td>
</tr>
<tr>
<td><span id="Value10"></span></td>
<td><span id="Value11"></span></td>
<td><span id="Value12"></span></td>
</tr>
<tr>
<td><span id="Value13"></span></td>
<td><span id="Value14"></span></td>
<td><span id="Value15"></span></td>
</tr>
<tr>
<td><span id="Value16"></span></td>
<td><span id="Value17"></span></td>
<td><span id="Value18"></span></td>
</tr></table>
</div>
)=====";


const char cockpit_fin[] PROGMEM = R"=====(
<div class="card">
<button class='b1' onclick="document.location='/razVMaxHMAx'">RAZ des VMax/HMAx</button>
)====="
#ifdef fs_STAT
R"=====(
<button class='b1' onclick="location.replace('/stat')">Stats</button>
)====="
#endif
R"=====(
</div></body></html>
)=====";


#ifdef fs_STAT
const char statistics[] PROGMEM = R"=====(
<script>
setInterval(function(){
getData();
},10000); //10000mSeconds update rate. 10s
function getData(){
var xhttp=new XMLHttpRequest();
xhttp.onreadystatechange=function() {
if(this.readyState==4&&this.status==200){
  this.responseText.split(';').forEach(func1);
}
};
xhttp.open("GET","readStatistics",true);
xhttp.send();
}

function func1(ligne,indexL){
 const mytable = document.getElementById("tbl");
 if (indexL <=1) document.getElementById("Value"+indexL).innerHTML = ligne;
 else {
  indexLigne = indexL-1;
 if (indexLigne > mytable.rows.length-1 ) {
  var row = mytable.insertRow(-1); 
   row.insertCell(0);row.insertCell(1);row.insertCell(2);
   row.insertCell(3);row.insertCell(4);
  }
ligne.split('$').forEach(func2);
function func2(val,indexCol) {
document.getElementById("tbl").rows[indexLigne].cells[indexCol].innerHTML = val;
}
}
}
getData();
</script>
<div class="card">
<button class='b1' onclick="document.location='/statReset'">RAZ stats</button>
<br><span id="Value0"></span>
<br><span id="Value1"></span>
<table id= "tbl" border="1" style="line-height:100%;">
<tr>
<td></td>
<td>Min</td>
<td>Max</td>
<td>Moyen<br>géné</td>
<td>Moyen<br>sur 20</td>
</tr>
<tr> <td></td> <td></td> <td></td> <td></td> <td></td> </tr>
</table>
</div>
</body>
</html>
)=====";
#endif  // fin de la page specifique pour statistiques

const char headerGPX[] PROGMEM = R"=====(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<gpx version="1.1"
creator="Balise"
xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
xmlns="http://www.topografix.com/GPX/1/1"
xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">
<trk>
<trkseg>
)=====";

// for the fun, un favicon  (un peu couteux 1406 octets ....) 
const char favicon[] PROGMEM = {
 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x10, 0x10, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x68, 0x05, 
0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 
0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x02, 0xA9, 0xDF, 0x00, 0x2B, 0x6F, 0x7A, 0x00, 0x2C, 0x75, 0x7A, 0x00, 0x00, 0xAF, 
0xE8, 0x00, 0x04, 0xB7, 0xD3, 0x00, 0x0B, 0x69, 0x7B, 0x00, 0x04, 0xB6, 0xD9, 0x00, 0x5F, 0x94, 
0xA8, 0x00, 0x04, 0x28, 0x38, 0x00, 0x00, 0x8D, 0xCB, 0x00, 0x38, 0x6F, 0x7A, 0x00, 0x06, 0x2C, 
0x32, 0x00, 0x04, 0x5D, 0x4F, 0x00, 0x04, 0xB9, 0xEB, 0x00, 0x3F, 0x70, 0x86, 0x00, 0x2C, 0x5F, 
0x6F, 0x00, 0x5A, 0x7B, 0x9D, 0x00, 0x05, 0xCE, 0xEB, 0x00, 0x04, 0xD7, 0xEB, 0x00, 0x27, 0x79, 
0x6C, 0x00, 0x08, 0xD0, 0xF1, 0x00, 0x12, 0x67, 0x5B, 0x00, 0x08, 0xD7, 0xEB, 0x00, 0x25, 0x55, 
0x52, 0x00, 0x04, 0xDE, 0xEE, 0x00, 0x00, 0x83, 0xC9, 0x00, 0x0D, 0xD6, 0xF1, 0x00, 0x06, 0xE4, 
0xEE, 0x00, 0x04, 0xBC, 0xDA, 0x00, 0x38, 0x71, 0x78, 0x00, 0x11, 0x6F, 0x73, 0x00, 0x34, 0x77, 
0x8A, 0x00, 0x07, 0xBD, 0xEF, 0x00, 0x02, 0xC8, 0xEC, 0x00, 0x16, 0x56, 0x56, 0x00, 0x2F, 0x67, 
0x6D, 0x00, 0x1C, 0x58, 0x53, 0x00, 0x00, 0xE4, 0xEF, 0x00, 0x2F, 0x6E, 0x79, 0x00, 0x00, 0xAE, 
0xE7, 0x00, 0x2B, 0xE0, 0xFD, 0x00, 0x00, 0xB9, 0xDB, 0x00, 0x37, 0x6C, 0x73, 0x00, 0x04, 0xBA, 
0xDE, 0x00, 0x21, 0x61, 0x65, 0x00, 0x3A, 0x70, 0x76, 0x00, 0x0B, 0x4E, 0x5A, 0x00, 0x00, 0xC0, 
0xE7, 0x00, 0x00, 0xC2, 0xE4, 0x00, 0x06, 0xC0, 0xDE, 0x00, 0x0E, 0xEC, 0xEC, 0x00, 0x24, 0x63, 
0x6B, 0x00, 0x35, 0x79, 0x88, 0x00, 0x00, 0x98, 0xD3, 0x00, 0x07, 0xBC, 0xED, 0x00, 0x00, 0xCC, 
0xE7, 0x00, 0x14, 0xB0, 0xE4, 0x00, 0x04, 0x64, 0x5D, 0x00, 0x2C, 0xD6, 0xD7, 0x00, 0x3D, 0x7F, 
0x88, 0x00, 0x3E, 0x7D, 0x8B, 0x00, 0x31, 0x62, 0x7A, 0x00, 0x57, 0x93, 0x9F, 0x00, 0x3B, 0xCD, 
0xE9, 0x00, 0x31, 0x6F, 0x74, 0x00, 0x4A, 0x7E, 0x85, 0x00, 0x0C, 0xD5, 0xF0, 0x00, 0x0D, 0xA2, 
0xDF, 0x00, 0x2E, 0x78, 0x74, 0x00, 0x22, 0x5E, 0x5D, 0x00, 0x08, 0xA8, 0xE8, 0x00, 0x03, 0xBD, 
0xDF, 0x00, 0x08, 0x56, 0x4F, 0x00, 0x15, 0x40, 0x55, 0x00, 0x27, 0xCE, 0xDB, 0x00, 0x06, 0xBD, 
0xE8, 0x00, 0x28, 0xCC, 0xE7, 0x00, 0x00, 0xCC, 0xE8, 0x00, 0x15, 0x51, 0x52, 0x00, 0x03, 0xC7, 
0xEB, 0x00, 0x1F, 0x41, 0x58, 0x00, 0x28, 0x6D, 0x66, 0x00, 0x32, 0x66, 0x63, 0x00, 0x28, 0xA6, 
0xD9, 0x00, 0x45, 0x71, 0x8C, 0x00, 0x07, 0xD0, 0xEB, 0x00, 0x13, 0x91, 0xD1, 0x00, 0x00, 0xA5, 
0xE0, 0x00, 0x56, 0x90, 0xA0, 0x00, 0x03, 0xDE, 0xE8, 0x00, 0x00, 0xB0, 0xD4, 0x00, 0x32, 0x68, 
0x72, 0x00, 0x36, 0x65, 0x7B, 0x00, 0x4D, 0x7B, 0x86, 0x00, 0x39, 0x70, 0x66, 0x00, 0x00, 0xB9, 
0xE6, 0x00, 0x60, 0x97, 0xA3, 0x00, 0x11, 0xDB, 0xF1, 0x00, 0x0B, 0xE9, 0xEE, 0x00, 0x3A, 0x71, 
0x7B, 0x00, 0x42, 0x6C, 0x75, 0x00, 0x08, 0xBC, 0xE6, 0x00, 0x35, 0xF3, 0xF0, 0x00, 0x27, 0xCB, 
0xEE, 0x00, 0x00, 0x98, 0xD5, 0x00, 0x04, 0xC2, 0xEF, 0x00, 0x3E, 0x78, 0x7E, 0x00, 0x00, 0xA2, 
0xCF, 0x00, 0x05, 0xCB, 0xE6, 0x00, 0x27, 0x89, 0x9F, 0x00, 0x43, 0x70, 0x8A, 0x00, 0x21, 0x74, 
0x73, 0x00, 0x1A, 0x50, 0x59, 0x00, 0x31, 0x65, 0x6A, 0x00, 0x48, 0x78, 0x7E, 0x00, 0x3D, 0x84, 
0x87, 0x00, 0x00, 0xA9, 0xDB, 0x00, 0x28, 0x4F, 0x4D, 0x00, 0x38, 0x66, 0x6D, 0x00, 0x2C, 0x72, 
0x76, 0x00, 0x50, 0x7A, 0x84, 0x00, 0x12, 0x6E, 0x62, 0x00, 0x03, 0xE8, 0xEC, 0x00, 0x12, 0x86, 
0xA0, 0x00, 0x73, 0xA7, 0xB8, 0x00, 0x35, 0x70, 0x79, 0x00, 0x36, 0x72, 0x76, 0x00, 0x03, 0xB5, 
0xE4, 0x00, 0x4E, 0x7E, 0x90, 0x00, 0x04, 0xB6, 0xE7, 0x00, 0x05, 0xBC, 0xDE, 0x00, 0x13, 0x4D, 
0x48, 0x00, 0x0C, 0xAD, 0xE7, 0x00, 0x03, 0xC0, 0xE1, 0x00, 0x5F, 0x96, 0xB3, 0x00, 0x2A, 0x8A, 
0x7F, 0x00, 0x33, 0x80, 0x7C, 0x00, 0x01, 0xC5, 0xE7, 0x00, 0x4C, 0x88, 0x9C, 0x00, 0x1A, 0x4A, 
0x51, 0x00, 0x10, 0xEB, 0xF5, 0x00, 0x00, 0x98, 0xD6, 0x00, 0x0D, 0xBA, 0xEA, 0x00, 0x3F, 0x78, 
0x88, 0x00, 0x17, 0xEF, 0xEF, 0x00, 0x4E, 0x93, 0xA2, 0x00, 0x1C, 0xBD, 0xD8, 0x00, 0x15, 0x60, 
0x5D, 0x00, 0x33, 0x6D, 0x68, 0x00, 0x18, 0xBE, 0xED, 0x00, 0x15, 0x5F, 0x63, 0x00, 0x69, 0x9F, 
0xC5, 0x00, 0x15, 0x7A, 0xA1, 0x00, 0x3F, 0x87, 0x91, 0x00, 0x21, 0x5D, 0x5D, 0x00, 0x22, 0x59, 
0x63, 0x00, 0x17, 0x69, 0x66, 0x00, 0x00, 0xBC, 0xDF, 0x00, 0x0C, 0x48, 0x55, 0x00, 0x01, 0xB4, 
0xEB, 0x00, 0x07, 0xAF, 0xE5, 0x00, 0x15, 0x42, 0x4C, 0x00, 0x1F, 0x67, 0x60, 0x00, 0x54, 0x7F, 
0x8B, 0x00, 0x33, 0x51, 0x5D, 0x00, 0x07, 0xB8, 0xE5, 0x00, 0x53, 0x80, 0x8E, 0x00, 0x10, 0x4E, 
0x55, 0x00, 0x4F, 0x85, 0x94, 0x00, 0x4F, 0x86, 0x97, 0x00, 0x04, 0x97, 0xCB, 0x00, 0x2F, 0xCD, 
0xDB, 0x00, 0x01, 0xCB, 0xE8, 0x00, 0x29, 0x66, 0x66, 0x00, 0x22, 0x70, 0x69, 0x00, 0x31, 0x64, 
0x60, 0x00, 0x3D, 0x74, 0x8F, 0x00, 0x08, 0x7E, 0x9C, 0x00, 0x32, 0x60, 0x6F, 0x00, 0x01, 0x85, 
0xA8, 0x00, 0x23, 0x49, 0x61, 0x00, 0x3A, 0x60, 0x6F, 0x00, 0x02, 0xAA, 0xE0, 0x00, 0x29, 0x73, 
0x7B, 0x00, 0x06, 0xDE, 0xEB, 0x00, 0x0E, 0x3E, 0x4A, 0x00, 0x02, 0xE2, 0xEE, 0x00, 0x45, 0x81, 
0x92, 0x00, 0x03, 0xB2, 0xDD, 0x00, 0x04, 0xB1, 0xE3, 0x00, 0x3B, 0x68, 0x75, 0x00, 0x33, 0x6E, 
0x7E, 0x00, 0x08, 0xAA, 0xE9, 0x00, 0x1C, 0x3E, 0x41, 0x00, 0x01, 0xB9, 0xE9, 0x00, 0x00, 0xC2, 
0xE0, 0x00, 0x1D, 0x6B, 0x64, 0x00, 0x02, 0xC7, 0xDD, 0x00, 0x03, 0x62, 0x53, 0x00, 0x28, 0x5F, 
0x6D, 0x00, 0x2D, 0x80, 0x90, 0x00, 0x52, 0x86, 0x98, 0x00, 0x30, 0x5A, 0x67, 0x00, 0x0E, 0x5A, 
0x56, 0x00, 0x00, 0xCA, 0xEF, 0x00, 0x08, 0xC9, 0xF5, 0x00, 0x01, 0xE5, 0xDD, 0x00, 0x2A, 0x76, 
0x6A, 0x00, 0x00, 0xDC, 0xEF, 0x00, 0x02, 0xDC, 0xEF, 0x00, 0x04, 0xB4, 0xC9, 0x00, 0x00, 0xAC, 
0xDE, 0x00, 0x00, 0x91, 0xA9, 0x00, 0x00, 0xAF, 0xDE, 0x00, 0x32, 0x69, 0x79, 0x00, 0x17, 0x63, 
0x5F, 0x00, 0x4A, 0x7D, 0x87, 0x00, 0x14, 0x67, 0x62, 0x00, 0x58, 0x91, 0xB0, 0x00, 0x19, 0xCB, 
0xE9, 0x00, 0x39, 0xAE, 0xD1, 0x00, 0x05, 0xB5, 0xDE, 0x00, 0x01, 0xF1, 0xEF, 0x00, 0x09, 0xE7, 
0xEC, 0x00, 0x0C, 0x4A, 0x54, 0x00, 0x35, 0x77, 0x76, 0x00, 0x02, 0xB4, 0xED, 0x00, 0x00, 0xC5, 
0xE1, 0x00, 0x21, 0x72, 0x5F, 0x00, 0x04, 0x61, 0x5A, 0x00, 0x0F, 0x8B, 0xC4, 0x00, 0x19, 0x4B, 
0x4E, 0x00, 0x02, 0xC1, 0xF0, 0x00, 0x1E, 0x49, 0x48, 0x00, 0x0D, 0x62, 0x54, 0x00, 0x55, 0xB6, 
0xBC, 0x00, 0x15, 0x5A, 0x57, 0x00, 0x05, 0xD1, 0xEA, 0x00, 0x48, 0x7B, 0x82, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x65, 0x90, 
0x5D, 0x59, 0x99, 0x3D, 0xE1, 0x0F, 0x4A, 0x3F, 0x9F, 0x09, 0x08, 0x6B, 0x53, 0x46, 0x64, 0xCA, 
0x81, 0xA9, 0x5A, 0x56, 0x93, 0x5C, 0xD7, 0x0C, 0x8B, 0x2F, 0xA8, 0xC8, 0x8C, 0x76, 0x77, 0x02, 
0x74, 0x20, 0x13, 0xB9, 0x1C, 0xC4, 0xB2, 0x01, 0x22, 0x43, 0x29, 0x7F, 0xE2, 0xEF, 0xBA, 0x52, 
0xBC, 0x35, 0xAD, 0xEE, 0xCD, 0xA0, 0xAB, 0x57, 0x75, 0x60, 0x06, 0x79, 0x95, 0xB0, 0xA5, 0xB1, 
0x27, 0xDD, 0x4C, 0x21, 0x0A, 0x39, 0xB4, 0xE7, 0x38, 0x8D, 0x70, 0x41, 0x18, 0xEA, 0xC2, 0x10, 
0xA6, 0x44, 0x69, 0x36, 0xC1, 0xB7, 0xD3, 0x96, 0xC3, 0xE0, 0xDF, 0x25, 0x72, 0x84, 0x5F, 0x8E, 
0x85, 0xD6, 0x66, 0x37, 0xBE, 0x58, 0x6C, 0x1A, 0x80, 0xBB, 0x33, 0xCF, 0x78, 0x9B, 0x51, 0xB6, 
0xBF, 0x7D, 0x30, 0xBD, 0xD4, 0xA1, 0x82, 0x47, 0xCE, 0xD2, 0x7B, 0x91, 0x67, 0x94, 0x4F, 0x1E, 
0xA7, 0x6E, 0x48, 0x5B, 0xDE, 0x9E, 0x50, 0x28, 0x31, 0x19, 0xD1, 0x26, 0x63, 0xEC, 0x2E, 0x2D, 
0x5E, 0x4D, 0x2A, 0x07, 0x9E, 0x8A, 0xD5, 0xE3, 0xE9, 0x17, 0x62, 0x7C, 0xCB, 0x9A, 0x97, 0xA3, 
0xAE, 0xDC, 0x2C, 0x83, 0x86, 0x6D, 0x1F, 0x6A, 0x0E, 0x12, 0x24, 0x0B, 0xA2, 0x9C, 0xAF, 0x0D, 
0x49, 0x4B, 0x1D, 0x32, 0xC6, 0x1B, 0xE6, 0x3B, 0x04, 0x15, 0x92, 0x71, 0x73, 0xB5, 0x16, 0xC5, 
0xE8, 0x40, 0xE4, 0x4E, 0xAC, 0x42, 0xCC, 0xC7, 0x8F, 0x68, 0xDB, 0x6F, 0x34, 0x2B, 0x3A, 0xED, 
0xA4, 0x05, 0xC9, 0xE5, 0x9D, 0xD0, 0x7A, 0xDA, 0x55, 0x54, 0xC0, 0xB3, 0xAA, 0xD9, 0x3C, 0xB8, 
0x7E, 0x03, 0x89, 0xEB, 0x45, 0x14, 0x88, 0xD8, 0x23, 0x11, 0x87, 0x3E, 0x98, 0x61, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00
};
#endif // #ifndef FS_PAGEPROGMEM_H
