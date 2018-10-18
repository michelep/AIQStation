// AIQ Station 
// Temperature, Humidity and Pressure
// with Lolin ESP8266
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty

// ************************************
// queueAddRow()
//
// add a row to data queue
// ************************************
void queueAddRow() {
#ifdef __DEBUG__
  Serial.println("[DEBUG] queueAddRow()");
#endif    
  if(dataQueue.isInitialized()) {
    dataRow data;

    data.ts = NTP.getTime();
    strncpy(data.temp,strTemp,6);;
    strncpy(data.hum,strHumidity,6);
    strncpy(data.pres,strPressure,6);
    strncpy(data.pm10,strPM10,6);
    strncpy(data.pm25,strPM25,6);
    data.uv = UVlevel;

    dataQueue.push(&data);
#ifdef __DEBUG__
    Serial.print("[DEBUG] Items in data queue: ");
    Serial.println(dataQueue.getCount());
#endif
  } else {
#ifdef __DEBUG__
    Serial.println("[DEBUG] Queue error");
#endif
  }
}

// Queue callback: send data to server
void queueCallback() { 
  dataRow data;
  char query[64];
  String response="";
  unsigned int statusCode;
  bool r;

  if(isConnected) {
    if(dataQueue.getCount() > 0) {
      do {
        r = dataQueue.pop(&data);
        sprintf(query,"key=%s&ts=%l&t=%s&p=%s&h=%s&uv=%d&pm25=%s&pm10=%s",config.api_key,data.ts,data.temp,data.pres,data.hum,data.uv,data.pm25,data.pm10);
#ifdef __DEBUG__  
        Serial.print("[DEBUG] REST: ");
        Serial.println(query);
#endif
        statusCode = restClient->post("/rest/data/put", query, &response);
#ifdef __DEBUG__
        Serial.print("[DEBUG] Status code from server: ");
        Serial.println(statusCode);
        Serial.print("[DEBUG] Response body from server: ");
        Serial.println(response);
        if((statusCode >= 200)&&(statusCode < 300)) {
          // OK
        } else {
          // Something goes wrong: mask as not connected and force new registration
          isConnected = false;
        }
#endif
      } while(r);
    }
  } else {
    if(strlen(config.api_key) > 0) {
      sprintf(query,"key=%s&hostname=%s&fw=%s-%s",config.api_key,config.hostname,FW_NAME,FW_VERSION);

      statusCode = restClient->post("/rest/register", query, &response);
#ifdef __DEBUG__
      Serial.print("[DEBUG] Status code from server: ");
      Serial.println(statusCode);
      Serial.print("[DEBUG] Response body from server: ");
      Serial.println(response);
#endif

      if((statusCode >= 200)&&(statusCode < 300)) {
        isConnected = true; // Mark as connected
      }
    } else {
      Serial.println("[ERROR] Api key not set");
    }
  }
}

