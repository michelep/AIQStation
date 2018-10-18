// AIQ Station 
// Temperature, Humidity and Pressure
// with Lolin ESP8266
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty

char temp[32],query[64];
unsigned int statusCode;
     
bool envSendData(char *url, char *data) {
  String response="";
  
#ifdef __DEBUG__  
  Serial.print("[DEBUG] REST: ");
  Serial.print(url);
  Serial.print(" => ");  
  Serial.println(query);
#endif

  statusCode = restClient->post(url, query, &response);

#ifdef __DEBUG__
  Serial.print("[DEBUG] Status code from server: ");
  Serial.println(statusCode);
  Serial.print("[DEBUG] Response body from server: ");
  Serial.println(response);
#endif  

  return true;
}

void envRegister() {
  if(strlen(config.api_key) > 0) {
    sprintf(query,"key=%s&hostname=%s&fw=%s-%s",config.api_key,config.hostname,FW_NAME,FW_VERSION);

    envSendData("/rest/register",query);
  } else {
    Serial.println("[ERROR] Api key not set");
  }
}

#define SEALEVELPRESSURE_HPA (1013.25)

void envCallback() {   
  int16_t adc0, adc1, adc2, adc3;
   
  if(isBME) {
    dtostrf(bme.readTemperature(),3,2,strTemp);
    dtostrf((bme.readPressure() / 100.0F), 3, 2, strPressure);
    dtostrf(bme.readHumidity(),3,2,strHumidity);
#ifdef __DEBUG__
    Serial.println("[DEBUG] Temp: " + String(strTemp)+ "°C");
    Serial.println("[DEBUG] Pressure: " + String(strPressure)+ "hPa");
    Serial.println("[DEBUG] Humidity: " + String(strHumidity)+ "%");
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
  Serial.print("AIN0: "); Serial.println(adc0);
#endif

  queueAddRow();

  // Now prepare and send data to collector server...
  //sprintf(query,"key=%s&t=%s&p=%s&h=%s&uv=%d&pm25=%s&pm10=%s",config.api_key,strTemp,strPressure,strHumidity,UVlevel,strPM25,strPM10);
  //envSendData("/rest/data/put",query);     
}
