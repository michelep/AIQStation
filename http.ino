// AIQ Station 
// Temperature, Humidity and Pressure
// with Lolin ESP8266
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty


String templateProcessor(const String& var)
{
  //
  // System values
  //
  if(var == "hostname") {
    return String(config.hostname);
  }
  if(var == "fw_name") {
    return String(FW_NAME);
  }
  if(var=="fw_version") {
    return String(FW_VERSION);
  }
  if(var=="uptime") {
    return String(millis()/1000);
  }
  if(var=="timedate") {
    return String();
  }
  //
  // Config values
  //
  if(var=="wifi_essid") {
    return String(config.wifi_essid);
  }
  if(var=="wifi_password") {
    return String(config.wifi_password);
  }
  if(var=="ntp_server") {
    return String(config.ntp_server);
  }
  if(var=="ntp_timezone") {
    return String(config.ntp_timezone);
  }
  if(var=="collector_host") {
    return String(config.collector_host);
  }
  if(var=="api_key") {
    return String(config.api_key);
  }  
  if(var=="auth_username") {
    return String(config.www_username);
  }  
  if(var=="auth_password") {
    return String(config.www_password);
  }  
  return String();
}

// ************************************
// initWebServer
//
// initialize web server
// ************************************
void initWebServer() {
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html").setTemplateProcessor(templateProcessor);
  
  // If present, set authentication
  if(strlen(config.www_username) > 0) {
    // server.setAuthentication(config.www_username, config.www_password);
  }

  server.on("/ajax", HTTP_POST, [] (AsyncWebServerRequest *request) {
    String action,value,response="";
/* #ifdef __DEBUG__
    int i, params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
    }
#endif */
    if (request->hasParam("action", true)) {
      action = request->getParam("action", true)->value();
      Serial.print("ACTION: ");
      if(action.equals("get")) {
        value = request->getParam("value", true)->value();
        Serial.println(value);
        if(value.equals("temp")) {
          response = String(lastTemp);
        }
        if(value.equals("humidity")) {
          response = String(lastHumidity);
        }
        if(value.equals("pressure")) {
          response = String(lastPressure);
        }
        if(value.equals("pm25")) {
          response = String(lastPM25);
        }
        if(value.equals("pm10")) {
          response = String(lastPM10);
        }
        if(value.equals("aq")) {
          response = String(lastAQ);
        }
 #ifdef __DEBUG__
        Serial.print("[DEBUG] POST action:");
        Serial.print(action);
        Serial.print(":");
        Serial.print(value);
        Serial.print("=");
        Serial.println(response);
 #endif
      }
    }
    request->send(200, "text/plain", response);
  });
  // Add events handler
  server.addHandler(&events);

  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.printf("NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      Serial.printf("GET");
    else if(request->method() == HTTP_POST)
      Serial.printf("POST");
    else if(request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if(request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if(request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if(request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if(request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });

  server.begin();
}

