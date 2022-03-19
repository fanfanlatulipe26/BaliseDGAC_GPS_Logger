
Ceci est une version beta. Les remarques sont les bienvenues !

# **BaliseDGAC\_GPS\_Logger V3  Emetteur/Récepteur**
Version d'une balise de signalisation style DGAC pour  [signalisation de drones et aéromodèles](https://www.ecologie.gouv.fr/sites/default/files/notice_signalement_electronique.pdf) avec possibilité d'enregistrement des traces GPS. 
La balise a deux modes de fonctionnement:
- Mode émetteur
- Mode récepteur pour contrôler le fonctionnement des balises du voisinage

|   <img src="/img/bal1.jpg" width="400"> | <img src="/img/bal2.jpg" width="400">  |
| ------------ | ------------ |
|Balise réalisée avec un module ESP32-C3 T-01C3. Poids: 11g|  [Quelques photos de la réalisations](/realisation.md)|

## **Crédit:**
Le cœur du logiciel qui transmet la trame spécifique d’identification à distance pour drones et aéromodèles est basé sur la version [GPS\_Tracker\_ESP8266V1\_WEB](https://github.com/dev-fred/GPS_Tracker_ESP8266) de "dev-fred" ainsi que sur les travaux de ["Tr@nquille"](https://www.tranquille-informatique.fr/modelisme/divers/balise-dgac-signalement-electronique-a-distance-drone-aeromodelisme.html)  
Les parties interface WEB et enregistrement de traces ont été rajoutées.


## **Principales caractéristiques:**
- Génération des signaux de signalisation électronique pour les aéromodèles, suivant les prescriptions de l'[arrêté du 27 décembre 2019](https://www.legifrance.gouv.fr/jorf/id/JORFTEXT000039685188) (loi drone …).
- Mode émetteur ou récepteur.
- Code compatible ESP32/ ESP32-C3 / ESP8266. 
- Interface Web accessible sur un point d'accès (AP) créé par la balise. Gestion et contrôle du bon fonctionnement de la balise. Gestion des préférences …
- Portail captif: lors de la connexion au réseau créé par la balise le navigateur est lancé et on se retrouve directement dans l’interface utilisateur, sans besoin de donner une adresse.
- Possibilité de coupure du point d’accès pour ne pas interférer avec les signaux radio de télécommande et limiter fortement la consommation de la balise.
- Fonction d’enregistrement des traces GPS dans le système de fichiers de l’ESP avec interface Web de gestion. Téléchargement de traces en format CSV et/ou GPX.
- Fonction de mise à jour du logiciel à travers la liaison Wi-Fi (OTA Over The Air).

Cette balise peut être utilisée en dehors du contexte signalisation d'aéromodèles pour faire par exemple des tests de vitesse lors de la mise au point de mobiles, de bateaux du type racers/offshore, de modèles de voitures RC etc …[Exemple ici](#scenario)

## **Matériel supporté**
**Microcontrôleurs supportés:**
- ESP8266 (par exemple module ESP01)
- ESP32
- ESP32-C3 (par exemple module TTGO T-01C3 ESP32-C3)

**Modules GPS supportés:**
- Quectel L80 (et GPS style base chipset:MediaTek MT3339 ??)
- Beitian BN-220, BN-180, BN-880 (et GPS style base chipset: u-blox M8030-KT ??)  

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
Le logiciel remplace les 12 derniers caractères par l'adresse MAC de la balise assurant l'unicité de l'identifiant.   
L'interface utilisateur affiche l'identifiant de la balise qui devra être enregistré sur le site AlphaTango.


## **Environnement logiciel. Compilation**
Les tests ont été faits dans l'environnement IDE Arduino 18.19.  
Il est impératif d'avoir les environnements les plus récents pour ESP8266 et ESP32. (Février 2022: ESP32 2.0.2, ESP8266 3.0.2)  
Seule la librairie TinyGPS++ ne fait pas partie des packages standards ESP32/ESP8266 et doit être installée.

Avant de compiler il faut choisir quelques options dans le fichier **fs\_option.h** (choix du GPS, choix des ports de communication pour le GPS, choix d’inclure ou non la mise à jour par OTA, la disponibilté d'un LED accessible dans le montage,  etc. …). Voir les commentaires.   
Le mode "récepteur" n'est pas supporté pour l'ESP8266.   
Le choix du type de processeur est fait  lors de la compilation en sélectionnant le bon type de carte dans l'IDE Arduino

### **Modules GPS**

Choisir dans le fichier fs\_options.h  une des lignes :

\#define GPS\_quectel //  style Quectel L80  et GPS style base chipset MediaTek MT3339  
ou  
\#define GPS\_ublox   // pour Beitian BN-220, BN-180, BN-880 et GPS style base chipset u-blox M8030-KT.  

Le logiciel a été testé avec un GPS QUECTEL L80 et un Beitian BN-880 (dont la partie GPS est compatible avec un BN-180,BN-220, BN-280)   
Les GPS qui utilisent les commandes style $PMTK251, $PMTK220, $PMTK314 (cas de Quectel, GlobalTop/Sierra Wireless, …) peuvent sûrement être utilisés.

### **Utilisation d'un LED**
Si un LED est donné dans la configuration par */#define pinLed xx*  (voir fichier fs\_option.h)  son clignotement est rythmé par l'émission des trames d'identification. 
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
   <img src="/img/preferences.jpg" align=left width="400"> 
Cette fenêtre permet de choisir le format des  traces GPS, la configuration du GPS (vitesse/rafraîchissement), la gestion du point d’accès Wi-Fi.  
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

A noter que si on utilise un ESP8266, la liaison série avec le GPS est entièrement émulée par le logiciel avec parfois des problèmes de qualité de transmission pour des vitesses élevées. Avec un ESP32, la liaison est gérée bien plus efficacement par le matériel

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

Enjoy !:blush:
