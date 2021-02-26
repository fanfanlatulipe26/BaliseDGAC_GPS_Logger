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
/*
 * version 1 https://discuss.ardupilot.org/t/open-source-french-drone-identification/56904/98
 *
 */


#pragma once
#include <cstdint>
#include <chrono>

/**
  * Cette class implemente le système d'identification numérique des drones français conformément à
  * This class implement the french drone identification frame as stated in
  * https://www.legifrance.gouv.fr/eli/arrete/2019/12/27/ECOI1934044A/jo/texte
  */
class droneIDFR {
public:
    /**
     * Constructeur de la librairie. Utilise une valeur par default pour l'ID du drone
     */
    //droneIDFR(): _droneID("ILLEGAL_DRONE_APPELEZ_POLICE17") {};
    /**
     *
     * Taille maximale de la frame
     *
     */
    static constexpr uint8_t FRAME_PAYLOAD_LEN_MAX = 251;
  
    /**
     * Setter position avec les coordonnées GPS en entier en centidegrees + altitude
     * @param lat
     * @param lon
     * @param alt
     */
    void set_current_position(int32_t lat, int32_t lon, int16_t alt) {
        _latitude = lat * 1e-2;
        _longitude = lon * 1e-2;
        _altitude = alt;

        _new_GPS_data = true;
        std::chrono::duration<double> time_period_GPS = std::chrono::high_resolution_clock::now() - _last_data_rcv;
        _last_data_rcv = std::chrono::high_resolution_clock::now();
        
        _data_period_GPS = (_data_period_GPS * 0.8) + (time_period_GPS.count() * 0.2);
        
        if(_home_set){
          // Update height above Home
          _height = _altitude - _home_altitude;

          // 2D distance from previous position sent (meters, deg)
          _2D_distance = distanceBetween(static_cast<double>(lat) * 1e-7, static_cast<double>(lon) * 1e-7, static_cast<double>(_last_latitude_sent) * 1e-5, static_cast<double>(_last_longitude_sent) * 1e-5);
          
          // 3D distance from last position sent (meters)
          _3D_distance_from_last_point_sent = sqrt((_2D_distance * _2D_distance) + ((_last_altitude_sent - _altitude) * (_last_altitude_sent - _altitude)));
        }
    }
    /**
     * Setter position avec les coordonnées GPS en double en centidegrees + altitude
     * Converti les valeurs en entiers.
     * @param lat
     * @param lon
     * @param alt
     */
    void set_current_position(double lat, double lon, int16_t alt) {
        _latitude = lat * 1e5;
        _longitude = lon * 1e5;
        _altitude = alt;

        _new_GPS_data = true;
        std::chrono::duration<double> time_period_GPS = std::chrono::high_resolution_clock::now() - _last_data_rcv;
        _last_data_rcv = std::chrono::high_resolution_clock::now(); 
        
        _data_period_GPS = (_data_period_GPS * 0.8) + (time_period_GPS.count() * 0.2);

        if(_home_set){
          // Update height above Home
          _height = _altitude - _home_altitude;

          // 2D distance from previous position sent (meters, deg)
          _2D_distance = distanceBetween(lat,lon, static_cast<double>(_last_latitude_sent) * 1e-5, static_cast<double>(_last_longitude_sent) * 1e-5);
          
          // 3D distance from last position sent (meters)
          _3D_distance_from_last_point_sent = sqrt((_2D_distance * _2D_distance) + ((_last_altitude_sent - _altitude) * (_last_altitude_sent - _altitude)));
        }
    }
    /**
     * Setter position Home avec les coordonnées GPS en entiers en centidegrees + altitude
     * @param lat
     * @param lon
     * @param alt
     */
    void set_home_position(int32_t lat, int32_t lon, int16_t alt) {
        _home_latitude = lat * 1e-2;
        _home_longitude = lon * 1e-2;
        _home_altitude = alt; 

        // Init
        _last_latitude_sent = _home_latitude;
        _last_longitude_sent = _home_longitude;
        _last_altitude_sent = _home_altitude;
        
        _home_set = true;
    }
    /**
     * Setter position Home avec les coordonnées GPS en double en centidegrees + altitude
     * Converti les valeurs en entiers.
     * @param lat
     * @param lon
     * @param alt
     */
    void set_home_position(double lat, double lon, int16_t alt) {
        _home_latitude = lat * 1e5;
        _home_longitude = lon * 1e5;
        _home_altitude = alt; 
        
        // Init
        _last_latitude_sent = _home_latitude;
        _last_longitude_sent = _home_longitude;
        _last_altitude_sent = _home_altitude;
        
        _home_set = true;
    }
      
    /**
     * Setter pour la vitesse au sol en m/s
     * @param ground_speed
     */
    void set_ground_speed(double ground_speed) {
        _ground_speed = ground_speed;
    }
    /**
     * Cap du drone en degree par rapport au nord
     * @param heading
     */
    void set_heading(uint16_t heading) {
        _heading = heading;
    }
    /**
     * Setter pour l'id du drone.
     * Utiliser cette fonction pour changer l'id par défault
     * @param id_value
     */
    void set_drone_id(const char* id_value) {
        // don't use std::copy as it isn't support on all targets like espressif32 sdk !
        // TODO : if size(id_value) < TLV_LENGTH[ID_FR], fill with 0
        memcpy(_droneID, id_value, TLV_LENGTH[ID_FR]);
    }
    /**
     * Renvoie la distance (3D) par rapport à la dernière position envoyée.
     * 
     */
    double get_distance_from_last_position_sent(){
        return _3D_distance_from_last_point_sent;
    }
    /**
     * Renvoie la vitesse GPS au sol.
     * @return ground speed in km/h
     */
    double get_ground_speed_kmh(){
        return _ground_speed * 3.6;
    }
    /**
     * Genère la frame 802.11 beacon complète
     * @param full_frame beacon frame buffer
     * @param start_from starting offset on the buffer
     * @return buffer space used
     */
    uint8_t generate_beacon_frame(uint8_t* full_frame, uint16_t start_from)
    {
        // Vendor specific 802.11 beacon frame
        full_frame[start_from] = FRAME_VS;
        start_from++;
        const uint16_t payload_marker = start_from;
        start_from++;

        for (auto i = 0; i<3; i++) {
            full_frame[start_from] = FRAME_OUI[i];
            start_from++;
        }
        full_frame[start_from] = FRAME_VS_TYPE;
        start_from++;
        const uint8_t payload_size = generate_drone_frame(full_frame, start_from);  // remove payload
        full_frame[payload_marker] = payload_size + 4;  // +OUI + VSTYPE
        return start_from + payload_size;
    }
    /**
     * Genère le contenu de l'identification drone
     * @param full_frame beacon frame buffer
     * @param start_from starting offset on the buffer
     * @return buffer space used
     */
    uint8_t generate_drone_frame(uint8_t* full_frame, uint16_t start_from) {
        uint8_t count = 0;
        // Protocol version
        full_frame[start_from + count] = PROTOCOL_VERSION;
        count++;
        full_frame[start_from + count] = TLV_LENGTH[PROTOCOL_VERSION];
        count++;
        full_frame[start_from + count] = FRAME_VS_TYPE;
        count++;
        // Drone ID FR
        full_frame[start_from + count] = ID_FR;
        count++;
        full_frame[start_from + count] = TLV_LENGTH[ID_FR];
        count++;
        for (auto i = 0; i < TLV_LENGTH[ID_FR]; i++) {
            full_frame[start_from + count] = _droneID[i];
            count++;
        }
        // LATITUDE
        full_frame[start_from + count] = LATITUDE;
        count++;
        full_frame[start_from + count] = TLV_LENGTH[LATITUDE];
        count++;
        for (auto i = TLV_LENGTH[LATITUDE] - 1; i >= 0; i--) {
            full_frame[start_from + count] = (get_2_complement(_latitude) >> (8 * i)) & 0xFF;
            count++;
        }
        // LONGITUDE
        full_frame[start_from + count] = LONGITUDE;
        count++;
        full_frame[start_from + count] = TLV_LENGTH[LONGITUDE];
        count++;
        for (auto i = TLV_LENGTH[LONGITUDE] - 1; i >= 0; i--) {
            full_frame[start_from + count] = (get_2_complement(_longitude) >> (8 * i)) & 0xFF;
            count++;
        }
        // ALTITUDE
        full_frame[start_from + count] = ALTITUDE;
        count++;
        full_frame[start_from + count] = TLV_LENGTH[ALTITUDE];
        count++;
        for (auto i = TLV_LENGTH[ALTITUDE] - 1; i >= 0; i--) {
            full_frame[start_from + count] = (get_2_complement(_altitude) >> (8 * i)) & 0xFF;
            count++;
        }
        // HEIGHT
        full_frame[start_from + count] = HEIGTH;
        count++;
        full_frame[start_from + count] = TLV_LENGTH[HEIGTH];
        count++;
        for (auto i = TLV_LENGTH[HEIGTH] - 1; i >= 0; i--) {
            full_frame[start_from + count] = (get_2_complement(_height) >> (8 * i)) & 0xFF;
            count++;
        }
        // HOME LATITUDE
        full_frame[start_from + count] = HOME_LATITUDE;
        count++;
        full_frame[start_from + count] = TLV_LENGTH[HOME_LATITUDE];
        count++;
        for (auto i = TLV_LENGTH[HOME_LATITUDE] - 1; i >= 0; i--) {
            full_frame[start_from + count] = (get_2_complement(_home_latitude) >> (8 * i)) & 0xFF;
            count++;
        }
        // HOME LONGITUDE
        full_frame[start_from + count] = HOME_LONGITUDE;
        count++;
        full_frame[start_from + count] = TLV_LENGTH[HOME_LONGITUDE];
        count++;
        for (auto i = TLV_LENGTH[HOME_LONGITUDE] - 1; i >= 0; i--) {
            full_frame[start_from + count] = (get_2_complement(_home_longitude) >> (8 * i)) & 0xFF;
            count++;
        }
        // GROUND SPEED
        full_frame[start_from + count] = GROUND_SPEED;
        count++;
        full_frame[start_from + count] = TLV_LENGTH[GROUND_SPEED];
        count++;
        full_frame[start_from + count] = round(_ground_speed);
        count++;
        // HEADING
        full_frame[start_from + count] = HEADING;
        count++;
        full_frame[start_from + count] = TLV_LENGTH[HEADING];
        count++;
        for (auto i = TLV_LENGTH[HEADING] - 1; i >= 0; i--) {
            full_frame[start_from + count] = (get_2_complement(_heading) >> (8 * i)) & 0xFF;
            count++;
        }
        return count;
        // TODO: check lenght
    }

    /**
     * Sauvegarde les données de la dernière trame envoyée : Position et Temps
     */
    void set_last_send() {
        // Temps
        _last_send = std::chrono::high_resolution_clock::now();
                
        // Position
        _last_latitude_sent = _latitude;
        _last_longitude_sent = _longitude;
        _last_altitude_sent = _altitude;      
    }
    
    /**
     * Notifie si la position Home est définie
     * @return true if Home is set
     */
    bool has_home_set() const {
        return _home_set == true;
    } 
    /**
     * Notifie quand 3s sont écoulés pour envoyer une nouvelle trame.
     * @return true if elapse time is > 3s
     */
    bool has_pass_time() {
        std::chrono::duration<double> elapsed = std::chrono::high_resolution_clock::now() - _last_send;
        return elapsed.count() + _data_period_GPS >= FRAME_TIME_LIMIT;
    }
    /**
     * Notifie si le drone a bougé de plus de 30m depuis le dernier envoi,
     * avec une trajectoire rectiligne et une vitesse "rapide" l'envoi est anticipé
     * pour éviter de dépasser les 30m.
     * @return true if distance from last point sent will be > 30m
     */
    bool has_pass_distance() {         
        _predicted_distance = 0;
        
        // If enough speed, look forward for next point with estimated distance increased by 10%
        if(_ground_speed >= (FRAME_DISTANCE_LIMIT/FRAME_TIME_LIMIT)){          
          _predicted_distance = (_data_period_GPS * _ground_speed * 1.1);
        }
        
        return (_3D_distance_from_last_point_sent +_predicted_distance) >= FRAME_DISTANCE_LIMIT;
    }
    /**
     * Notifie si la condition de distance ou de temps est dépassée pour envoyer une nouvelle trame,
     * et si de nouvelles données GPS sont présentes.
     * @return
     */
    bool time_to_send() {
        bool current_status = _new_GPS_data;

        if(current_status){
          // Reset
          _new_GPS_data = false;
        }
        
        return current_status && (has_pass_time() || has_pass_distance());
    }

private:
    /**
     * Temps limite entre deux trames en s
     */
    static constexpr uint8_t FRAME_TIME_LIMIT = 3;  // in s
    /**
     * Distance limite entre deux trames en m
     */
    static constexpr double FRAME_DISTANCE_LIMIT = 30.0; // in m
    /**
     * Enumeration des types de données à envoyer
     */
    enum DATA_TYPE: uint8_t {
      RESERVED = 0,
      PROTOCOL_VERSION = 1,
      ID_FR = 2,
      ID_ANSI_CTA = 3,
      LATITUDE = 4,        // In WS84 in degree * 1e5
      LONGITUDE = 5,       // In WS84 in degree * 1e5
      ALTITUDE = 6,        // In MSL in m
      HEIGTH = 7,          // From Home in m
      HOME_LATITUDE = 8,   // In WS84 in degree * 1e5
      HOME_LONGITUDE = 9,  // In WS84 in degree * 1e5
      GROUND_SPEED = 10,   // In m/s
      HEADING = 11,        // Heading in degree from north 0 to 359.
      NOT_DEFINED_END = 12,
    };

    /**
     * Tableau TLV (TYPE, LENGTH, VALUE) avec les tailles attendu des différents type données.
     */
    static constexpr uint8_t TLV_LENGTH[] {
            0,  // [DATA_TYPE::RESERVED]
            1,  // [DATA_TYPE::PROTOCOL_VERSION]
            30, // [DATA_TYPE::ID_FR]
            0,  // [DATA_TYPE::ID_ANSI_CTA]
            4,  // [DATA_TYPE::LATITUDE]
            4,  // [DATA_TYPE::LONGITUDE]
            2,  // [DATA_TYPE::ALTITUDE]
            2,  // [DATA_TYPE::HEIGTH]
            4,  // [DATA_TYPE::HOME_LATITUDE]
            4,  // [DATA_TYPE::HOME_LONGITUDE]
            1,  // [DATA_TYPE::GROUND_SPEED]
            2,  // [DATA_TYPE::HEADING]
    };

    static constexpr uint8_t FRAME_COPTER_ID = 3;
    static constexpr uint8_t FRAME_PLANE_ID = 4;

    /**
     * Beacon frame VS:
     */
    static constexpr uint8_t FRAME_VS = 0XDD;
    /**
     * Beacon frame OUI
     */
    const uint8_t FRAME_OUI[3] = {0x6A, 0x5C, 0x35};
    /**
     * Beacon frame VS TYPE
     */
    static constexpr uint8_t FRAME_VS_TYPE = 1;

    int32_t _latitude;
    int32_t _longitude;
    int16_t _altitude;
    int16_t _height;
    int32_t _home_latitude;
    int32_t _home_longitude;
    int16_t _home_altitude;
    double _ground_speed;
    uint16_t _heading;
    uint8_t _droneID[TLV_LENGTH[ID_FR]+1]; // +1 for null termination
    std::chrono::system_clock::time_point _last_send = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point _last_data_rcv = std::chrono::system_clock::now();
    // for travelled distance calculation
    int32_t _last_latitude_sent;
    int32_t _last_longitude_sent;
    int16_t _last_altitude_sent;
    double _data_period_GPS = 0;
    double _2D_distance;
    double _3D_distance_from_last_point_sent;
    double _predicted_distance;
    bool _home_set = false;
    bool _new_GPS_data = false;

    static inline uint32_t get_2_complement(int32_t value) {
        return value & 0xFFFFFFFF;
    }
    static inline uint16_t get_2_complement(int16_t value) {
        return value & 0xFFFF;
    }

    // Taken from TinyGPS++
    /**
     * Calcule une approximation de la distance entre deux coordonnées WS84 (GPS)
     * @param lat1
     * @param long1
     * @param lat2
     * @param long2
     * @return distance en m
     */
    static double distanceBetween(double lat1, double long1, double lat2, double long2)
    {
        // returns distance in meters between two positions, both specified
        // as signed decimal-degrees latitude and longitude. Uses great-circle
        // distance computation for hypothetical sphere of radius 6372795 meters.
        // Because Earth is no exact sphere, rounding errors may be up to 0.5%.
        // Courtesy of Maarten Lamers
        double delta = radians(long1-long2);
        const double sdlong = sin(delta);
        const double cdlong = cos(delta);
        lat1 = radians(lat1);
        lat2 = radians(lat2);
        const double slat1 = sin(lat1);
        const double clat1 = cos(lat1);
        const double slat2 = sin(lat2);
        const double clat2 = cos(lat2);
        delta = (clat1 * slat2) - (slat1 * clat2 * cdlong);
        delta = sq(delta);
        delta += sq(clat2 * sdlong);
        delta = sqrt(delta);
        const double denom = (slat1 * slat2) + (clat1 * clat2 * cdlong);
        delta = atan2(delta, denom);
        return abs(delta * 6372795);
    }
};
