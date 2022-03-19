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
// Quelques define pour choisir la configuration voulue
// Le code est compatible ESP32, ESP32-C3 et ESP8266.  
//  - dans l'IDE Arduino, sélectionner le bon type de carte pour la compilation 
//  - ci dessous
//      - définir le type de GPS
//      - définir les pins utilisées pour communiquer avec le GPS
//      - définir eventuellement la pin utilisée pour faire un reset "usine" 
//        Utile si on a perdu le mot de passe ...., et pour éviter de recharger en entier le logiciel ....
//        Pin a mettre temporairement à la masse pour reinitilaiser les options/preferences "standard" lors du reset
//      - définir eventuellement la pin utilisée pour commander un LED 
//        (cette pin ne peut pas être partagée avec un pin de de communication avec le GPS)
//

//  Option logiciel. 
#define fs_OTA      // pour permettre une mise a jour OTA
#define fs_RECEPTEUR  // pour generer le code récepteur (uniquement pour une carte à base ESP32 / ESP32C3)
//#define fs_STAT     // pour avoir une page de statisques,durée exécution, erreurs GPS, etc ...Utile en phase développement ..

// Si GPS_quectel ou GPS_ublox  est non defini on suppose que le GPS est initialisé et configuré , liaison à 9600 etc ....
#define GPS_quectel //  style Quectel L80  et GPS style base chipset:MediaTek MT3339
//#define GPS_ublox   // pour Beitian BN-220, BN-180, BN-880 et GPS style base chipset: u-blox M8030-KT 

// Pour ESP01 8266
//#define GPS_RX_PIN 0            // D1 Brancher le fil Tx du GPS . FS pour ESP01
////#define GPS_TX_PIN 2            // D2 Brancher le fil Rx du GPS . FS pour ESP01.(pour envoyer des commande )
#define pinFactoryReset 2 

// Pour ESP01-C3  , même format que ESP01 mais avec un ESP32-C3
// LILYGO® TTGO T-01C3 ESP32-C3
// Config OK
// Autre config LILYGO® TTGO T-01C3 ESP32-C3 plus simple cablage
#define GPS_RX_PIN 8           // D1 Brancher le fil Tx du GPS . FS pour ESP01-C3/
#define GPS_TX_PIN 9            // D2 Brancher le fil Rx du GPS . FS pour ESP01.(pour envoyer des commandes )
#define pinFactoryReset 2
#define pinLed 3  // builtin LED du module T-01C3

// Pour qq boards ESP32 dispo
//#define GPS_RX_PIN 15            // D1 Brancher le fil Tx du GPS . FS pour ESP32 OLED 128x64 compil carte WEMOS LOLIN32
//#define GPS_TX_PIN 13            // D2 Brancher le fil Rx du GPS . FS pour ESP32 OLED 128x64

//#define GPS_RX_PIN 15           // D1 Brancher le fil Tx du GPS . FS pour ESP32 Dev Kit
//#define GPS_TX_PIN 4            // D2 Brancher le fil Rx du GPS . FS pour ESP32 Dev Kit
//#define pinLed 2              // builtin LED du module 32 Dev Kit
//#define pinFactoryReset 13

//#define GPS_RX_PIN 5            // D1 Brancher le fil Tx du GPS
//#define GPS_TX_PIN 4            // D2 Brancher le fil Rx du GPS

// ------------------------------------------------------------------------------------------------

#endif  /* FS_OPTIONS_H */
