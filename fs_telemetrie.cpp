#include <Arduino.h>
#include "fs_options.h"
#if defined(fs_iBus)
#include "fs_telemetrie.h"
#pragma message "compil fs_telemetrie.h !."
#include <IBusBM.h>
#include <TinyGPS++.h>
#if defined(ESP32)
#if defined(CONFIG_IDF_TARGET_ESP32C3)
HardwareSerial iBusSerial(0);  // UART 0 sur ESP32C3 ( pour iBus  1 wire / half duplex)
#pragma message "Utilisation de HardwareSerial 0 ESP32C3 pour iBus  !."
#else
HardwareSerial iBusSerial(2);  // UART 2 sur ESP32 ( pour iBus  1 wire / half duplex)
#pragma message "Utilisation de HardwareSerial 2 ESP32 pour iBus !."
#endif
#else
#error "Il faut utiliser une carte ESP32 ou ESP32C3 pour la tememetrie iBus !."
#endif
IBusBM iBus;

uint8_t gps_lat;
uint8_t gps_lon;
uint8_t gps_alt;
uint8_t gps_speed;
uint8_t gps_spe;
uint8_t gps_status;
uint8_t gps_head;
uint8_t gps_second;
void iBusInit() {
#if defined(CONFIG_IDF_TARGET_ESP32C3)
  Serial.println("Arret de Serial uart0... pour l'utiliser pour la telemetrie");
  Serial.flush();
  Serial.end();
#endif
//   iBus.begin(iBusSerial, 0, iBus_RX, iBus_TX);  // des problèmes si on utilise le timer ?? avec le wifi ??
// //      Ok si init iBus apres Wifi & Co, avec timer 0 ??
#ifdef iBusUseTimer
  iBus.begin(iBusSerial, 1, iBus_RX, iBus_TX);  // timer 0 des problèmes avec EEPROM, wifi etc ... timer 1 idem ...
#else
  iBus.begin(iBusSerial, IBUSBM_NOTIMER, iBus_RX, iBus_TX);
#endif
  gps_lat = iBus.addSensor(IBUS_SENSOR_TYPE_GPS_LAT, 4);
  gps_lon = iBus.addSensor(IBUS_SENSOR_TYPE_GPS_LON, 4);
  gps_alt = iBus.addSensor(IBUS_SENSOR_TYPE_GPS_ALT, 4);
  gps_head = iBus.addSensor(IBUS_SENSOR_TYPE_CMP_HEAD, 2);
  gps_speed = iBus.addSensor(IBUS_SENSOR_TYPE_GROUND_SPEED, 2);
  gps_spe = iBus.addSensor(IBUS_SENSOR_TYPE_SPE, 2);  //
#define IBUS_SENSOR_TYPE_SPE 0x7e  // Speed 2bytes km/h
  //  gps_status = iBus.addSensor(IBUS_SENSOR_TYPE_GPS_STATUS, 2);  // en fait nombre de satellites sur 1er octete
  // gps_second = iBus.addSensor(IBUS_SENSOR_TYPE_RPM, 2);         // pour test ... On envoe le temps (seconde seulement)
}

void iBusSetValue(TinyGPSPlus gps) {
  iBus.setSensorMeasurement(gps_lat, gps.location.lat() * 1E7);  //4bytes signed WGS84 in degrees * 1E7
  iBus.setSensorMeasurement(gps_lon, gps.location.lng() * 1E7);  // 4bytes signed WGS84 in degrees * 1E7
  iBus.setSensorMeasurement(gps_alt, gps.altitude.value());     // en cm  . 4bytes signed!!! GPS alt m*100. 
  iBus.setSensorMeasurement(gps_head, gps.course.deg());        // en d°
  iBus.setSensorMeasurement(gps_speed, gps.speed.mps() * 100);  // en m/s *100
  iBus.setSensorMeasurement(gps_spe, gps.speed.kmph());  //  Speed 2bytes km/h
  //  iBus.setSensorMeasurement(gps_status, gps.satellites.value() << 8); // nbr de satelite le 1er octet (x12 ici). Le 2éme ??
  //  iBus.setSensorMeasurement(gps_second, gps.time.second());            // pour faire bouger ...
}
void iBusRestart() {
  iBusSerial.begin(115200, SERIAL_8N1, iBus_RX, iBus_TX);
}
void iBusStop() {
  iBusSerial.end();
}
void iBusLoop() {
  iBus.loop();
}
#endif  // #if defined(fs_iBus)
