#ifndef ESPConnect_h
#define ESPConnect_h


#include <functional>
#include <Arduino.h>

#if defined(ESP8266)
  #include "ESP8266WiFi.h"
  #include "WiFiClient.h"
  #include "ESPAsyncTCP.h"
#elif defined(ESP32)
  #include "WiFi.h"
  #include "WiFiClient.h"
  #include "AsyncTCP.h"
  #include "Preferences.h"
#endif

#include "ESPAsyncWebServer.h"
#include "DNSServer.h"
#include "espconnect_webpage.h"
#include <functional>

/* Library Default Settings */
#define ESPCONNECT_DEBUG 0


#if ESPCONNECT_DEBUG == 1
  #define ESPCONNECT_SERIAL(x) Serial.print("[ESPConnect]["+String(millis())+"] "+x)
  #define ESPCONNECT_SERIALP(x) Serial.print(x)
#else
  #define ESPCONNECT_SERIAL(x)
  #define ESPCONNECT_SERIALP(x)
#endif

#define DEFAULT_AP_SSID "AP_12345678"
#define DEFAULT_AP_PSWD ""

#define WIFI_CHECK_INTERVAL_SHORT 1000
#define WIFI_CHECK_INTERVAL_LONG 3000

class ESPConnectClass {

  private:
    DNSServer* _dns = nullptr;
    AsyncWebServer* _server = nullptr;

    String _auto_connect_ssid = DEFAULT_AP_SSID;
    String _auto_connect_password = DEFAULT_AP_PSWD;

    String _sta_ssid = "";
    String _sta_password = "";

    bool m_isAPActive = false;
    bool m_canConnectWifi = true;
    uint8_t m_oldStatu = WL_DISCONNECTED;

  private:
    void load_sta_credentials();

    void initResponse();

    void activeAP_DNS();

    void stopAP_DNS();
  public:
    // Set Custom AP
    void initAPInfo(const char* ssid = DEFAULT_AP_SSID, const char* password = DEFAULT_AP_PSWD);

    void setup(AsyncWebServer* server);

    void loop();

    // Erase Saved WiFi Credentials
    bool erase();

    /*
      Data Getters
    */

    // Return true / false depending of connection status
    bool isConnected();

    // Gets SSID of connected endpoint
    String getSSID();
    String getPassword();
};

extern ESPConnectClass ESPConnect;

#endif