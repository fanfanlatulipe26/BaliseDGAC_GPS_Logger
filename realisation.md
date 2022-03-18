# Réalisation #

Quelques photos d'une réalisation.

**Composants:**
- module TTGO T-01C3 ESP32-C3
- module régulateur de tension AMS1117 800MA 5V à 3,3V
- module GPS QUECTEL L80
- petit morceau de plaquette circuit imprimé double face pour prototype.
- des broches droites / coudées récupérées sur des connecteurs.

Il est **très fortement recommandé** de programmer le module ESP32-C3 avec le logiciel Balise en utilisant un module adaptateur classique avant de câbler
ce montage.
Les mises à jour successives se feront en OTA.  
Une fois câblé, l'accès aux broches permettant un chargement via la liaison série est bien plus difficile.  

**Cablage**  
 | |Cablage|  |
  | -----------|---------  | ---------------- |  
 |TXD1 du L80  |<--->  | GPIO 8 de l'ESP01-C3  |
 |TXD1 du L80  |<---> |  GPIO 8 de l'ESP01-C3  |
 |Vcc du L80  |<--->  | +3.3v   |
 |V_BCKP du L80  |<--->  | +3.3v   |

|   <img src="/img_realisation/20220304_054235.jpg" width="400"> | <img src="/img_realisation/20220304_055358.jpg" width="400">  |
|  ------ | -------------- |
|   <img src="/img_realisation/20220304_092637.jpg" width="400"> | <img src="/img_realisation/20220304_094239.jpg" width="400">  |
|   <img src="/img_realisation/20220304_094310.jpg" width="400"> | <img src="/img_realisation/20220304_094325.jpg" width="400">  |
|   <img src="/img_realisation/20220304_085521.jpg" width="400"> | <img src="/img_realisation/20220304_095022.jpg" width="400">  |
|   <img src="/img_realisation/20220304_095044.jpg" width="400"> | |



