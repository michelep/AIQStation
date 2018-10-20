// AIQ Station 
// Temperature, Humidity and Pressure
// with Lolin ESP8266
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty
    
#define SEALEVELPRESSURE_HPA (1013.25)

// Define minimum delta to detect a value change
#define DELTA_T 0.1
#define DELTA_H 0.1
#define DELTA_P 1

void envCallback() {   
  float t,p,h,volt;
  char temp[32];
  bool isChange=false;
   
  if(isBME) {
////// TEMPERATURE
    t = bme.readTemperature();
    if(!isnan(t)) {
        if(abs(t-lastTemp) > DELTA_T) { // Temp change
          lastTemp = t;
          isChange=true;
        }
    }
#ifdef __DEBUG__
    Serial.print("[DEBUG] Temp: ");
    Serial.print(t);
    Serial.println("°C");
#endif
/////// PRESSURE
    p = (bme.readPressure() / 100.0F);
    if(!isnan(p)) {
        if(abs(p-lastPressure) > DELTA_P) { // Pressure change
          lastPressure = p;
          isChange=true;
        }
    }

#ifdef __DEBUG__
    Serial.print("[DEBUG] Pressure: ");
    Serial.print(p);
    Serial.println("hPa");
#endif
////// HUMIDITY  
    h = bme.readHumidity();
    if(!isnan(h)) {
        if(abs(h-lastHumidity) > DELTA_H) { // Humidity change
          lastHumidity = h;
          isChange=true;
        }
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

  if(isUV) {
    // Read analog value from UV sensor
    UVlevel = analogRead(A0);
    // UVLevel 0-1V 
    // <50mV      = 0
    // 50  < 227  = 1
    // 227 < 318  = 2
    // 318 < 408  = 3
    // 408 < 503  = 4
    // 503 < 606  = 5
    // 606 < 696  = 6
    // 696 < 795  = 7
    // 795 < 881  = 8
    // 976 < 1079 = 9
    // 1079< 1170 = 10
    // >1170      = 11+
#ifdef __DEBUG__
    Serial.print("[DEBUG] UV value: ");
    Serial.print(UVlevel);
    Serial.println(" mV");
#endif 
  }
  
  // ADC read
  adc0 = ads.readADC_SingleEnded(0);
  adc1 = ads.readADC_SingleEnded(1);
  adc2 = ads.readADC_SingleEnded(2);
  adc3 = ads.readADC_SingleEnded(3);
  
  // Convert to voltage
#ifdef __DEBUG__
  volt = (adc0 * 0.1875)/1000;
  Serial.print("[DEBUG] AIN0: "); Serial.print(volt); Serial.println("v");
  volt = (adc1 * 0.1875)/1000;
  Serial.print("[DEBUG] AIN1: "); Serial.print(volt); Serial.println("v");
  volt = (adc2 * 0.1875)/1000;
  Serial.print("[DEBUG] AIN2: "); Serial.print(volt); Serial.println("v");
  volt = (adc3 * 0.1875)/1000;
  Serial.print("[DEBUG] AIN3: "); Serial.print(volt); Serial.println("v");
#endif

  // If something change, enqueue data for collector server
  if(isChange) {
    queueAddRow();    
    isChange = false;
    dataWd = MAX_DATAWD;
  }
}
