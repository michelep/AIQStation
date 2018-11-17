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
    dtostrf(lastTemp, 1, 2, data.temp);
    dtostrf(lastHumidity, 1, 2, data.hum);
    dtostrf(lastPressure, 1, 2, data.pres);
    dtostrf(lastPM10, 1, 2, data.pm10);
    dtostrf(lastPM25, 1, 2, data.pm25);
    dtostrf(lastAQ, 1, 2, data.aq);

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
  char query[128];
  String response="";
  unsigned int statusCode;
  bool r;
#ifdef __DEBUG__
  Serial.println("[DEBUG] queueCallback()");
#endif      

  if(isConnected) {
    if((dataQueue.isInitialized()) && (dataQueue.getCount() > 0)) {
      do {
        r = dataQueue.pop(&data);
        if(r) {
          sprintf(query,"key=%s&ts=%d&t=%s&p=%s&h=%s&aq=%s&pm25=%s&pm10=%s",config.api_key,data.ts,data.temp,data.pres,data.hum,data.aq,data.pm25,data.pm10);
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
            // Something goes wrong: mark as not connected to force new registration next loop
            isConnected = false;
            break;
          }
        }
#endif
      } while(r);
    }
  } else {
    if(strlen(config.api_key) > 0) {
      sprintf(query,"key=%s&hostname=%s&fw=%s-%s",config.api_key,config.hostname,FW_NAME,FW_VERSION);

#ifdef __DEBUG__  
      Serial.print("[DEBUG] REST: ");
      Serial.println(query);
#endif

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

