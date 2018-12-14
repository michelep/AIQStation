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
  DEBUG_PRINT("[DEBUG] queueAddRow()");

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

    DEBUG_PRINT("[QUEUE] Items in data queue: "+String(dataQueue.getCount()));
  } else {
    DEBUG_PRINT("[QUEUE] Queue error");
  }
}

// Queue callback: send data to server
void queueCallback() { 
  dataRow data;
  char query[128];
  String response="";
  unsigned int statusCode;
  bool r;

  RestClient restClient = RestClient(config.collector_host); 

  restClient.setContentType("application/x-www-form-urlencoded");

  DEBUG_PRINT("[DEBUG] queueCallback()");

  if(isConnected) {
    if((dataQueue.isInitialized()) && (dataQueue.getCount() > 0)) {
      do {
        r = dataQueue.pop(&data);
        if(r) {
          sprintf(query,"key=%s&ts=%d&t=%s&p=%s&h=%s&aq=%s&pm25=%s&pm10=%s",config.api_key,data.ts,data.temp,data.pres,data.hum,data.aq,data.pm25,data.pm10);
          DEBUG_PRINT("[QUEUE] REST: "+String(query));

          statusCode = restClient.post("/rest/data/put", query, &response);

          DEBUG_PRINT("[QUEUE] Status code from server: "+String(statusCode));
          DEBUG_PRINT("[QUEUE] Response body from server: "+String(response));
          if((statusCode >= 200)&&(statusCode < 300)) {
            // OK
            dataCounter++;
            lastDataTS = now();
          } else {
            // Something goes wrong: mark as not connected to force new registration next loop
            isConnected = false;
            dataErrorCounter++;
            break;
          }
        }
      } while(r);
    }
  } else {
    if(strlen(config.api_key) > 0) {
      sprintf(query,"key=%s&hostname=%s&fw=%s-%s",config.api_key,config.hostname,FW_NAME,FW_VERSION);

      DEBUG_PRINT("[QUEUE] REST: "+String(query));

      statusCode = restClient.post("/rest/register", query, &response);

      DEBUG_PRINT("[QUEUE] Status code from server: "+String(statusCode));
      DEBUG_PRINT("[QUEUE] Response body from server: "+String(response));

      if((statusCode >= 200)&&(statusCode < 300)) {
        isConnected = true; // Mark as connected
      }
    } else {
      DEBUG_PRINT("[QUEUE] Api key not set");
    }
  }
}
