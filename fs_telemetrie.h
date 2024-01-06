// Code capteurs dans https://github.com/opentx/opentx/blob/2.3/radio/src/telemetry/flysky_ibus.cpp
// et https://github.com/betaflight/betaflight/blob/master/src/main/telemetry/ibus_shared.h
#define IBUS_SENSOR_TYPE_GPS_LAT 0x80 //4bytes signed WGS84 in degrees * 1E7  :OK mais pas de diff and le nom "GPS"
#define IBUS_SENSOR_TYPE_GPS_LON 0x81 //4bytes signed WGS84 in degrees * 1E7 : OK mais pas de diff and le nom
#define IBUS_SENSOR_TYPE_GPS_ALT 0x82 //4bytes signed!!! GPS alt m*100. GAlt Semble OK même grande valeur sur 4octets
#define IBUS_SENSOR_TYPE_CMP_HEAD 0x08 //Heading  0..360 deg, 0=north 2bytes.  Nom:Hdg

#define IBUS_SENSOR_TYPE_RPM 0x07 // throttle value / battery capacity.    Pour test

//#define IBUS_SENSOR_TYPE_COG 0x0a //2 bytes  Course over ground(NOT heading, but direction of movement) in degrees * 100, 0.0..359.99 degrees. unknown max uint. Nom Hdg
#define IBUS_SENSOR_TYPE_GPS_STATUS 0x0b //2 bytes.   Seul 1er byte utilisé: nbr de satellites
#include <TinyGPS++.h>
void iBusInit();
void iBusSetValue(TinyGPSPlus gps); 
void  iBusLoop();
