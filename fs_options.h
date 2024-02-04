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
#ifndef FS_OPTIONS_H
#define FS_OPTIONS_H
//------------------------------------------------------------------------------ 
// Quelques "#define" pour choisir la configuration voulue
// Le code est compatible ESP32, ESP32-C3 et ESP8266.  
//  - dans l'IDE Arduino, sélectionner le bon type de carte pour la compilation 
//  - ci dessous
//      - choisir la configuration logicielle: OTA, récepteur, GSM, telemetrie iBus
//        (GSM et telemetrie sont incompatibles)
//      - définir le type de GPS
//      - définir les pins GPS_RX_PIN et GPS_TX_PIN utilisées pour communiquer avec le GPS.
//      - définir eventuellement les pins iBus_RX et iBus_TX  utilisées pour communiquer
//        avec le recepteur FlySky /iBus pour la telemetrie.
//        ATTENTION:ne pas oublier la diode entre pin iBus_RX et récepteur , cathode cote Arduino.
//      - ATTENTION: avec un ESP32C3 l'option GSM ou iBus fait perdre les sortie de debug sur Serial monitor de l'IDE Arduino
//      - définir eventuellement la pin utilisée pour faire un reset "usine" 
//        Utile si on a perdu le mot de passe ...., et pour éviter de recharger en entier le logiciel ....
//        Pin a mettre temporairement à la masse pour reinitilaiser les options/preferences "standard" lors du reset
//      - définir eventuellement la pin utilisée pour commander un LED 
//        (cette pin ne peut pas être partagée avec un pin de de communication avec le GPS)
//        Si le LED est inversé ( ON si LOW) mettre en négatif la valeur de la pin.
//      - définir éventuellement les pins GSM_RX et GSM_TX utilisés pour communiquer avec le module GSM
//--------------------------------------------------------------------------------------------------------
//  Options logiciel. 
#define fs_OTA        // pour permettre une mise a jour OTA pour logiciel 
#define fs_RECEPTEUR  // pour inclure aussi le code récepteur (uniquement pour une carte à base ESP32 / ESP32C3)
//#define repondeurGSM  // pour envoyer dans un SMS la position avec une module GSM SIM800L (uniquement pour une carte à base ESP32 / ESP32C3)et incompatible avec fs_iBus
//                   Il faudra aussi définir GSM_RX et GSM_TX (voir exemples plus loin)
#define fs_STAT     // pour avoir une page de statisques,durée exécution, erreurs GPS, etc ...Utile en phase développement ..
#define fs_iBus // pour envois informations GPS par telemetrie type FlySky / iBUS (incompatible avec repondeurGSM)
//                  Il faudra aussi définir iBus_RX et iBus_TX (voir exemples plus loin)
//--------------------------------------------------------------------------------------------------------
//  Options choix du type de GPS
//--------------------------------------------------------------------------------------------------------
// Si GPS_quectel ou GPS_ublox  est non defini on suppose que le GPS est initialisé et configuré , liaison à 9600 etc ....
#define GPS_quectel  //  style Quectel L80  et GPS style base chipset:MediaTek MT3339
//#define GPS_ublox   // pour Beitian BN-220, BN-180, BN-880 et GPS style base chipset: u-blox M8030-KT 

//--------------------------------------------------------------------------------------------------------
//  Options configuration matérielle
//--------------------------------------------------------------------------------------------------------
// Pour ESP01 8266
/*
#define GPS_RX_PIN 0            // D1 Brancher le fil Tx du GPS . FS pour ESP01
#define GPS_TX_PIN 2            // D2 Brancher le fil Rx du GPS . FS pour ESP01.(pour envoyer des commande )
#define pinFactoryReset 2 
*/

//--------------------------------------------------------------------------------------------------------
// Pour 8266 ESP8266 D1 MINI Pro 
/*
#define GPS_RX_PIN 5 // sur pin 5, label D1, brancher le fil Tx du GPS .
#define GPS_TX_PIN 4 // sur pin 4, label D2, brancher le fil Rx du GPS .(pour envoyer des commandes )

#define GSM_RX 16   // sur pin 16, label D0, vers RX du module GSM SIM800L
#define GSM_TX 14   // sur pin 14, label D5, vers TX du module GSM SIM800L
//#define pinFactoryReset 2 
#define pinLed -2   //  LRD inversé sur ce moduke (ON si LOW)
*/
//--------------------------------------------------------------------------------------------------------------------------
// Pour ESP01-C3  , même format que ESP01 mais avec un ESP32-C3
// LILYGO® TTGO T-01C3 ESP32-C3
/*
#define GPS_RX_PIN 8           // D1 Brancher le fil Tx du GPS .
#define GPS_TX_PIN 9            // D2 Brancher le fil Rx du GPS .
#define pinFactoryReset 2
#define pinLed 3  // optionel. builtin LED du module T-01C3
// GSM pas testé sur ce module ....
//#define GSM_RX 2    //  exemple pour TTGO T-01C3 ESP32-C3 RX du module GSM SIM800L sur pin 2 GPIO2 module: transmssion ver GSM
//#define GSM_TX 20   //  exemple pour TTGO T-01C3 ESP32-C3 TX du module GSM SIM800L sur pin RX0 du module (pin GPIO20 de ESP32-C3): réception duGSM
//#define GSM_RX 21   // vers RX du module GSM SIM800L  U0TX
//#define GSM_TX 20   // vers TX du module GSM SIM800L  U0RX
// Par defaut Uart 0 est sur pin 20/rx et 21/tx
// On perdra le debug sur Serial
#define iBus_RX 20   // vers pin iBus sensor du recepteur Flysky
#define iBus_TX 21   // vers pin iBus sensor du recepteur Flysky
*/
//----------------------------------------------------------------------------------------------------------------------------
//  Pour module  ESP3-C3-32S-kit  NodeMCU-Series
/*
#define GPS_RX_PIN 9             // D1 Brancher le fil Tx du GPS . 
#define GPS_TX_PIN 10            // D2 Brancher le fil Rx du GPS . 
//#define GSM_RX 19   // vers RX du module GSM SIM800L
//#define GSM_TX 18   // vers TX du module GSM SIM800L (ne pas oublier la diode, cathode cote Arduino)
// On ne peut utiliser pin 20/21 car conflit avec adaptateur USB pour UART0 ???
#define iBus_RX 19   // vers pin iBus sensor du recepteur Flysky
#define iBus_TX 18   // vers pin iBus sensor du recepteur Flysky avec diode, cathode coté ESP
//#define pinLed 19
*/
//----------------------------------------------------------------------------------------------------------------------------
//  Pour ESP32 OLED 128x64 compil carte WEMOS LOLIN32
/*
#define GPS_RX_PIN 15            // D1 Brancher le fil Tx du GPS .
#define GPS_TX_PIN 13            // D2 Brancher le fil Rx du GPS .
*/

//----------------------------------------------------------------------------------------------------------------------------
//  Pour ESP32 ESP32 Dev Kit

#define GPS_RX_PIN 4           // D1 Brancher le fil Tx du GPS . 
#define GPS_TX_PIN 2            // D2 Brancher le fil Rx du GPS .
//#define pinLed 2              // builtin LED du module 32 Dev Kit
//#define pinFactoryReset 15
//#define GSM_RX 18   //  exemple pour ESP32 Dev kit. vers RX du module GSM SIM800L
//#define GSM_TX 19   //  exemple pour ESP32 Dev kit.  vers TX du module GSM SIM800L
//#define iBus_RX 25   //  exemple pour ESP32 Dev kit. vers pin iBus sensor du recepteur Flysky
//#define iBus_TX 27   //  exemple pour ESP32 Dev kit. vers pin iBus sensor du recepteur Flysky
#define iBus_RX 16   // (label RX2) exemple pour ESP32 Dev kit. vers pin iBus sensor du recepteur Flysky
#define iBus_TX 17   // (label TX2)  exemple pour ESP32 Dev kit. vers pin iBus sensor du recepteur Flysky


// ------------------------------------------------------------------------------------------------

#endif  /* FS_OPTIONS_H */
