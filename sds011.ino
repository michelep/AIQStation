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
  uint8_t c,timeout=0;
  float p10, p25;
#ifdef __DEBUG__
  Serial.println("[DEBUG] sdsCallback()");
#endif  
  if(!sdsIsWarmup) {
    // Wake up, SDS !
    sds.wakeup();
    // Wait 10 seconds before reading values...
    sdsTask.setInterval(10000); /* 10 secondi to warm up */
    // Set true
    sdsIsWarmup = true;
#ifdef __DEBUG__
  Serial.println("[DEBUG] SDS011 is warming... back in 10 seconds...");
#endif  
  } else {
    while((c)&&(timeout < 10)) {
      delay(500);
      c = sds.read(&p25, &p10);
      if(!c) {   
        dtostrf(p10,3,1,strPM10);
        dtostrf(p25,3,1,strPM25);
#ifdef __DEBUG__
        Serial.print("[DEBUG] P2.5: " + String(p25));
        Serial.println("[DEBUG] P10:  " + String(p10));
#endif 
        // Save last valid acquirement
        sdsLast = millis();
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
