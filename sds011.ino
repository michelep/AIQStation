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
  
  DEBUG_PRINT("[DEBUG] sdsCallback()");
  
  if(!sdsIsWarmup) {
    // Wake up, SDS !
    sds.wakeup();
    // Wait 30 seconds before reading values...
    sdsTask.setInterval(30000); /* 30 secondi to warm up */
    // Set true
    sdsIsWarmup = true;

    DEBUG_PRINT("[SDS011] Warming...");
  } else {
    while((error)&&(timeout < 10)) {
      delay(500);
      error = sds.read(&pm25, &pm10);

      if(!error) {   
        lastPM25 = pm25;
        lastPM10 = pm10;
        DEBUG_PRINT("[SDS011] PM2.5:"+String(pm25)+" PM10:"+String(pm10));
        sdsCount++;
      } else {
        DEBUG_PRINT("[SDS011] SDS010 error");
      }
      timeout++;
    }
    // Go back to sleep...
    DEBUG_PRINT("[SDS011] Sleeping");
    sds.sleep();
    // Restore original interval
    sdsTask.setInterval(SDS_INTERVAL);
    sdsIsWarmup=false;
  }
}
