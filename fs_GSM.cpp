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
#include "fsBalise.h"
#include "fs_options.h"
#include "fs_GSM.h"
#include "AsyncSMS.h"
#include "SoftwareSerial.h"

//#if defined(repondeurGSM) && !defined(ESP32)  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//#error "Option repondeurGSM valide uniquement avec une carte ESP32/ESP32C3"
//#endif
#ifdef repondeurGSM  //  compilation conditionnelle de tout le code GSM
extern double lat_prev;
extern double lon_prev;
extern pref preferences;
SoftwareSerial GSMSerial;
AsyncSMS smsHelper(&GSMSerial);
//AsyncSMS smsHelper(&GSMSerial, true);  // autoStateRefresh ??

void processingLogger(const char *msg) {
  Serial.println(msg);
}

void SMSReceived(char * number, char * message) {
  // lon = 2.294427;
  char smsBuffer[250];
  //Serial.printf_P(PSTR("SMS message received from %s: %s\n---------  SMS end -----------\n"), number, message);
  if ( strcmp(message, preferences.SMSCommand) == 0) {
    // send your location only if received SMS contains a secret sentence...
    sprintf_P(smsBuffer, PSTR( "Hi , I am here: https://www.google.com/maps/search/?api=1&query=%f%%2C%f"), lat_prev, lon_prev);
  }
  else
    sprintf_P(smsBuffer, PSTR("Hi , I am still alive !"));
  // Serial.printf_P(PSTR("SMS reply: %s\n"), smsBuffer);
  smsHelper.send(number, smsBuffer);
}
void GSMInit()
{
  Serial.print(F("Lecture venant du GSM sur pin ")); Serial.println(GSM_TX);
  Serial.print(F("Ecriture vers le GSM sur pin ")); Serial.println(GSM_RX);
  delay(10000);  // attente init GSM ... ???
#ifdef ESP32
  GSMSerial.begin(57600, SWSERIAL_8N1, GSM_TX, GSM_RX);  // attention !! Des pbs si vitesse faible ??  OK esp32
#else
  GSMSerial.begin(19200, SWSERIAL_8N1, GSM_TX, GSM_RX);  //
#endif
  if (!GSMSerial) { // If the object did not initialize, then its configuration is invalid
    Serial.println(F("Invalid SoftwareSerial pin configuration, check config"));
  }
  //  SerialGSM.println("AT");  SerialGSM.println("AT");
  smsHelper.smsReceived = *SMSReceived;
  smsHelper.logger = *processingLogger; // pour debug ...
  smsHelper.init();
  smsHelper.deleteAllSMS();

}


void GSMCheck()
{
  smsHelper.process();
}
#endif
