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

#include "fs_options.h"

//  ATTENTION: il faut conditionner la compilation du code, sinonle linker
//            va tout prendre, même si on veut une version mini sans OTA...

#ifdef fs_OTA
char pageOTA[] PROGMEM = R"=====(
<form class="card" method='POST' id='upload_form' enctype='multipart/form-data'>
<input type='file' name='update' >
<input type='button' onclick='uploadFile()' style='background-color:red;' value='Update'>
</form>
<div>
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
if(xhr.readyState==4)gel('res').innerHTML=xhr.responseText;
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
background:#1f98bd81;
border-radius:10px;
color:#000;
margin:0px;
font-size:4vw;
text-align:center;
/* line-height: 0%; */
}
.gauche{
text-align:left;
}
.b1{
background-color:LimeGreen;
}  
.b2{
background-color:red; 
}
.box {
font-size:inherit;
}
input,button{
font-size:inherit;
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
.nobord{
border:0px;
}
input[type=checkbox]+label{
} 
input[type=checkbox]:checked+label{
background-color:#4CAF50;
} 
input[type=radio]:checked+label{
background-color:#4CAF50;
}
</style>
<script>
function D1(){return confirm("Effacer ce fichier ?");}
function gel(id){return document.getElementById(id);}
</script>
</head>
)=====";

#ifdef fs_STAT
char topMenu[] PROGMEM = R"=====(
<body>
<div class ="card">
<table class ='nobord'>
<tr>
<td class ='nobord'><button class='b1' onclick="document.location='/'">Cockpit</button></td>
<td class ='nobord'><button class='b1' onclick="document.location='/spiff'">Traces</button></td>
<td class ='nobord'><button class='b1' onclick="document.location='/optionsSysteme'">Préférences / Système</button></td>
<td class ='nobord'><button class='b1' onclick="document.location='/stat'">Stats</button></td>
</tr>
</table>
</div>
)=====";
#else
char topMenu[] PROGMEM = R"=====(
<body>
<div class ="card">
<table class ='nobord'>
<tr>
<td class ='nobord'><button class='b1' onclick="document.location='/'">Cockpit</button></td>
<td class ='nobord'><button class='b1' onclick="document.location='/spiff'">Traces</button></td>
<td class ='nobord'><button class='b1' onclick="document.location='/optionsSysteme'">Préférences / Système</button></td>
</tr>
</table>
</div>
)=====";
#endif
char menuSysteme[] PROGMEM = R"=====(
<div class ='card'>
<button class='b1' onclick="document.location='/OTA_'">MaJ OTA</button>
<button class='b1' onclick="document.location='/resetUsine'">Reset Usine</button>
<button class='b1' onclick="document.location='/reset'">Reset</button>
<button class='b2' onclick='if(confirm("Voulez vous vraiment reformater le syst&egrave;me de fichiers\n et tout effacer ?")){document.location="/formatage_"}'>Formatage</button>
</div>
)=====";

char pageOption[] PROGMEM = R"=====(
<div id="d1" class="card , gauche">
<form action="/optionLogProcess">
<b>Préférences gestion de la trace</b><br>
<input type="checkbox" id="logNo" name="logNo">
<label for="logNo">Pas d'enregistrement de trace</label><br>
<label for="logAfter">Enregistrement après un déplacement de </label> 
<input type="text" id="logAfter" name="logAfter" pattern="^-?[0-9]{1,}" size="1" required value="x">mètre(s).<br>   
Format de téléchargement 
<input type="radio" id="logCSV" name="formatTrace" value="csv">
<label for="logCSV">CSV</label> 
<input type="radio" id="logGPX" name="formatTrace" value="gpx" onclick="gel('logVitesse').checked=false">
<label for="logGPX">GPX</label><br> 
<input type="checkbox" id="logVitesse" name="logVitesse" onclick="gel('logGPX').checked=false;gel('logCSV').checked=true;"> 
<label for="logVitesse">Inclure la vitesse</label>  
<input type="checkbox" id="logAltitude" name="logAltitude">
<label for="logAltitude">Inclure l'altitude</label><br> 
<input type="checkbox" id="logHeure" name="logHeure">
<label for="logHeure">Inclure l'heure</label> 
<br><input type="submit" value="Submit">
</form>
<hr><b>Configuration GPS</b> 
<form action="/optionGPSProcess">
<label for="baud">Vitesse GPS:</label>
<select class ="box" id="baud" name="baud">
<option id ="B9600" value="9600">9600bds</option>
<option id ="B19200" value="19200">19200bds</option>
<option id ="B38400" value="38400">38400bds</option>
<option id ="B57600" value="57600">57600bds</option>
<option id ="B115200" value="115200">115200bds</option>
</select><br>
<label for="hz">Rafraichissement GPS:</label>
<select class="box"  id="hz" name="hz">
<option id ="F1" value="1">1hz</option>
<option id ="F3" value="3">3hz</option>
<option id ="F5" value="5">5hz</option>
<option id ="F7" value="7">7hz</option>
<option id ="F10" value="10">10hz</option>
</select>
<br><input type="submit" value="Submit">
</form>
<hr><b>Préférences WiFi (attention !!)</b> 
<form action="/optionWiFiProcess">
<label for="password">Mot de passe</label>
<input type="text" id="password" name="password" size ="8" minlength="8"  maxlength="8" value="">
<br><input type="submit" value="Submit">
</form>
</div></body></html>
)=====";


char byeBey[] PROGMEM = R"=====(
<div id="d1" class="card , gauche" >
La balise redémarre.  Bye Bye !!
</div></body></html>
)=====";

char cockpit[] PROGMEM = R"=====(
<script>
setInterval(function() {
getData();
},3000);//3000mSeconds update rate
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
</tr></table></div></body></html>
)=====";

#ifdef fs_STAT
char statistics[] PROGMEM = R"=====(
<script>
setInterval(function(){
getData();
},10000); //10000mSeconds update rate. 10s
function getData(){
var xhttp=new XMLHttpRequest();
xhttp.onreadystatechange=function() {
if(this.readyState==4&&this.status==200){
this.responseText.split(';').forEach((item)=>{
var i=item.split('$')[0].trim();
var val=item.split('$')[1].replace(/"/g,'').trim();
document.getElementById("Value"+i).innerHTML=val;
});
}
};
xhttp.open("GET","readStatistics",true);
xhttp.send();
}
getData();
</script>
<div class="card">
<button class='b1' onclick="document.location='/statReset'">RAZ stats</button>
<br><span id="Value0"></span>
<br><span id="Value1"></span>
<table border="1" style="line-height:100%;">
<tr>
<td></td>
<td>Min</span></td>
<td>Max</span></td>
<td>Moyen<br>géné</span></td>
<td>Moyen<br>sur 20</span></td>
</tr>
<tr>
<td>Durée ecr trace</td>
<td><span id="Value2">0</span></td>
<td><span id="Value3">0</span></td>
<td><span id="Value4">0</span></td>
<td><span id="Value5">0</span></td>
</tr>
<tr>
<td>Long segment</td>
<td><span id="Value6">0</span></td>
<td><span id="Value7">0</span></td>
<td><span id="Value8">0</span></td>
<td><span id="Value9">0</span></td>
</tr>
<tr>
<td>Période Beacon</td>
<td><span id="Value10">0</span></td>
<td><span id="Value11"></span></td>
<td><span id="Value12"></span></td>
<td><span id="Value13"></span></td>
</tr>
<tr>
<td>Période loop</td>
<td><span id="Value14"></span></td>
<td><span id="Value15"></span></td>
<td><span id="Value16"></span></td>
<td><span id="Value17"></span></td>
</tr>
<tr>
<td>Durée Server handle/pro</td>
<td><span id="Value18"></span></td>
<td><span id="Value19"></span></td>
<td><span id="Value20"></span></td>
<td><span id="Value21"></span></td>
</tr>
<tr>
<td>Durée Sendpkt</td>
<td><span id="Value22"></span></td>
<td><span id="Value23"></span></td>
<td><span id="Value24"></span></td>
<td><span id="Value25"></span></td>
</tr>
</table>
</div>
</body>
</html>
)=====";
#endif  // fin de la page specifique pour statistiques

char headerGPX[] PROGMEM = R"=====(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<gpx version="1.1"
creator="Balise"
xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
xmlns="http://www.topografix.com/GPX/1/1"
xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">
<trk>
<trkseg>
)=====";
