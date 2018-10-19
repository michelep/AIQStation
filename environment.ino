// AIQ Station 
// Temperature, Humidity and Pressure
// with Lolin ESP8266
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty
    
#define SEALEVELPRESSURE_HPA (1013.25)

void envCallback() {   
  int16_t adc0, adc1, adc2, adc3;
  float t,h,p;
  char temp[32];
   
  if(isBME) {
    t = bme.readTemperature();
    p = bme.readPressure();
    h = bme.readHumidity();
    
    if(!isnan(t)&&!isnan(p)&&!isnan(h)) {   
      // char *   dtostrf (double __val, signed char __width, unsigned char __prec, char *__s)
          
      dtostrf(t,6,2,strTemp);
      dtostrf((p / 100.0F), 6, 2, strPressure);
      dtostrf(h,6,2,strHumidity);
#ifdef __DEBUG__
      Serial.print("[DEBUG] Temp: ");
      Serial.print(strTemp);
      Serial.println("°C");
      Serial.print("[DEBUG] Pressure: ");
      Serial.print(strPressure);
      Serial.println("hPa");
      Serial.print("[DEBUG] Humidity: ");
      Serial.print(strHumidity);
      Serial.println("%");
      Serial.print("[DEBUG] Approx. Altitude = ");
      Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
      Serial.println(" m");
#endif 
      // Clear line
      lcd.clearLine(2);
      lcd.clearLine(3);
      lcd.clearLine(4);

      sprintf(temp,"Temp: %s C",strTemp);
      lcd.drawString(0,2,temp);
      sprintf(temp,"Pres: %s hPa",strPressure);
      lcd.drawString(0,3,temp);
      sprintf(temp,"Hum: %s%%",strHumidity);
      lcd.drawString(0,4,temp);
    } else {
       // Ooops !
    }
  }

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
  Serial.println("[DEBUG] A0 value: " + String(UVlevel)+ "mV");
#endif 

  // Clear line
  lcd.clearLine(5);
  sprintf(temp,"UV: %d mV",UVlevel);
  lcd.drawString(0,5,temp);

  // Clear line
  lcd.clearLine(6);
  sprintf(temp,"PM: %s %s",strPM25,strPM10);
  lcd.drawString(0,6,temp);

  // Clear line
  lcd.clearLine(7);
  // ADC read
  adc0 = ads.readADC_SingleEnded(0);
  adc1 = ads.readADC_SingleEnded(1);
  adc2 = ads.readADC_SingleEnded(2);
  adc3 = ads.readADC_SingleEnded(3);
  
  // Convert to voltage
  float volt = (adc0 * 0.1875)/1000;
  dtostrf(volt,3,2,strBattery);
  sprintf(temp,"BATT %s",strBattery);
  lcd.drawString(0,7,temp);
    
#ifdef __DEBUG__
  Serial.print("[DEBUG] AIN0: "); Serial.println(adc0);
  Serial.print("[DEBUG] AIN1: "); Serial.println(adc1);
  Serial.print("[DEBUG] AIN2: "); Serial.println(adc2);
  Serial.print("[DEBUG] AIN3: "); Serial.println(adc3);
#endif

  queueAddRow();    
}
