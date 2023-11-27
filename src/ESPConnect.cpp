#include "ESPConnect.h"
#include <esp_log.h>
#define TAG "Conn"

unsigned long wifiCheckInterval = WIFI_CHECK_INTERVAL_SHORT;
unsigned long wifiCheckPrevTime = 0;

unsigned long wifiReconnInterval = 20000;
unsigned long wifiReconnPrevTime = 0;


void ESPConnectClass::initResponse() 
{
  ESP_LOGI(TAG, "initResponse");
  _server->on("/espconnect/scan", HTTP_GET, [&](AsyncWebServerRequest *request)
  {
    m_timeOfLastScan = millis();

    int n = WiFi.scanComplete();
    if(n == WIFI_SCAN_FAILED){
      m_scanning = true;
      WiFi.scanNetworks(true, false, true, 200); // WiFi.scanNetworks(true) 会遇到AP自动掉线的bug
      return request->send(202);
    }else if(n == WIFI_SCAN_RUNNING){
      return request->send(202);
    } else {
      String json = "[";
      for (int i = 0; i < n; ++i) {
        String ssid = WiFi.SSID(i);
        // Escape invalid characters
        ssid.replace("\\","\\\\");
        ssid.replace("\"","\\\"");
        json+="{";
        json+="\"name\":\""+ssid+"\",";
        #if defined(ESP8266)
          json+="\"open\":"+String(WiFi.encryptionType(i) == ENC_TYPE_NONE ? "true": "false");
        #elif defined(ESP32)
          json+="\"open\":"+String(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "true": "false");
        #endif
        json+="}";
        if(i != n-1) json+=",";
      }
      json += "]";

      WiFi.scanDelete();
      ESP_LOGI(TAG, "api /espconnect/scan -- scanning finished%d", millis()-m_timeOfLastScan);
      m_timeOfLastScan = std::numeric_limits<unsigned long>::max();
      m_scanning = false;
      request->send(200, "application/json", json);
    }
  });

  // Accept incomming WiFi Credentials
  _server->on("/espconnect/connect", HTTP_POST, [&](AsyncWebServerRequest *request)
  {
    ESP_LOGI(TAG, "api /espconnect/connect");
    // Get FormData
    String ssid = request->hasParam("ssid", true) ? request->getParam("ssid", true)->value().c_str() : "";
    String password = request->hasParam("password", true) ? request->getParam("password", true)->value().c_str() : "";

    if(ssid.length() <= 0){
      return request->send(403, "application/json", "{\"message\":\"Invalid SSID\"}");
    }

    if(ssid.length() > 32 || password.length() > 64){
      return request->send(403, "application/json", "{\"message\":\"Credentials exceed character limit of 32 & 64 respectively.\"}");
    }
    
      // erase();
    // Save WiFi Credentials in Flash
    int ok = 0;
    #if defined(ESP8266)
      struct station_config	config = {};
      ssid.toCharArray((char*)config.ssid, sizeof(config.ssid));
      password.toCharArray((char*)config.password, sizeof(config.password));
      config.bssid_set = false;
      ok = wifi_station_set_config(&config);
    #elif defined(ESP32)
      Preferences preferences;
      preferences.begin("espconnect", false);
      preferences.putString("ssid", ssid.c_str());
      preferences.putString("password", password.c_str());
      preferences.end();
    #endif

    #if defined(ESP8266)
      if(ok == 1){
    #elif defined(ESP32)
      if(ok == ESP_OK) {
    #endif

        request->send(202, "application/json", "{\"message\":\"Credentials Saved. Connecting...\"}");

        // WiFi.disconnect(false);
        m_connectReq = true;
        _sta_ssid = ssid;
        _sta_password = password;
        m_timeOfWiFiConnected = std::numeric_limits<unsigned long>::max();
        wifiCheckInterval = WIFI_CHECK_INTERVAL_SHORT; 
        wifiReconnPrevTime = 0;
        wifiCheckPrevTime = 0; // check Wifi state, now !
      }else{
        ESPCONNECT_SERIAL(String("WiFi config failed with:")+String(ok)+"\n");
        return request->send(500, "application/json", "{\"message\":\"Error while saving WiFi Credentials: "+String(ok)+"\"}");
      }
  });

  _server->on("/espconnect/STAState", HTTP_GET, [&](AsyncWebServerRequest *request)
  {
    ESP_LOGI(TAG, "api /espconnect/STAState");
    // STA Info
    uint8_t statu = WiFi.status(); // Get WiFi Statu

    if (statu == WL_CONNECTED) { // WiFi has connected
      String staIP = WiFi.localIP().toString();
      String info = String("{\"staIP\":\"") +staIP+ "\"}";
      ESP_LOGI(TAG, "STA connected:", staIP.c_str());
      return request->send(200, "application/json", info.c_str());
    } else {
      ESP_LOGI(TAG, "Get WiFi State: %d", statu);
      return request->send(202, "application/json", "{}");
    }
  });
  
  _server->on("/espconnect/erase", HTTP_GET, [&](AsyncWebServerRequest *request)
  {
    ESP_LOGI(TAG, "api /espconnect/erase");
    request->send(200);

    erase();
    WiFi.disconnect(false);
    m_needActiveAP = true;
    wifiCheckPrevTime = 0; // check Wifi state, now !
  });
  
  _server->on("/espconnect", HTTP_GET, [&](AsyncWebServerRequest *request)
  {
    ESP_LOGI(TAG, "api /espconnect");
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", ESPCONNECT_HTML, ESPCONNECT_HTML_SIZE);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
}

/* 
  Loads STA Credentials into memory
*/

void ESPConnectClass::load_sta_credentials()
{
  ESP_LOGI(TAG, "load_sta_credentials");
  // Get saved WiFi Credentials
  #if defined(ESP8266)
    station_config config = {};
    wifi_station_get_config(&config);
    for(int i=0; i < strlen((char*)config.ssid); i++){
      _sta_ssid += (char)config.ssid[i];
    }
    for(int i=0; i < strlen((char*)config.password); i++){
      _sta_password += (char)config.password[i];
    }
  #elif defined(ESP32)
    Preferences preferences;
    preferences.begin("espconnect", false);
    _sta_ssid = preferences.getString("ssid", "");
    _sta_password = preferences.getString("password", "");
    preferences.end();

    ESP_LOGI(TAG, "load ssid:%s, pswd:%s", _sta_ssid.c_str(), _sta_password.c_str());
  #endif
}


void ESPConnectClass::activeAP_DNS()
{
  ESP_LOGI(TAG, "activeAP_DNS");

  WiFi.mode(WIFI_AP_STA);
  // Start Access Point
  WiFi.softAP(_auto_connect_ssid.c_str(), _auto_connect_password.c_str());
  
  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  WiFi.setSleep(false);

  // if (_dns) {
  //   _dns->stop();
  //   delete _dns;
  // }

  // _dns = new DNSServer();
  // _dns->setErrorReplyCode(DNSReplyCode::NoError);
  // _dns->start(53, "*", WiFi.softAPIP());

  m_isAPActive = true;
}

void ESPConnectClass::stopAP_DNS()
{
  ESP_LOGI(TAG, "stopAP_DNS");
  // if (_dns) {
  //   _dns->stop();
  //   delete _dns;
  //   _dns = nullptr;
  // }
  
  ESPCONNECT_SERIAL("Closed Portal\n");
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);

  m_isAPActive = false;
}


void ESPConnectClass::setup(AsyncWebServer* server)
{
  ESP_LOGI(TAG, "setup");
  _server = server;

  WiFi.onEvent([this](arduino_event_id_t event, arduino_event_info_t info){
    switch(event) {
        case ARDUINO_EVENT_WIFI_STA_START: 
          break;
        case ARDUINO_EVENT_WIFI_STA_STOP: 
          break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED: 
          m_timeOfWiFiConnected = millis();
          break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: 
          break;
        case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE: 
          break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP: 
          break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6: 
          break;
        case ARDUINO_EVENT_WIFI_STA_LOST_IP: 
          break;
    }
  });

  initResponse();

  m_timeOfLastScan = std::numeric_limits<unsigned long>::max();
  m_timeOfWiFiConnected = std::numeric_limits<unsigned long>::max();

  load_sta_credentials();

  uint8_t isSTAConnected = false;
  if(_sta_ssid.length()){
    ESP_LOGI(TAG, "has _sta_ssid, try connect STA");
    WiFi.mode(WIFI_STA);
    isSTAConnected = WiFi.begin(_sta_ssid.c_str(), _sta_password.c_str()) == WL_CONNECTED || WiFi.waitForConnectResult(WIFI_CHECK_INTERVAL_LONG) == WL_CONNECTED;
  }

  if (!isSTAConnected) {
    activeAP_DNS();
  } else {
    String staIP = WiFi.localIP().toString();
    ESP_LOGI(TAG, "STA connected:", staIP.c_str());
  }
}



void ESPConnectClass::loop()
{
  // if (_dns) {
  //   _dns->processNextRequest();
  //   yield();
  // }

  if (!m_scanning && millis()-wifiCheckPrevTime > wifiCheckInterval) {
    wifiCheckPrevTime = millis();

    uint8_t statu = WiFi.status(); // Get WiFi Statu

    if (statu == WL_CONNECTED) { // WiFi has connected
      ESP_LOGI(TAG, "is connected");
      m_needActiveAP = false;
      if (m_isAPActive && millis()-m_timeOfWiFiConnected > STOP_AP_INTERVAL) { // stop ap
        stopAP_DNS();
        wifiCheckInterval = WIFI_CHECK_INTERVAL_LONG;
      }

    } else if (m_needActiveAP && m_isAPActive == false) { // active AP if need
        ESP_LOGI(TAG, "reopen AP");
        m_needActiveAP = false;
        activeAP_DNS();
    } else { // WiFi not connected
      ESP_LOGI(TAG, "WiFi not connected");

      if (m_connectReq) { 
        // 执行 WiFi.scanNetworks(true, false, true, 200); 后, 再执行WiFi.begin出现异常，无法连接。但是重新执行WiFi.scanNetworks(true);恢复正常
        int n = WiFi.scanComplete();
        if(n == WIFI_SCAN_FAILED){
          WiFi.scanNetworks(true); // WiFi.scanNetworks(true) 会遇到AP自动掉线的bug
          return;
        } 
        // else if(n == WIFI_SCAN_RUNNING){
        //   return;
        // } 
        else {
          m_connectReq = false;
          WiFi.scanDelete();
        }
      }

      if (millis()-wifiReconnPrevTime > wifiReconnInterval && !m_scanning && _sta_ssid.length()) 
      {
        wifiReconnPrevTime = millis();
        //int n = WiFi.scanComplete();
        ESP_LOGI(TAG, "WiFi begin: ssid:%s, pswd:%s", _sta_ssid.c_str(), _sta_password.c_str());
        if (WL_CONNECT_FAILED == WiFi.begin(_sta_ssid.c_str(), _sta_password.c_str()))
        {
          ESP_LOGI(TAG, "WiFi begin: failed");
        }
      }

      if (!WiFi.isConnected() && m_isAPActive == false) { // Active AP after a moment if WiFi not connected
        ESP_LOGI(TAG, "needActiveAP");
        m_needActiveAP = true;
        wifiCheckInterval = WIFI_CHECK_INTERVAL_SHORT;
      }
    }
  }

  // if (millis() - m_timeOfLastScan > 10000 && m_scanning) {
  //   ESP_LOGI(TAG, "time to scanDelete");
  //   WiFi.scanDelete();
  //   m_timeOfLastScan = std::numeric_limits<unsigned long>::max();
  //   m_scanning = false;
  // }
}

/*
  Set Custom AP Credentials
*/
void ESPConnectClass::initAPInfo(const char* ssid, const char* password){
  ESP_LOGI(TAG, "initAPInfo");
  _auto_connect_ssid = ssid;
  _auto_connect_password = password;
}

#include <esp_wifi.h>

/*
  Erase Stored WiFi Credentials
*/
bool ESPConnectClass::erase(){
  ESP_LOGI(TAG, "erase");
  #if defined(ESP8266)
// -----------  Added this section to erase stored wifi credentials - RLB -20220318  --------
    int ok = 0;
    struct station_config	config = {};
    memset(&config.ssid, 0, sizeof(config.ssid));
    memset(&config.password, 0, sizeof(config.password));
    config.bssid_set = false;
    ok = wifi_station_set_config(&config);
// -----------------------------------------------------------------------------------------
    return WiFi.disconnect(true);
  #elif defined(ESP32)
    Preferences preferences;
    preferences.begin("espconnect", false);
    preferences.putString("ssid", "");
    preferences.putString("password", "");
    preferences.end();
    _sta_ssid = "";
    _sta_password = "";
    wifi_config_t conf;
    if (esp_wifi_get_config((wifi_interface_t)ESP_IF_WIFI_STA, &conf) == ESP_OK) 
    {
      memset(conf.sta.ssid, 0, sizeof(conf.sta.ssid)) ;
      memset(conf.sta.password, 0, sizeof(conf.sta.password)) ;
      ESP_LOGI(TAG, "erase nvs ssid:%d size, pswd:%d size", sizeof(conf.sta.ssid), sizeof(conf.sta.password));
      if(esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_STA, &conf) == ESP_OK){
        ESP_LOGI(TAG, "reset nvs");
      }
    }
    // WiFi.disconnect(true, true);    
    return true;
  #endif
}


/*
  Return Connection Status
*/
bool ESPConnectClass::isConnected(){
  return (WiFi.status() == WL_CONNECTED);
}

String ESPConnectClass::getSSID(){
  return _sta_ssid;
}

String ESPConnectClass::getPassword(){
  return _sta_password;
}

ESPConnectClass ESPConnect;
