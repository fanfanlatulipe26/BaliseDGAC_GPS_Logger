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
// Recepteur trames Beacon type balise signalement DGA
// Credits:
// https://github.com/michelep/ESP32_BeaconSniffer
// https://www.tranquille-modelisme.fr/balise-dgac-signalement-electronique-a-distance-drone-aeromodelisme.html


#include "fs_options.h"
#ifdef fs_RECEPTEUR   // compilation conditionnelle de tout le code récepteur
#include <byteswap.h>
#include "esp_wifi.h"
#include <WiFi.h>
#include "fs_WebServer.h"
extern fs_WebServer server;
extern HardwareSerial serialGPS;
#include <LittleFS.h>

#include "fs_recepteur.h"
#include "fs_pagePROGMEM.h"

//#define LED 2
//#define ON HIGH
//#define OFF LOW

static const char *TAG = "sniffer";

#define OUI_VER 0x01355C6A

#define B_VERSION 0x01
#define B_IDENT_FR 0x02
#define B_IDENT_ANSI 0x03
#define B_CUR_LAT 0x04
#define B_CUR_LON 0x05
#define B_CUR_ALT 0x06
#define B_CUR_HAU 0x07
#define B_STA_LAT 0x08
#define B_STA_LON 0x09
#define B_CUR_VIT 0x0A
#define B_CUR_DIR 0x0B
#define B_CODE_INFO 0x0C

extern boolean modeRecepteur;  // false: mode balise avec emmision de trames; true: mode reception de trames beacon

static const char* const WIFI_PHY_RATE[] =
{
  "1_Mbps_with_long_preamble",
  "2_Mbps_with_long_preamble",
  "5.5_Mbps_with_long_preamble",
  "11_Mbps_with_long_preamble",
  "",
  "2_Mbps_with_short_preamble",
  "5.5_Mbps_with_short_preamble",
  "11_Mbps_with_short_preamble",
  "48_Mbps",
  "24_Mbps",
  "12_Mbps",
  "6_Mbps",
  "54_Mbps",
  "36_Mbps",
  "18_Mbps",
  "9_Mbps",
  "MCS0_with_long_GI,_6.5_Mbps_for_20MHz,_13.5_Mbps_for_40MHz",
  "MCS1_with_long_GI,_13_Mbps_for_20MHz,_27_Mbps_for_40MHz",
  "MCS2_with_long_GI,_19.5_Mbps_for_20MHz,_40.5_Mbps_for_40MHz",
  "MCS3_with_long_GI,_26_Mbps_for_20MHz,_54_Mbps_for_40MHz",
  "MCS4_with_long_GI,_39_Mbps_for_20MHz,_81_Mbps_for_40MHz",
  "MCS5_with_long_GI,_52_Mbps_for_20MHz,_108_Mbps_for_40MHz",
  "MCS6_with_long_GI,_58.5_Mbps_for_20MHz,_121.5_Mbps_for_40MHz",
  "MCS7_with_long_GI,_65_Mbps_for_20MHz,_135_Mbps_for_40MHz",
  "MCS7_with_short_GI,_72.2_Mbps_for_20MHz,_150_Mbps_for_40MHz",
  "250_Kbps_LORA",
  "500_Kbps_LORA",
};

static const char* const WIFI_CBW[] =
{
  "20MHz",
  "40MHz",
};
//DEBUT SITE WEB

//String trame = "";
//nom du point d'acces wifi
//const char ssid[] = "DECODEUR_DGAC_XAV";

// wifi channel doit être 6
const int wifi_channel = 6;
//pour la puissance
int8_t P1;

typedef struct
{
  float lat;
  float lon;
  int16_t alt;
  int16_t haut;
  float latHome;
  float lonHome;
  int16_t route;
  int16_t age = 20000; // pour simuler une balise pas mise à jour
  int8_t codeinfo;
  int8_t vitesse;
  int8_t  rssi;
  uint8_t cwb;
  uint8_t rate;
  char identifiant[31];
} balise_t;

balise_t lesBalises[5];
balise_t curBalise;
bool modeDetail ;  // true si on regarde le detil d'une balise : dans ce cas toujours la garder dansla table, même si elle est "vieille"
//  pas bon si plusieurs clients ....
int numBaliseDetail;

void handleRecepteurDetail()
{
  modeDetail = true;
  numBaliseDetail = server.arg("detail").toInt();
  Serial.print(F("Détail pour ")); Serial.print(numBaliseDetail); Serial.print("  ");Serial.println(lesBalises[numBaliseDetail].identifiant);
  server.chunkedResponseModeStart(200, "text/html");
  server.sendContent_P(fs_style);
  String page =  "<div class='card'>";
  page += "<p><b>Balise " +  String(lesBalises[numBaliseDetail].identifiant) + "</b></p>";
  
  page += "<table><tr><td colspan=2><b>Position de départ</b></td>";
  page += "<tr><td>Latitude: " + String(lesBalises[numBaliseDetail].latHome) + "</td><td>Longitude:" + String(lesBalises[numBaliseDetail].lonHome);
  page += "</td></tr>\n<tr><td colspan=2><b>Dernière position connue</b></td>";
  page += "<tr><td>Latitude: " + String(lesBalises[numBaliseDetail].lat) + "</td><td>Longitude: " + String(lesBalises[numBaliseDetail].lon);
  page += "</td></tr>\n<tr><td>Altitude: " + String(lesBalises[numBaliseDetail].alt) + "</td><td>Hauteur: " + String(lesBalises[numBaliseDetail].haut);
  page += "</td></tr>\n<tr><td>Vitesse: " + String(lesBalises[numBaliseDetail].vitesse) + "</td><td>Direction: " + String(lesBalises[numBaliseDetail].route);
  page += "</td></tr>\n<tr><td>RSSI: " + String(lesBalises[numBaliseDetail].rssi) + "</td><td>Age(sec): " + String(lesBalises[numBaliseDetail].age);
  page += "</td></tr>\n<tr><td>rate: " + String(WIFI_PHY_RATE[lesBalises[numBaliseDetail].rate]) + "</td><td>cwb: " + String(WIFI_CBW[lesBalises[numBaliseDetail].cwb]);
  page += "</td></tr></table>\n";
  page += "</div> ";
  page += "<script> function autoRefresh() {window.location.replace('/recepteurDetail?detail=" +String(numBaliseDetail)+ "');}";
  page += "setInterval('autoRefresh()', 1000); </script> </body> </html> ";
  server.sendContent(page);
  server.chunkedResponseFinalize();
}
void handleRecepteurRefresh()
{
  modeDetail = false;
  server.chunkedResponseModeStart(200, "text/html");
  server.sendContent_P(fs_style);
  String page = "<div class ='card'><button class='b1' onclick= \"document.location='/reset'\">Retour en mode émetteur (reset)</button></div>";
  page +=  "<div class='card'><form action='/recepteurDetail' method='post'>\n";
  page += "<table><tr><th>Balises dans le voisinage</th><th>RSSI</th><th>Age</th></tr>";
  for (int i = 0; i < sizeof (lesBalises) / sizeof( balise_t); i++) {
    if (lesBalises[i].age <= 60) { // ne lister que les balises reçues depuis moint de 60s
      page += "<tr><td><button class='b1' type='submit' name='detail' value='" + String(i) + "'>" + String(lesBalises[i].identifiant) + "</button></td>";
      //  page += "<tr><td><button class='b1' type='submit' name='detail' value='" +  String(lesBalises[i].identifiant) + "'>" + String(lesBalises[i].identifiant) + "</button></td>";
      page += "<td>" + String(lesBalises[i].rssi) + "</td>";
      page += "<td>" + String(lesBalises[i].age) + "</td></tr>\n";
    }
  }
  page += " </table> </form> </div> ";
  page += "<script> function autoRefresh() {window.location.replace('/recepteurRefresh');}";
  page += "setInterval('autoRefresh();', 1000); </script> </body> </html> ";
  server.sendContent(page);
  server.chunkedResponseFinalize();
}

static void decoupeID_balise()
{
  /*
    //découpe l'id de la balise:
    TRI_constructeur = ID_Balise.substring(0, 3);
    CODE_balise = ID_Balise.substring(3, 6);
    SN_balise = ID_Balise.substring(6);
  */
}


/** structure for Beacon Management Frame 802.11 + mandatory fields*/
typedef struct
{
  uint8_t subtype;              /**<  Frame subtype*/
  uint8_t type;               /**<  Frame typa*/
  uint16_t duration;              /**<  */
  uint8_t receiver_address[6];        /**<  Receiver mac-address*/
  uint8_t transmitter_address[6];       /**<  Transmitter mac-address*/
  uint8_t BSS_Id[6];              /**<  */
  uint16_t sequence_frag;           /**< 4 bits LSB frag, 12 bits MSB sequence sequence_frag = (0x000F & (frag)) | ( 0x0FFF & sequence<<4) */
  uint64_t timestamp;             /**<  Timestamp of frame*/
  uint16_t beacon_interval;         /** Interval between beacon frame */
  uint16_t capability;            /**mandatory fields */
  uint8_t ssid_element_id;          /** SSID Element ID */
  uint8_t ssid_length;            /** SSID length */
  uint8_t ssid_value[]; /** SSID value*/

} __attribute((__packed__)) frame_header_t;

typedef struct
{
  uint8_t type;       /**<  */
  uint8_t length;       /**<  */
  uint8_t payload[];      /**<  */
} __attribute((__packed__)) TLV_t;

void addBalise() {
  for (int i = 0; i < sizeof (lesBalises) / sizeof( balise_t); i++)
  {
    if (strncmp(curBalise.identifiant, lesBalises[i].identifiant, 30) == 0) {
      // La balise existe dans la table:  mise à jours des infos seulement
      lesBalises[i] = curBalise;
      lesBalises[i].age = 0;
      //  Serial.printf("Update balise % s en % i\n", lesBalises[i].identifiant, i);
      return;
    }
  }
  int ageM = 0, iAgeM = 0;
  // balise inconnue. La rajouter dans la table à la place de la balise qui n'est pas mise à jour depuis longtemps
  // Mais si est en mode detail, ne pas ecraser la balise qui est suivi, même si elle est tres vieille
  for (int i = 0; i < sizeof (lesBalises) / sizeof( balise_t); i++) {
    if(!modeDetail || i != numBaliseDetail){
    if (lesBalises[i].age > ageM ) {
      ageM = lesBalises[i].age;
      iAgeM = i;
    }
    }
    
  }
 
  lesBalises[iAgeM] = curBalise;
  lesBalises[iAgeM].age = 0;
  
  // Serial.printf("Rajout balise % s en % i\n", lesBalises[iAgeM].identifiant, iAgeM );
}


static void wifi_sniffer_cb(void *recv_buf, wifi_promiscuous_pkt_type_t type)
{

  static int32_t *oui_ver;
  static int32_t index, ptr, i;
  static wifi_promiscuous_pkt_t *sniffer;
  static TLV_t *tlv, *b_tlv;
  static vendor_ie_data_t *gse_vendor;
  static frame_header_t *packet;
  static float f_value;
  static char identifiant[31];

  sniffer = (wifi_promiscuous_pkt_t *)recv_buf;

  if ((*(uint16_t *)sniffer->payload) == 0x0080) //beacon frame
  {
    packet = (frame_header_t*) sniffer->payload;
    index = sizeof(*packet) + packet->ssid_length;
    while (index < (sniffer->rx_ctrl.sig_len - 4))
    {
      tlv = (TLV_t*)((sniffer->payload) + index);
      //WIFI_VENDOR_IE_ELEMENT_ID  ???
      if (tlv->type == 0xDD)
      {
        oui_ver = (int32_t*) tlv->payload;
        if ( *oui_ver == OUI_VER)
        {
          gse_vendor = (vendor_ie_data_t*)(tlv);
          ptr = 0;
          while (ptr < (gse_vendor->length - 4))
          {
            b_tlv = (TLV_t*)((gse_vendor->payload) + ptr);
            switch (b_tlv->type)
            {
              case B_VERSION :
                //s_value = (uint8_t)(*(b_tlv->payload));
                //printf ("\t\t\"version\": \"%d\",\r\n", s_value);
                break;
              case B_IDENT_FR :
                memcpy (curBalise.identifiant, b_tlv->payload, b_tlv->length);
                identifiant[b_tlv->length + 1] = '\0';
                break;
              case B_IDENT_ANSI :
                //memcpy (identifiant, b_tlv->payload, b_tlv->length);
                //identifiant[b_tlv->length + 1] = '\0';
                //printf("\t\t\"ident ansi\": \"%s\",\r\n", identifiant);
                break;
              case B_CUR_LAT :
                curBalise.lat = (float)((signed)(__bswap_32(*(int32_t*)(b_tlv->payload)))) / 100000.0;
                break;
              case B_CUR_LON :
                curBalise.lon = (float)((signed)(__bswap_32(*(int32_t*)(b_tlv->payload)))) / 100000.0;
                break;
              case B_CUR_ALT :
                curBalise.alt = __bswap_16(*(int16_t*)(b_tlv->payload));
                break;
              case B_CUR_HAU :
                curBalise.haut = __bswap_16(*(int16_t*)(b_tlv->payload));
                break;
              case B_STA_LAT :
                curBalise.latHome = (float)((signed)(__bswap_32(*(int32_t*)(b_tlv->payload)))) / 100000.0;
                break;
              case B_STA_LON :
                curBalise.lonHome = (float)((signed)(__bswap_32(*(int32_t*)(b_tlv->payload)))) / 100000.0;
                break;
              case B_CUR_VIT :
                curBalise.vitesse = *(b_tlv->payload);
                break;
              case B_CUR_DIR :
                curBalise.route = __bswap_16(*(int16_t*)(b_tlv->payload));
                break;
              case B_CODE_INFO :
                curBalise.codeinfo = (double)(*(b_tlv->payload));;
                break;
              default:
                //printf("b_tlv->type inconnu! : %0X\r\n", b_tlv->type);
                break;
            }
            ptr = ptr + (b_tlv->length) + 2;
          }

          //infos wifi
          curBalise.rssi = sniffer->rx_ctrl.rssi;
          curBalise.cwb = sniffer->rx_ctrl.cwb;
          curBalise.rate = sniffer->rx_ctrl.rate;

          addBalise() ;  //  rajouter la balise dans la table
        } //if vendor ok
      }
      index = index + tlv->length + 2;
    }
  }
}

/* Initialize wifi with tcp/ip adapter */
static void initialize_wifi_sniffer()
{
  wifi_promiscuous_filter_t wifi_promiscuous_filter =
  {
    .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT
  };
  //  initialize_nvs();
  // tcpip_adapter_init() ;
  // ESP_ERROR_CHECK(esp_event_loop_create_default());
  // wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  //  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  // ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  // ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
  //  ESP_ERROR_CHECK( esp_wifi_start() );

  esp_wifi_set_promiscuous_filter(&wifi_promiscuous_filter);
  esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_cb);
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
  esp_wifi_set_channel(wifi_channel, WIFI_SECOND_CHAN_NONE);
  ESP_LOGI(TAG, "start WiFi promiscuous ok");
}


hw_timer_t *timer = NULL;
bool timerOccurs = false;

void IRAM_ATTR onTimer() {
  timerOccurs = true;
}
void handleRecepteur()
{
  Serial.println(F("Lancement recepteur"));
  // couper la liaison avec le GPS (IT ...), ferme filesystem ...
  serialGPS.end();
  LittleFS.end();
  modeRecepteur = true;
  initialize_wifi_sniffer();
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);
  Serial.println(F("En attente de trames..."));
  handleRecepteurRefresh();
}

void loopRecepteur()  // appellé à chaque boucle depuis la boucle void loop()
{
  if (timerOccurs)
    // incrementer l'age des informations reçues des balises
  {
    //Serial.println("Timer !!");
    for (int i = 0; i < sizeof (lesBalises) / sizeof( balise_t); i++)
      lesBalises[i].age++;
    timerOccurs = false;
  }
}
#endif
