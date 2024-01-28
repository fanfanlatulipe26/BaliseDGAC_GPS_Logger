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
/*------------------------------------------------------------------------------
*/
// classe héritée de WebServer pour utiliser chunkedResponseModeStart & Co
// Utile uniquement pour ESP32 car chunkedResponseModeStart existe uniquement dans le package ESP8266.
// Demande faite de l'inclure dans le SDK ESP32
// https://github.com/espressif/arduino-esp32/issues/5080
#ifndef FS_WEBSERVER_H
#define FS_WEBSERVER_H

#include <WebServer.h>
class fs_WebServer : public WebServer
{
  public:
    fs_WebServer(int port = 80): WebServer(port) { }
    fs_WebServer(IPAddress addr, int port): WebServer(addr, port){ }
    using WebServer:: send_P;
    void send_P(int code, PGM_P content_type, PGM_P content) {
      size_t contentLength = 0;
      if (content != NULL) {
        contentLength = strlen_P(content);
      }
      String header;
      char type[64];
      memccpy_P((void*)type, (PGM_VOID_P)content_type, 0, sizeof(type));
      _prepareHeader(header, code, (const char* )type, contentLength);
      _currentClientWrite(header.c_str(), header.length());
      if (contentLength) {  // if rajouté par FS ...........................+++++
        sendContent_P(content);
      }
    }

    bool chunkedResponseModeStart_P (int code, PGM_P content_type) {
      if (_currentVersion == 0)
        // no chunk mode in HTTP/1.0
        return false;
      setContentLength(CONTENT_LENGTH_UNKNOWN);
      send_P(code, content_type, "");
      return true;
    }
    bool chunkedResponseModeStart (int code, const char* content_type) {
      return chunkedResponseModeStart_P(code, content_type);
    }
    bool chunkedResponseModeStart (int code, const String& content_type) {
      return chunkedResponseModeStart_P(code, content_type.c_str());
    }
    void chunkedResponseFinalize () {
      sendContent(emptyString);
    }
};
#endif // FS_WEBSERVER_H
