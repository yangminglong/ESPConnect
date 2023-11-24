#include "ESPConnect.h"
#include <esp_log.h>

#define TAG "Conn"

void ESPConnectClass::initResponse() 
{
  ESP_LOGI(TAG, "initResponse");
  _server->on("/espconnect/scan", HTTP_GET, [&](AsyncWebServerRequest *request)
  {
    m_canConnectWifi = false;
    ESP_LOGI(TAG, "api /espconnect/scan");
    String json = "[";
    int n = WiFi.scanComplete();
    if(n == WIFI_SCAN_FAILED)
    {
      WiFi.scanNetworks(true);
      return request->send(202);
    }
    else if(n == WIFI_SCAN_RUNNING)
    {
      return request->send(202);
    }
    else
    {
      for (int i = 0; i < n; ++i){
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
      WiFi.scanDelete();
      if(WiFi.scanComplete() == -2){
        WiFi.scanNetworks(true);
      }
    }
    json += "]";
    request->send(200, "application/json", json);
    json = String();
  });

  // Accept incomming WiFi Credentials
  _server->on("/espconnect/connect", HTTP_POST, [&](AsyncWebServerRequest *request)
  {
    m_canConnectWifi = false;
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
      if(ok == ESP_OK){
    #endif
        _sta_ssid = ssid;
        _sta_password = password;
        m_canConnectWifi = true;
        request->send(200, "application/json", "{\"message\":\"Credentials Saved. Rebooting...\"}");
      }else{
        ESPCONNECT_SERIAL(String("WiFi config failed with:")+String(ok)+"\n");
        return request->send(500, "application/json", "{\"message\":\"Error while saving WiFi Credentials: "+String(ok)+"\"}");
      }
  });
  
  _server->on("/espconnect", HTTP_GET, [&](AsyncWebServerRequest *request)
  {
    m_canConnectWifi = false;
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
  #endif
}


void ESPConnectClass::activeAP_DNS()
{
  ESP_LOGI(TAG, "activeAP_DNS");

  WiFi.mode(WIFI_AP_STA);
  // Start Access Point
  WiFi.softAP(_auto_connect_ssid.c_str(), _auto_connect_password.c_str());
  
  if (_dns) {
    _dns->stop();
    delete _dns;
  }

  _dns = new DNSServer();
  _dns->setErrorReplyCode(DNSReplyCode::NoError);
  _dns->start(53, "*", WiFi.softAPIP());

  m_isAPActive = true;
}

void ESPConnectClass::stopAP_DNS()
{
  ESP_LOGI(TAG, "stopAP_DNS");
  if (_dns) {
    _dns->stop();
    delete _dns;
    _dns = nullptr;
  }
  
  ESPCONNECT_SERIAL("Closed Portal\n");
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);

  if (WiFi.status() != WL_CONNECTED)
    WiFi.begin(_sta_ssid.c_str(), _sta_password.c_str());

  m_isAPActive = false;
}


void ESPConnectClass::setup(AsyncWebServer* server)
{
  ESP_LOGI(TAG, "setup");
  _server = server;

  initResponse();

  load_sta_credentials();

  uint8_t isSTAConnected = false;
  if(_sta_ssid != ""){
    WiFi.mode(WIFI_STA);
    isSTAConnected = WiFi.begin(_sta_ssid.c_str(), _sta_password.c_str()) == WL_CONNECTED || WiFi.waitForConnectResult(WIFI_CHECK_INTERVAL_LONG) == WL_CONNECTED;
  }

  if (!isSTAConnected) {
    activeAP_DNS();
  }
}

unsigned long wifiCheckInterval = WIFI_CHECK_INTERVAL_SHORT;
unsigned long wifiCheckPrevTime = 0;

unsigned long wifiReconnInterval = 20000;
unsigned long wifiReconnPrevTime = 0;

bool needActiveAP = false;
int connectedSecs = 0;

void ESPConnectClass::loop()
{
  if (_dns) {
    _dns->processNextRequest();
    yield();
  }
  if (millis() - wifiCheckPrevTime > wifiCheckInterval) {
    ESP_LOGI(TAG, "loop");
    wifiCheckPrevTime = millis();

    uint8_t statu = WiFi.status();
    if (statu == WL_CONNECTED) {
      ESP_LOGI(TAG, "is connected");
      connectedSecs++;
      if (connectedSecs > 20) {
        erase();
      }
      needActiveAP = false;
      if (m_isAPActive) {
        stopAP_DNS();
        wifiCheckInterval = WIFI_CHECK_INTERVAL_LONG;
      }
    } else if (needActiveAP && m_isAPActive == false) {
        ESP_LOGI(TAG, "reopen AP");
        needActiveAP = false;
        activeAP_DNS();
        wifiCheckInterval = WIFI_CHECK_INTERVAL_SHORT;
    } else {
      connectedSecs = 0;
      bool isSTAConnected = false;
      if (millis() - wifiReconnPrevTime > wifiReconnInterval && m_canConnectWifi) {
        ESP_LOGI(TAG, "reconnect sta.");
        wifiReconnPrevTime = millis();
        isSTAConnected = WiFi.begin(_sta_ssid.c_str(), _sta_password.c_str()) == WL_CONNECTED;
      }
      if (isSTAConnected == false && m_isAPActive == false) {
        needActiveAP = true;
        wifiCheckInterval = WIFI_CHECK_INTERVAL_LONG;
      }
    }
  }
}

/*
  Set Custom AP Credentials
*/
void ESPConnectClass::initAPInfo(const char* ssid, const char* password){
  ESP_LOGI(TAG, "initAPInfo");
  _auto_connect_ssid = ssid;
  _auto_connect_password = password;
}


/*
  Erase Stored WiFi Credentials
*/
bool ESPConnectClass::erase(){
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
    WiFi.disconnect(true, true);
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
