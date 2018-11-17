// AIQ Station 
// Temperature, Humidity and Pressure
// with Lolin ESP8266
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty

// *******************************
// SDS callback
//
// Wake up SDS, acquire data and save it
// *******************************
void sdsCallback() {
  uint8_t timeout=0;
  int error;
  float pm10, pm25;
#ifdef __DEBUG__
  Serial.println("[DEBUG] sdsCallback()");
#endif  
  if(!sdsIsWarmup) {
    // Wake up, SDS !
    sds.wakeup();
    // Wait 30 seconds before reading values...
    sdsTask.setInterval(30000); /* 30 secondi to warm up */
    // Set true
    sdsIsWarmup = true;
#ifdef __DEBUG__
  Serial.println("[DEBUG] SDS011 is warming... back in 10 seconds...");
#endif  
  } else {
    while((error)&&(timeout < 10)) {
      delay(500);
      error = sds.read(&pm25, &pm10);

      if(!error) {   
        lastPM25 = pm25;
        lastPM10 = pm10;
#ifdef __DEBUG__
        Serial.print("[DEBUG] P2.5: ");
        Serial.println(String(pm25));
        Serial.print("[DEBUG] P10: ");
        Serial.println(String(pm10));
#endif 
        sdsCount++;
      } else {
#ifdef __DEBUG__
        Serial.println("[ERROR] SDS010 error");
#endif
      }
      timeout++;
    }
    // Go back to sleep...
#ifdef __DEBUG__
    Serial.println("[DEBUG] SDS011 sleeping");
#endif  
    sds.sleep();
    // Restore original interval
    sdsTask.setInterval(SDS_INTERVAL);
    sdsIsWarmup=false;
  }
}
