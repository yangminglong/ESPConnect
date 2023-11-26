/*
  --------------------------------
  ESPConnect - AutoConnect Example
  --------------------------------

  Initializes ESPConnect and attaches itself to AsyncWebServer.
  
  Github & WiKi: https://github.com/ayushsharma82/ESPConnect
  Works with both ESP8266 & ESP32
*/

#include <Arduino.h>

#if defined(ESP8266)
  /* ESP8266 Dependencies */
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #include <ESPAsyncWebServer.h>
#elif defined(ESP32)
  /* ESP32 Dependencies */
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #include <esp_log.h>
#endif
#include <ESPConnect.h>

#define TAG "main"
AsyncWebServer server(80);

String eventName(arduino_event_id_t id) {
    switch(id) {
        case ARDUINO_EVENT_WIFI_READY: return "WIFI_READY";
        case ARDUINO_EVENT_WIFI_SCAN_DONE: return "SCAN_DONE";
        case ARDUINO_EVENT_WIFI_STA_START: return "STA_START";
        case ARDUINO_EVENT_WIFI_STA_STOP: return "STA_STOP";
        case ARDUINO_EVENT_WIFI_STA_CONNECTED: return "STA_CONNECTED";
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: return "STA_DISCONNECTED";
        case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE: return "STA_AUTHMODE_CHANGE";
        case ARDUINO_EVENT_WIFI_STA_GOT_IP: return "STA_GOT_IP";
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6: return "STA_GOT_IP6";
        case ARDUINO_EVENT_WIFI_STA_LOST_IP: return "STA_LOST_IP";
        case ARDUINO_EVENT_WIFI_AP_START: return "AP_START";
        case ARDUINO_EVENT_WIFI_AP_STOP: return "AP_STOP";
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED: return "AP_STACONNECTED";
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: return "AP_STADISCONNECTED";
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED: return "AP_STAIPASSIGNED";
        case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED: return "AP_PROBEREQRECVED";
        case ARDUINO_EVENT_WIFI_AP_GOT_IP6: return "AP_GOT_IP6";
        case ARDUINO_EVENT_WIFI_FTM_REPORT: return "FTM_REPORT";
        case ARDUINO_EVENT_ETH_START: return "ETH_START";
        case ARDUINO_EVENT_ETH_STOP: return "ETH_STOP";
        case ARDUINO_EVENT_ETH_CONNECTED: return "ETH_CONNECTED";
        case ARDUINO_EVENT_ETH_DISCONNECTED: return "ETH_DISCONNECTED";
        case ARDUINO_EVENT_ETH_GOT_IP: return "ETH_GOT_IP";
        //case ARDUINO_EVENT_ETH_LOST_IP: return "ETH_LOST_IP";
        case ARDUINO_EVENT_ETH_GOT_IP6: return "ETH_GOT_IP6";
        case ARDUINO_EVENT_WPS_ER_SUCCESS: return "WPS_ER_SUCCESS";
        case ARDUINO_EVENT_WPS_ER_FAILED: return "WPS_ER_FAILED";
        case ARDUINO_EVENT_WPS_ER_TIMEOUT: return "WPS_ER_TIMEOUT";
        case ARDUINO_EVENT_WPS_ER_PIN: return "WPS_ER_PIN";
        case ARDUINO_EVENT_WPS_ER_PBC_OVERLAP: return "WPS_ER_PBC_OVERLAP";
        case ARDUINO_EVENT_SC_SCAN_DONE: return "SC_SCAN_DONE";
        case ARDUINO_EVENT_SC_FOUND_CHANNEL: return "SC_FOUND_CHANNEL";
        case ARDUINO_EVENT_SC_GOT_SSID_PSWD: return "SC_GOT_SSID_PSWD";
        case ARDUINO_EVENT_SC_SEND_ACK_DONE: return "SC_SEND_ACK_DONE";
        case ARDUINO_EVENT_PROV_INIT: return "PROV_INIT";
        case ARDUINO_EVENT_PROV_DEINIT: return "PROV_DEINIT";
        case ARDUINO_EVENT_PROV_START: return "PROV_START";
        case ARDUINO_EVENT_PROV_END: return "PROV_END";
        case ARDUINO_EVENT_PROV_CRED_RECV: return "PROV_CRED_RECV";
        case ARDUINO_EVENT_PROV_CRED_FAIL: return "PROV_CRED_FAIL";
        case ARDUINO_EVENT_PROV_CRED_SUCCESS: return "PROV_CRED_SUCCESS";
        default: return "Unkonw ID" + String(id);
    }
}

void setup() { 
  ESP_LOGI(TAG, "Setup..");

  delay(1000);

  WiFi.onEvent([](arduino_event_id_t event, arduino_event_info_t info){
    ESP_LOGI(TAG, "Event:", eventName(event));
  });
  /*
    AutoConnect AP
    Configure SSID and password for Captive Portal
  */
  ESPConnect.initAPInfo("AP_ESPConfig");

  ESPConnect.setup(&server);

  server.on("/", HTTP_GET, [&](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Hello from ESP, Accessing /espconnect to start configuring WIFI.");
  });

  server.begin();
}

void loop(){ 
  ESPConnect.loop();
}