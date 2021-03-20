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

//------------------------------------------------------------------------------ 
// Quelques define pour choisir la configuration voulue
// si GPS_STANDARD est non defini on retrouve la séquence initialisation dans la version d'origine du GPS
//  avec selection canal Galileo/Gonas  (BN220 ???). Voir README de github
#define GPS_STANDARD
#define fs_OTA      // pour permettre une mise a jour OTA
//#define fs_STAT     // pour avoir une page Web de statisques sur durée exécution, erreurs GPS, etc ...

// pin a mettre temporairement à la masse  pour reinitilaiser les options/preferences "standard" lors d'un reset
// (utile si on a trop joué avec les préférences WiFi, adresse IP & Co ...
#define pinFactoryReset 2      

//#define ENABLE_BUZZER
//#define ENABLE_LED
#define GPS_RX_PIN 0            // D1 Brancher le fil Tx du GPS . FS pour ESP01
#define GPS_TX_PIN 2            // D2 Brancher le fil Rx du GPS . FS pour ESP01.(pour envoyer des commande si #9600bds et 1hz)

//#define GPS_RX_PIN 5            // D1 Brancher le fil Tx du GPS
//#define GPS_TX_PIN 4            // D2 Brancher le fil Rx du GPS

// ------------------------------------------------------------------------------------------------
#define GPS_9600 9600           // Valeur par défaut
#define GPS_57600 57600         // Autre config possible du GPS

#define BUZZER_PIN_G         14 //D5
#define BUZZER_PIN_P         13 //D7
