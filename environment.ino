// AIQ Station 
// Temperature, Humidity and Pressure
// with Lolin ESP8266
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty
    
#define SEALEVELPRESSURE_HPA (1013.25)

void mq135Callback() {
  DEBUG_PRINT("[DEBUG] MQ135 ready...");
  isMQ135 = true;
}

void envFetchData() {
  float t,p,h,volt,aq,rzero,c;
  char temp[32];

// Print I2C bus status...
// i2c_status();
   
  if(isBME) {
    bme.takeForcedMeasurement();
////// TEMPERATURE
    c=10;
    do {
      t = bme.readTemperature();
      c--;
      delay(500);
    } while(isnan(t)&&(c > 0));
   
    if(!isnan(t)) {
      lastTemp = t;
      DEBUG_PRINT("[BME280] Temp: "+String(t)+"°C");
    } else {
      DEBUG_PRINT("[BME280] Failed read temp");
    }
/////// PRESSURE
    c=10;
    do {
      p = (bme.readPressure() / 100.0F);
      c--;
      delay(500);      
    } while(isnan(p)&&(c > 0));
    
    if(!isnan(p)) {
      lastPressure = p;
      DEBUG_PRINT("[BME280] Pressure: "+String(p)+"hPa");
    } else {
      DEBUG_PRINT("[BME280] Failed read pressure");
    }

////// HUMIDITY 
    c=10;
    do {
      h = bme.readHumidity();
      c--;
      delay(500);      
    } while(isnan(h)&&(c > 0));
    
    if(!isnan(h)) {
      lastHumidity = h;
      DEBUG_PRINT("[BME280] Humidity: "+String(h)+"%");
    } else {
      DEBUG_PRINT("[BME280] Failed read humidity");
    }

/////// APPROX ALTITUDE
    
    DEBUG_PRINT("[BME280] Approx. Altitude = "+String(bme.readAltitude(SEALEVELPRESSURE_HPA))+" m");
  } else {
      // Try initialize BME280...
      isBME = BME280Init();
  }
  
  if((isMQ135)&&(!isnan(t))&&(!isnan(h))) {
    rzero = mq135.getCorrectedRZero(t,h);
    DEBUG_PRINT("[MQ135] MQ135 RZero: "+String(rzero));
    aq = mq135.getCorrectedPPM(t,h);
    DEBUG_PRINT("[MQ135] MQ135 PPM: "+String(aq));
    lastAQ = aq;
  }
}

void envCallback() {   
  DEBUG_PRINT("[DEBUG] Environment callback...");

  envFetchData();
  
  queueAddRow();    
}
