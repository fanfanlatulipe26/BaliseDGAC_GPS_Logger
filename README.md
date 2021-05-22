
# BaliseDGAC_GPS_Logger V3
Version d'une balise de signalisation style DGAC pour drone et aéromodélisme avec enregistrement des traces.
[Voir ici les principales modifications par rapport à la version 2](#principales-modifications-par-rapport-à-la-version-2)
# Balise avec enregistrement de traces
Le coeur du logiciel qui transmet la trame spécifique d'identification à distance pour drone et aéromodélisme est basé sur la version **GPS_Tracker_ESP8266V1_WEB** de "dev-fred" disponible à https://github.com/dev-fred/GPS_Tracker_ESP8266 ainsi que sur les travaux de "Tr@nquille" disponible à https://www.tranquille-informatique.fr/modelisme/divers/balise-dgac-signalement-electronique-a-distance-drone-aeromodelisme.html. Les parties interface WEB et enregistrement de traces ont été rajoutées.

Le code est compatible ESP8266 et ESP32. Voir quelques remarques ci après; XXXXXXXXXXXXXXXXXXX

La réalisation a été faite avec un ESP01 (ESP8266) et un GPS QUECTEL L80 ce qui donne une réalisation très compacte, mais bien d'autres  possibilités existent avec par exemple un ESP8266 D1, un GSP BN220 etc...
La consommation de cette configuration varie de 80 à 90mA.

## Principales caractéristiques:
- **interface Web** accessible sur un point d'accés (AP) crée par la balise. Gestion et controle du bon fonctionnement de la balise.
- Possibilité de **coupure du Point d'Accés** pour ne pas interférer avec les signaux radio de télécommande et limiter fortement la consommation de la balise.
- Fonction d'**enregistrement des traces** format **CSV/GPX** dans le système de fichiers de l'ESP avec interface Web de gestion (effacer / télécharger / choix des champs / conditions d'enregistrement). 
- **identificateur de la balise**: l'adresse MAC est utilisée comme numéro de série.
- Fonction de mise à jour du logiciel à travers la liaison WiFI (**OTA** Over The Air)
- **portail captif**: lors de la connexion au réseau créé par la balise le navigateur est lancé et on se retrouve directement dans l'interface utilisateur, sans besoin de donner une adresse IP
- Interface utilisateur pour la **gestion des préférences**, la gestion "système" etc …

|   ![](/img/cockpit_LI.jpg) | ![](/img/traces.png)  |
| ------------ | ------------ |

Si il y a plus de 4 fichiers de traces, le logiciel efface automatiquement les fichiers les plus anciens  quand la place manque dans la mémoire. Le fichier le plus ancien, le plus récent et le plus volumineux sont mis en valeur.  
La page "**Péférences**" permet de choisir le format de téléchargement des traces **CSV** et/ou **GPX** et la même trace peut être téléchargée dans les 2 formats. Les traces CSV sont plus faciles à analyser dans Excel par exemple, alors qu'un site comme [GEO JAVAWA](https://www.geo.javawa.nl/trackanalyse/index.php?lang=en) permet une analyse fine, segment par segment de traces GPX.
On peut importer ces fichiers CSV dans Google Maps pour visualisation: 
     Google Maps/Menu/Vos adresses/Cartes/Créer une carte/Importer  
ou les transformer en fichier GPX, KML,... avec par exemple [GPSVisualizer](https://www.gpsvisualizer.com/)

L'enregistrement des points de trace ne se fait que lorsque la balise est en mouvement et est totalement décorrélé de l'émission des trames d'identification.
La page "**Préférences**" permet de choisir la distance minimale qui provoque l'enregistrement, dès que le fix GPS est fait.
La vitesse de transmission du GPS et sa fréquence de rafraichissement sont aussi sélectionnées sur cette page (19200Bds et 10Hz conseillés)

…

![](/img/preferences.png)
 
Pour l'aspect WiFi, par défaut le réseau est ouvert (pas de mot de passe) et l'adresse IP est 192.168.4.1  
Le portail captif permet une connexion aisée, sans le besoin de donner l'adresse IP (Fonctionne très bien sous Windows avec Firefox, Chrome, Edge. Est un peu plus capricieux sous Android où il suffit de donner une adresse pouvant être valide comme xx.fr !!)  
Le bouton ***Reset*** redémarre la balise et ***Reset Usine*** restaure les préférences à leurs valeurs par défaut.  
***Format*** réinitialise le système de gestion de fichiers.  
***OTA*** permet une mise à jour du logiciel par la liaison WiFi.  
![](/img/OTA.png)
## Gestion de la trace
Chaque point de trace occupe 20 octets en mémoire, ce qui permet d'enregistrer environ 12000 points dans une partition filesystem de 256Ko. Avec un modèle volant à 20m/s (soit environ 70km/h) et en choisissant d'enregistrer 1 point pour chaque déplacement de 10m on pourra donc enregistrer environ 1h30 de vol. 
Le GPS met à jour ses mesures au maximum à 10hz soit toutes les 0.1s ce qui donne un enregistrement de 20mn seulement dans les mêmes conditions de vitesse si on choisit d'enregistrer la trace tous les 2m.
Pour un modèle volant au dessus de 50km/h (14 m/s)  il est illusoire de vouloir enregistrer une trace avec des points distants de 1m .....

Si besoin est, pour quelques dizaines de centimes il est possible d'augmenter la mémoire d'un ESP8266  jusqu'à 16mb ;-)

 ## Compilation
 
 Avant de compiler il faut choisir quelques options dans le fichier fs_option.h(choix des pins IO pour le GPS, choix d'inclure ou non la mise à jour par OTA, vitesse/gestion GPS, génération d'une page [statistiques](#génération-de-statistiques) etc ...)
 
 ###ESP8266
- pour une réalisation à base de ESP01 il est obligatoire d'utiliser les pin 0 et 2 pour le GPS (voir commentaire)
- Le fichier compilé avec option OTA occupe environ 373Kb et si on veut maximiser la place laissée au système de gestion de fichiers on peut choisir dans l'IDE Arduino, pour une module ESP01 une map mémoire FS 256Kb/OTA 375kb
Avec les options OTA et STA (statistiques) le fichier compilé occupe environ 378Kb et il faut choisir une map mémoire FS 192kb/OTA 406kb
Outil/Type de Carte "Generic ESP8266 Module"   Flash Size 1MB(FS:256KB OTA ¨375KB) ce qui permet d'enregistrer plusieurs heures de vols.
Sans OTA on peut choisir un filesystem de 512KB
- Pour un premier chargement du programme il est conseillé d'utiliser l'option Outils/Erase Flash: "All Flash Contents"
- Les librairies LittleFS, DNSServer, EEPROM sont installées en même temps que le SDK ESP8266. 

###ESP32
- la version actuelle (mai 2021) du SDK ESP32 1.0.6 ne contient pas de librairie LittleFS. Cela sera chose faite à partir de la version 2.0.0. En attendant il faut donc installer explicitement la librairie LITTLEFS à partir de https://github.com/lorol/LITTLEFS. Cette opération ne sera pas nécessaire lorsque le SDK ESP32 2.0.0 sera disponible.
- 
## Modules GPS
Le logiciel a été testé avec un GPS QUECTEL L80 dont la gestion est principalement assurée dans le fichier fs_GPS.cpp. Des GPS qui utilisent les commandes style $PMTK251, $PMTK220,  $PMTK314 (cas de Quectel, GlobalTop/Sierra Wireless, ...) peuvent être utilisés directement.  Les modules Beitian demandent sûrement une adaptation plus complexe.
Si le GPS a une configuration connue et satisfaisante lors d'un cold start, il suffit de valider les 2 premières lignes dans la fonction  fs_initGPS(). On perdra alors la possibilité de choisir dynamiquement la vitesse et la fréquence de rafraichissement.

## Problèmes connus

 - Il peut arriver (rarement) quue les trames d'identification ne soient plus émises, alors que
   le fix GPS est normal: le programme semble attendre (parfois
   plusieurs dizaines de secondes) quelque part dans le code des
   librairies spécifiques de l'ESP.... A approfondir ...
 - La communication entre le GPS et le processeur via SoftwareSerial
   n'est pas très robuste et parfois des erreurs de transmissions ne
   sont pas détectées, ce qui donne des points GPS aberrants conduisant
   à des calculs de longueur de segment parcourus faux et donc à des
   points de trace faux. [Voir TinyGPSPlus issue#87](https://github.com/mikalhart/TinyGPSPlus/issues/87)

## Principales modifications par rapport à la version 2

- sur option, coupure du point accès Wifi (interface WEB) pendant l'utilisation pour limiter la consommation.
- code compatible ESP32 / ESP8266 (choix d'une option)

## Principales modifications par rapport à la version 1

 - téléchargement de traces en format CSV et/ou GPX.
 - enregistrement de la trace décorrélé de l'émission des trames d'identification.
 - enregistrement des points de traces uniquement dicté par le déplacement de la balise.
 - choix possible de la vitesse de transmission du GPS et de sa fréquence de mise à jour (34500bds, 10hz recommandés).
 - au moins 4 fichiers traces sont gardés en mémoire.
 - option pour générer une page de statistiques pour analyser le comportement du logiciel.
## Génération de statistiques
Surtout intéressante dans un phase de développemnt/validation, l'option **fs_STAT** disponible dans le fichier **fs_options.h** permet de générer une page Web supplémentaire et de suivre en temps réel des statistiques sur le fonctionnement de la balise:
 - nombre de trames GPS correctes/incorrectes
 -  nombre de points traces enregistrés
 -  durée / période d'exécution de certaines parties du logiciel.
 -  ...
 
En phase de tests, il est possible de choisir une longueur de segment maximum négative à partir de la page de **Préférences**. Cette valeur devient alors en fait la période d'enregistrement de la trace: -200 donnera un point de trace toutes les 200ms.
 Dans tous les cas le fix GPS doit être validé. Ceci permet d'explorer rapidement "sur la table" des choix vitesse / rafraichissement GPS, de voir l'évolution du taux d'erreurs, des temps d’exécution etc ...

# Enjoy !  Les commentaires sont les bienvenus.
#### Idées de développements futurs. 

- nettoyage du code. Limiter l'utilisation de "string".
- mise à jour quand la version "officielle" de ystème de gestion de fichier sera disponible sur ESP32
- ???
