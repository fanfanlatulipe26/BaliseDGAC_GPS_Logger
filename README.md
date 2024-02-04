# **BaliseDGAC\_GPS\_Logger V4.2 Emetteur/Récepteur/Tracker GSM/Télémétrie FlySky iBus**

Version d'une balise de signalisation style DGAC pour  [signalisation de drones et aéromodèles](https://www.ecologie.gouv.fr/sites/default/files/notice_signalement_electronique.pdf) avec possibilité d'enregistrement des traces GPS et incluant optionnellement un module GSM permettant de recevoir un SMS de localisation. Télémétrie FlySky IBus possible pour un retour des informations GPS.
La balise a deux modes de fonctionnement:
- Mode émetteur
- Mode récepteur pour contrôler le fonctionnement des balises du voisinage

|   <img src="/img/bal1.jpg" width="400"> | <img src="/img/bal2.jpg" width="400">  |
| ------------ | ------------ |
|Balise réalisée avec un module ESP32-C3 T-01C3. Poids: 11g|  [Quelques photos de la réalisation](/realisation.md)|  

## **Crédit:**
Le cœur du logiciel qui transmet la trame spécifique d’identification à distance pour drones et aéromodèles est basé sur la version [GPS\_Tracker\_ESP8266V1\_WEB](https://github.com/dev-fred/GPS_Tracker_ESP8266) de "dev-fred" ainsi que sur les travaux de ["Tr@nquille"](https://www.tranquille-informatique.fr/modelisme/divers/balise-dgac-signalement-electronique-a-distance-drone-aeromodelisme.html)  
Les parties interface WEB, enregistrement de traces, tracker GSM, télémétrie ... ont été rajoutées.


## **Principales caractéristiques:**
- Génération des signaux de signalisation électronique pour les aéromodèles, suivant les prescriptions de l'[arrêté du 27 décembre 2019](https://www.legifrance.gouv.fr/jorf/id/JORFTEXT000039685188) (loi drone …).
- Mode **émetteur ou récepteur**.
- Possibilité d'inclure un module GSM pour recevoir un **SMS de localisation** facilitant la recherche du modèle en cas de perte.
- Possibilité de faire des retours de **télémétrie dans un environnement FlySky iBus**.
- Code compatible ESP32/ ESP32-C3 / ESP32-S3 / ESP8266. 
- **Interface Web** accessible sur un point d'accès (AP) créé par la balise. Gestion et contrôle du bon fonctionnement de la balise. Gestion des préférences …
- **Portail captif**: lors de la connexion au réseau créé par la balise le navigateur est lancé et on se retrouve directement dans l’interface utilisateur, sans besoin de donner une adresse.
- Possibilité de coupure du point d’accès pour ne pas interférer avec les signaux radio de télécommande et limiter fortement la consommation de la balise.
- Fonction **d’enregistrement des traces GPS** dans le système de fichiers de l’ESP avec interface Web de gestion. Téléchargement de traces en format CSV et/ou GPX.
- Fonction de mise à jour du logiciel à travers la liaison Wi-Fi (OTA Over The Air).
- **Possibilité de changer l'identificateur de la balise**.

Cette balise peut être utilisée en dehors du contexte signalisation d'aéromodèles pour faire par exemple des tests de vitesse lors de la mise au point de mobiles, de bateaux du type racers/offshore, de modèles de voitures RC etc …[Exemple ici](#scenario)

<figure>
    <img src="/img/schéma_2_schéma_1.jpg" width="600">
    <figcaption>Réalisation avec répondeur GSM optionnel</figcaption>
</figure><figure>
   <img src="/img/schéma_ibus_schéma.jpg" width="600"> 
    <figcaption>Réalisation avec télémétrie iBus</figcaption>
</figure>

Les composants entourés d'un cadre noir sont utilisés uniquement si un module GSM est inclus dans la configuration.  
Les noms des pins sur le module processeur ESP correspondent aux noms des pins qui doivent être définis dans le fichier fs_options.h (voir plus loin)

## **Matériel supporté**
**Microcontrôleurs supportés:**
- ESP8266 (par exemple module ESP01)
- ESP32
- ESP32-C3 (par exemple module TTGO T-01C3 ESP32-C3). [Voir ici quelques remarques](#ESP32C3)
- ESP32-S3<br>

**Modules GPS supportés:**
- Quectel L80 (et GPS style base chipset:MediaTek MT3339 ??)
- Beitian BN-220, BN-180, BN-880 (et GPS style base chipset: u-blox M8030-KT ??)  

**Module optionel GSM supporté:**
- La réalisation a été faite avec un mini module **GSM SIM800L** (module "rouge") coûtant quelques euros.<br>
[Voir ici quelques remarques sur le module SIM800L](#SIM800L)

Le choix d'un module  **LILYGO® TTGO T-01C3 ESP32-C3** ayant les mêmes dimensions et brochage qu'un ESP01 mais basé sur un ESP32-C3  permet une réalisation compacte et performante. Par rapport à un ESP01 classique, ce module dispose de plus de mémoire (4MB), de plus de puissance de traitement, d'un LED indépendant, d'une entrée/sortie supplémentaire, d'un connecteur pour une antenne externe optionnelle, etc.… Il semble aussi moins sensible aux problèmes d'alimentation que le module ESP01/ESP8266.  
Bien d’autres possibilités existent avec par exemple un ESP8266 D1, un GPS BN220 etc.

## **Message d'identification**
Dès que la balise est sous tension, même en l’absence d’un fix GPS, la balise émet des messages d'identification conforme à l'arrêté du 27 décembre 2019.  
Le format de l'identifiant diffusé est le suivant:
- Un code sur 3 caractères, sensé représenter le trigramme constructeur. 
  Il doit être **obligatoirement** 000 pour une construction amateur "DIY"
- Un code sur 3 caractères, sensé représenter le modèle de la balise 
- Un code sur 24 caractères, sensé représenter le numéro de série de la balise.

Il est donc du genre: "000FSB000000000000YYYYYYYYYYYY"  
Par défaut, le logiciel remplace les 12 derniers caractères par l'adresse MAC de la balise assurant l'unicité de l'identifiant.  
Il est possible, par l'interface Web, de changer les 24 derniers caractères de l'identifiant.  
L'interface utilisateur affiche l'identifiant courant de la balise qui devra être enregistré sur le site AlphaTango.

## **SMS de localisation**
Il est possible d'inclure dans la réalisation un module GSM permettant d'envoyer sur demande un SMS de localisation.
Par défaut, si la balise reçoit un SMS elle répond par un SMS contenant un lien avec les dernières coordonnées GPS valides connues. Ce lien ouvre Google Maps avec un pointeur sur la position du modèle.
Il est possible, par l'interface Web, de protéger cette fonction par un mot de passe: seul un SMS envoyé à la balise et contenant ce mot de passe provoquera l'envoi des coordonnées GPS du modèle. 

## **Télémétrie FlySky / iBus**
Testé avec un récepteur FS-iA6B et un émetteur avec le logiciel openTX.  
L’option fs_iBus donnée dans le  fichier fs_options.h permet de transformer la balise en un ensemble de capteurs FlySky/iBus. Seront transmises les informations suivantes venant du GPS:  
-	Les coordonnées GPS 
-	L’altitude en mètre
-	Le cap (0..360 deg, 0=nord)
-	La vitesse
  
2 broches d’entrées/sorties du processeur, iBus_RX et iBus_TX, définies dans le fichier fs_options  sont utilisées pour la liaison avec le récepteur. Le fils iBus, venant du récepteur RC est relié directement à iBus_RX, et par l’intermédiaire d’une diode (genre 1N4148) à iBux_TX (cathode vers iBUX_TX).  Voir  un exemple de câblage ci-dessus.   
La balise ne peut pas être utilisée dans une "daisy-chain" de capteurs. 
L’option télémétrie n’est supportée que sur les réalisations à base d’ESP32 (famille) et est incompatible avec l’option GSM.  
La mise en œuvre de la télémétrie peut être contrôlée par l’interface Web.  

## **Environnement logiciel. Compilation**
Il est impératif d'avoir les environnements les plus récents pour ESP8266 et ESP32. (Février 2024: ESP32 2.0.11, ESP8266 3.1.2)  
La librairie TinyGPSP++ ne fait pas partie des packages standards et doit être installée. Chercher TinyGPSPlus dans le library manager de l'IDE Arduino ou directement dans [GitHub](https://github.com/mikalhart/TinyGPSPlus).   
La librairie IBusBM doit être aussi installée si on souhaite mettre en oeuvre la télémetrie FLySky iBus. Chercher IBusBM dans le library manager de l'IDE Arduino ou directement dans [GitHub](https://github.com/bmellink/IBusBM).   

Avant de compiler il faut choisir quelques options dans le fichier **fs\_options.h** (choix du GPS, choix des ports de communication pour le GPS, choix d’inclure ou non la mise à jour par OTA, la disponibilté d'un LED accessible dans le montage,  etc. …). Voir les commentaires.   
Le mode "récepteur" n'est pas supporté pour l'ESP8266.   
Le choix du type de processeur est fait  lors de la compilation en sélectionnant le bon type de carte dans l'IDE Arduino

## **Modules GPS**

Choisir dans le fichier fs\_options.h  une des lignes :

\#define GPS\_quectel //  style Quectel L80  et GPS style base chipset MediaTek MT3339  
ou  
\#define GPS\_ublox   // pour Beitian BN-220, BN-180, BN-880 et GPS style base chipset u-blox M8030-KT.  

Le logiciel a été testé avec un GPS QUECTEL L80 et un Beitian BN-880 (dont la partie GPS est compatible avec un BN-180,BN-220, BN-280)   
Les GPS qui utilisent les commandes style $PMTK251, $PMTK220, $PMTK314 (cas de Quectel, GlobalTop/Sierra Wireless, …) peuvent sûrement être utilisés.

## **Utilisation d'un LED**
Si un LED est donné dans la configuration par */#define pinLed xx*  (voir fichier fs\_options.h)  son clignotement est rythmé par l'émission des trames d'identification. 
- En absence de fix GPS: clignotement lent, période de 6 secondes.
- Après un fix GPS: flash très rapide lors de l'envoi d'une trame. (le flash est un peu plus long si la balise est en mode économie d'énergie/mise en sommeil)

# **Mode Emetteur**
Cest le mode par défaut de la balise lors de sa mise sous tension. Les trames d’identification sont émises.  Une interface Web permet son contrôle après connexion au point d’accès  créé par la balise (système de portail captif)

## **Fenêtre « Cockpit »**

 <img src="/img/cockpit.jpg"  align="left" width="400">
Elle affiche l’état général de la balise et permet de contrôler son bon fonctionnement.  
La vitesse maximale ainsi que la hauteur maximale atteintes depuis la mise sous tension sont aussi affichées ainsi que le nombre de trames d'identification émises et le nombre de points GPS enregistrés depuis le dernier démarrage de la balise.
<br clear="both">  

## **Fenêtre « Trace »** 

|   <img src="/img/trace_2.jpg" width="400"> | <img src="/img/detail_trace.png" width="400">  |
| ------------ | ------------ |

La fenêtre « Trace » permet la gestion des traces GPS enregistrées.   
Des couleurs mettent en évidence  le fichier le plus ancien, le plus gros et le plus récent.  
Un click sur le nom d’un fichier ouvre une fenêtre donnant les caractéristiques principales de la trace (Nombre de points, heure de début/fin, vitesse maximum, hauteur maximum …)   
Un fichier peut être téléchargé localement pour analyse fine.

<a name="warning1">**Attention :** dans certains cas (sous Android ?),  le navigateur ouvert automatiquement lors de la connexion au portail captif ne permet pas de faire des téléchargements de fichiers. Il faut alors utiliser explicitement le navigateur standard. Ceci ne se produit pas sur un PC Windows.

## **Fenêtre « Préférences »**
Cette fenêtre permet de choisir le format des  traces GPS, la configuration du GPS (vitesse/rafraîchissement), la gestion du point d’accès Wi-Fi.  
  <br clear="both">  
   <img src="/img/preferences_1.jpg" align=left width="400"> 
   <img src="/img/preferences_2.jpg" align=right width="400">
  <br clear="both">  
  
### **Format de la trace GPS**
On peut choisir d'enregistrer ou non une trace GPS.
La création d’une trace ne commence qu’après l’obtention d’un fix GPS et l'enregistrement d'un nouveau point de trace est déclenché  (Choix exclusif):
- Soit par une distance parcourue supérieure à X mètres
- Soit par un temps écoulé de T millisecondes depuis le dernier point enregistré.  

Les traces sont enregistrées dans un format neutre et le choix du format GPX/CSV n'est pris en compte que lors du téléchargement: le même fichier peut donc être chargé une fois en format GPX, une fois en format CSV.

Les traces CSV sont plus faciles à analyser dans Excel par exemple et un site comme [GEO JAVAWA](https://www.geo.javawa.nl/trackanalyse/index.php?lang=en) permet une analyse fine, segment par segment de traces GPX. On peut importer ces fichiers CSV dans [Google Maps](https://www.google.fr/maps) pour visualisation: Google Maps/Menu/Vos adresses/Cartes/Créer une carte/Importer ou les transformer en fichier GPX, KML,… avec par exemple [GPSVisualizer](https://www.gpsvisualizer.com/)., etc ...

### **Configuration du GPS**
La vitesse de transmission du GPS et sa fréquence de rafraîchissement sont configurables (9600/19200/38400/57600 bds, 1/3/5/7/10 Hz).

La vitesse de transmission doit être augmentée si une fréquence de rafraîchissement  importante est choisie.

Si on veut enregistrer une trace fine, avec des points proches, il est nécessaire de choisir une fréquence de rafraîchissement élevée (par exemple si on utilise la balise pour une recherche de vitesse maximum sur de courtes périodes de temps). Le choix 19200Bds et 10Hz est un bon compromis.

A noter que si on utilise un ESP8266, la liaison série avec le GPS est entièrement émulée par le logiciel avec parfois des problèmes de qualité de transmission pour des vitesses élevées. Avec un ESP32, la liaison est gérée bien plus efficacement par le matériel.  

### **Gestion du point d'accès Wi-Fi**
A la mise sous tension, la balise crée un point d'accès Wi-Fi ouvert dont le nom par défaut est BALISE\_adresse-Mac (genre BALISE\_60:55:F9:71:59:5C)

Dès la connexion au réseau réalisée, un navigateur est ouvert et donne accès à l'interface utilisateur de gestion de la balise. Ce portail captif fonctionne très bien sous Windows mais est parfois un peu plus capricieux sous Android et il peut être nécessaire   de donner au navigateur une adresse quelconque du genre http://xx.fr pour accéder à l'interface utilisateur !!. Voir aussi  les problèmes potentiels lors du [téléchargement](#warning1) de traces GPS)

Il est possible de modifier le nom du réseau et de le protéger par un mot de passe. Si après modification du nom du réseau on veut rétablir son nom par défaut il suffit d'effacer le champ "Nom du réseau" et de soumettre cette modification. Il n'est pas nécessaire de se souvenir de l'adresse MAC de la balise.

Il est possible de couper le point d'accès en fonctionnement.  
Si la coupure est autorisée elle interviendra après un délai suivant l'obtention d'un fix GPS ou si la vitesse de déplacement de la balise est supérieure à 2m/s (7.2km/h)   
Si l'interface utilisateur est utilisée, ce délai commencera à courir uniquement lorsque la page "Cockpit" de l'interface sera affichée.

Il est possible de couper complètement l'activité Wi-Fi de la balise entre deux émissions de la trame d'identification. Ceci permet éventuellement une réduction des interactions entre les équipements de télécommande et la balise et **permet surtout d'abaisser fortement la consommation du dispositif**.

Il est à noter que la mise en sommeil de la balise entraîne un retard  de l'ordre de 20ms entre le moment où la trame doit être émise et son émission réelle. 

**Remarque :** Dans le cas d’une balise construite avec un ESP8266, il est important d’utiliser une version du package de base ESP8266 supérieure à 3.0.0 car sinon ce retard  atteint 300ms ou plus, ce qui peut être rédhibitoire si on veut en même temps  enregistrer des traces GPS  avec une haute résolution …  

### **Gestion de la télémétrie FlySky / iBus**
Il est possible de couper ou activer la télémesure.  

### **Gestion de l'identificateur de la balise**
**Aux risques et périls de l'utilisateur !!** (rester en règle avec AlphaTango ...)   
Par défaut l'identificateur de la balise est construit à partir de l'adresse MAC du controleur Wifi et est donc unique.
Il est du genre: "000FSB000000000000YYYYYYYYYYYY"  
Il est possible, par l'interface Web, de changer les 24 derniers caractères de l'identifiant. Les "espaces" et caractères accentués sont interdits. L'identificateur sera complété automatiquement à gauche par des 0.  
L'interface utilisateur affiche l'identifiant courant de la balise qui devra être enregistré sur le site AlphaTango.    
Pour rétablir l'identifiant par défaut, il suffit d'effacer complètement le champ identificateur et de soumettre cette modification.

Le changement de l'identificateur permet le déplacement de la balise d'un modèle à l'autre, même si ces modèles ne sont pas du même type (avions, drones, planeurs...) et de la même gamme de poids: il faut simplement enregistrer le bon identificateur pour le bon modèle dans AlphaTango et changer l'identificateur de la balise lors de l'installation dans le modèle .....

### **Bouton  « Reset » :** 
Redémarre la balise 
### **Bouton « Formatage » :**
Réinitialise le système de gestion de fichiers 
### **Bouton « Maj OTA »**
Permet une mise à jour très aisée du logiciel par la liaison Wi-Fi (au détriment de la mémoire réservée pour les fichiers traces, ce qui peut éventuellement être un problème avec  un module style ESP8266 avec peu de mémoire …)   
Pour une mise à jour, il suffit de sélectionner le fichier résultat de compilation qui est  en général dans  
  C:\Users\XXXXX\AppData\Local\Temp\arduino\_build\_YYYYYY\le\_projet.ino.bin  
(Sélectionner le répertoire arduino\_build\_... qui a la date de modification la plus récente)


# Mode Récepteur
|   <img src="/img/recepteur_1.png" width="400"> | <img src="/img/detail_balise.jpg" width="400">  |
| ------------ | ------------ |

Le mode récepteur permet de contrôler le fonctionnement des balises actives du voisinage. Une page liste les identifiants des balises les plus actives.  
  Un click sur un identifiant de balise ouvre une page contenant des détails sur les valeurs émises.   
  Le retour en mode « Emetteur » doit obligatoirement se faire en utilisant le bouton « Retour en mode émetteur » ou par un redémarrage complet de la balise (mise hors tension /en tension). Ne pas utiliser le bouton « retour » du navigateur Web.

<a name="ESP32C3">  
  
# Remarque sur l’utilisation d’un ESP32C3 avec option répondeur GSM ou télémétrie iBus
Le logiciel de la balise utilise une liaison série hardware (UART) pour communiquer avec le GPS.
De même il a besoin d’une autre liaison série hardware pour communiquer avec le module GSM, ou avec le iBus du récepteur.
L’ESP32C3 a uniquement 2 UARTs, 0 et 1. L’UART 1 est utilisé pour le GPS, et l’UART 0 est normalement utilisé par l’IDE Arduino pour télécharger le logiciel.  
Des module de petites tailles comme le LILYGO® TTGO T-01C3 ESP32-C3 ont peu de broches exposées et imposent de réutiliser des broches "réservées" à l’UART0/IDE.
Si on choisit l’option GSM ou iBus le logiciel utilise alors l’UART 0 et il devient difficile de télécharger le logiciel avec l’IDE.  
Il est conseillé avant de faire le câblage définitif, de télécharger une version simple du logiciel (sans GSM ou iBus) afin de faire quelques tests. Les versions suivantes seront chargées par OTA, sans lien direct avec l’IDE.  
Dans tous les cas on perdra la sortie de debug "Serial Monitor".  
Le problème ne se pose pas avec un ESP32 ou un ESP32S3 (3 UARTs disponibles)

<a name="SIM800L">

# Remarques sur le module GSM SIM800L  
  <p>
 <img src="/img/sim800L.jpg"  align="left"  width=500><img src="/img/IMG_20221016_145705.jpg" align="center"   width=400>
  </p>  
  
C'est un module 2G, parfois très instable et ne pouvant se connecter au réseau. Dans ce cas, le LED émet des séries de flashs rapides. Le problème est en général résolu en soudant  un condensateur de 1000µF **directement** en parallèle sur le gros condensateur jaune visible sur la photo ou en le remplacant directement par un nouveau condensateur tantale SMS  6V 1000uF 108 boitier type C.  

Il existe aussi sur le marché des modules incluant directement un condensateur de 1000µF (marqué 108 sur les photos), mais ils sont difficiles à trouver.  
Quand le module est en contact avec le réseau cellulaire, le LED émet un flash toutes les 3 secondes.  
 Une carte micro SIM est bien sûr nécessaire.(Un "simple" forfait 0€ / 2€ chez un opérateur français bien connu fait l'affaire ! ...)  


 <a name="scenario">
   
 # Scénario d'utilisation de la balise

Cette balise peut être utilisée en dehors du contexte signalisation d'aéromodèles pour faire par exemple des tests de vitesse lors de la mise au point de mobiles,  de bateaux du type racers/offshore, de modèle de voitures RC etc …
Scénario dutilisation:
- Ne pas permettre la coupure du point d’accès Wi-Fi
- Choisir une fréquence de rafraîchissement GPS élevée ( 19200bds / 10Hz)
- Choisir éventuellement de créer un fichier trace en prenant des points de mesure rapprochés (Un délai de 100ms entre points correspondants à la fréquence max de rafraîchissement du GPS)
- Faire un essai et faire revenir le mobile
- Se reconnecter au point d’accès créé pas la balise.
- Analyser la vitesse max rapportée dans la page « Cockpit »
- Remettre à zéro ces valeurs
- Changer les réglages et refaire un essai.
- Etc …

Les traces GPS enregistrées permettent de retrouver un historique des essais.

 # Principales modifications
- 4.0 final. Pas de changements
- 4.0b2
  - suppression de warning
  - correction problèmes include SoftwareSerial pour ESP8266
  - correction descriptions exemples de RX/TX pin pour module ESP8266 D1
- 4.0b1
  - ajout option répondeur GSM/SMS
  - gestion led "inversé"
-  3.1
     - amélioration du système de portail captif (changement de l'adresse IP de la balise etc...)
     - cosmétique dans l'interface Web
 
Enjoy !:blush:
