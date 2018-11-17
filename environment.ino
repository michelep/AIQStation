// AIQ Station 
// Temperature, Humidity and Pressure
// with Lolin ESP8266
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty
    
#define SEALEVELPRESSURE_HPA (1013.25)

void mq135Callback() {
#ifdef __DEBUG__
  Serial.println("[DEBUG] MQ135 ready...");
#endif
  isMQ135 = true;
}

void envCallback() {   
  float t,p,h,volt,aq,rzero;
  char temp[32];
   
  if(isBME) {
////// TEMPERATURE
    t = bme.readTemperature();
    if(!isnan(t)) {
      lastTemp = t;
    }
#ifdef __DEBUG__
    Serial.print("[DEBUG] Temp: ");
    Serial.print(t);
    Serial.println("°C");
#endif
/////// PRESSURE
    p = (bme.readPressure() / 100.0F);
    if(!isnan(p)) {
      lastPressure = p;
    }

#ifdef __DEBUG__
    Serial.print("[DEBUG] Pressure: ");
    Serial.print(p);
    Serial.println("hPa");
#endif
////// HUMIDITY  
    h = bme.readHumidity();
    if(!isnan(h)) {
      lastHumidity = h;
    }

#ifdef __DEBUG__
    Serial.print("[DEBUG] Humidity: ");
    Serial.print(h);
    Serial.println("%");
#endif
/////// APPROX ALTITUDE
    
#ifdef __DEBUG__
    Serial.print("[DEBUG] Approx. Altitude = ");
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");
#endif 
  }
  
  if((isMQ135)&&(!isnan(t))&&(!isnan(h))) {
#ifdef __DEBUG__
    Serial.print("[DEBUG] MQ135 RZero: ");
#endif
    rzero = mq135.getCorrectedRZero(t,h);
#ifdef __DEBUG__
    Serial.println(rzero);
    Serial.print("[DEBUG] MQ135 PPM: ");
#endif    
    aq = mq135.getCorrectedPPM(t,h);
#ifdef __DEBUG__
    Serial.println(aq);
#endif
    lastAQ = aq;
  }

  // If something change, enqueue data for collector server
  queueAddRow();    
}
