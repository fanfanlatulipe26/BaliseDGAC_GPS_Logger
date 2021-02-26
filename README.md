# BaliseDGAC_GPS_Logger
Version d'une balise de signalisation style DGAC pour drone et aéromodélisme avec enregistrement des traces
# Balise avec enregistrement de traces
Cette version de la balise DGAC de signalement électronique à distance pour drone et aéromodélisme est basée sur la version **GPS_Tracker_ESP8266V1_WEB** de "dev-fred" disponible à https://github.com/dev-fred/GPS_Tracker_ESP8266 .

La réalisation a été faite avec un ESP01 (ESP8266) et un GPS QUECTEL L80 ce qui donne une réalisation très compacte, mais bien d'autres  possibilités existent avec par exemple un ESP8266 D1, un GSP BN220 etc…

## Principales modifications:
- Extension de l'**interface Web**
- Ajout d'une fonction d'**enregistrement des traces** dans le système de fichiers de l'ESP avec interface Web de gestion (effacer / télécharger / choix des champs). 
- Modification de l'**identificateur de la balise**: l'adresse MAC est utilisée comme numéro de série.
- Ajout d'une fonction de mise à jour du logiciel à travers la liaison WiFI (**OTA** Over The Air)
- Ajout d'un **portail captif**: lors de la connexion au réseau créé par la balise le navigateur est lancé et on se retrouve directement dans l'interface utilisateur, sans besoin de donner une adresse IP
- Interface utilisateur pour la **gestion des préférences**, la gestion "système" etc …

## Compilation:
- Avant de compiler il faut choisir quelques options dans le fichier fs_option.h(choix des pins IO pour le GPS, choix d'inclure ou non la mise à jour par OTA, vitesse/gestion GPS)
- Le fichier compilé avec option OTA occupe environ 370Kb et si on veut maximiser la place laissée au système de gestion de fichiers on peut choisir dans l'IDE Arduino, pour une module ESP01 un map mémoire FS 256Kb/OTA 375kb
Outil/Type de Carte "Generic ESP8266 Module"   Flash Size 1MB(FS:256KB OTA ¨375KB) ce qui permet d'enregistrer plusieurs heures de vols.
Sans OTA on peut choisir un file system de 512KB
- Les librairies LittleFS, DNSServer, EEPROM sont installées en même temps que le SDK ESP8266.

|   ![](/img/cockpit_LI.jpg) | ![](/img/traces.png)  |
| ------------ | ------------ |

Le logiciel efface automatiquement les fichiers les plus anciens  quand la place manque dans la mémoire. Le fichier le plus ancien, le plus récent et le plus volumineux sont mis en valeur.  
Les points de trace sont enregistrés à la même cadence que l'émission des trames Beacon, dès que le fix GPS est fait, et éventuellement après que la balise se soit déplacée de N mètres(paramétrable).  
On peut importer ces fichiers CSV dans Google Maps pour visualiser:  
Google Maps/Menu/Vos adresses/Cartes/Créer une carte/Importer  
On peut aussi les transformer en fichier GPX, KML  etc avec par exemple https://www.gpsvisualizer.com/   
Etc …

![](/img/preferences.png)

La page" préférences "permet de choisir le contenu de la trace, sa mise en œuvre etc. Les coordonnées latitude/longitude sont toujours enregistrées.  
Pour l'aspect WiFi, par défaut le réseau est ouvert (pas de mot de passe) et l'adresse IP est 192.168.4.1  
Le portail captif permet une connexion aisée, sans le besoin de donner l'adresse IP (Fonctionne très bien sous Windows avec Firefox, Chrome, Edge. Est un peu plus capricieux sous Androiïd où il suffit de donner une adresse pouvant être valide comme xx.fr !!)  
Le bouton ***Reset*** redémarre la balise et ***Reset Usine*** restaure les préférences à leurs valeurs par défaut.  
***Format*** réinitialise le système de gestion de fichiers.  
***OTA*** permet une mise à jour du logiciel par la liaison WiFi.  
![](/img/OTA.png)

# Enjoy !  Les commentaires sont les bienvenus.
#### Idées de développements futurs
- Simplification de l'interface: je ne crois pas que le choix adresse Ip /gateway/sous réseau ait de l'intérêt
- Arrêt du webserver /AP après N minutes ?? pour ne pas interférer avec le 2.4G de la télécommande ??
- Choix du format trace CSV / GPX  (GPX plus coûteux en place disque)
- Choix de la fréquence de l'enregistrement de la trace indépendante de l'émission Beacon (périodicité, déplacement max)
- Statistiques sur l'utilisation du système de fichiers littleFS ( durée écriture de la trace …). 
- Ne garder que les N derniers fichiers ? (si problèmes de performance en écriture de llitleFS)
