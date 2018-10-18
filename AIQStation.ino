// Air Quality Station 
// Temperature, Humidity and Pressure
// with Lolin ESP8266
//
// Bosch BME820 Temp-Hum-Pressure
// SDS011 PM2.5/PM10 Laser sensor
// UV Ray detector on A0 - range 0-1V: 
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty
//
// A part of iHot project - http://ihot.zerozone.it
// 
// Program using: NodeMCU 1.0 (ESP12E) Module

// #define HTTP_DEBUG // Need to debug HTTP transactions ?
// Enable diagnostic messages to serial
#define __DEBUG__ 1

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266httpUpdate.h>

#include <EEPROM.h>

#include <TaskScheduler.h>

#include <ArduinoJson.h>

// REST Client
// https://github.com/DaKaZ/esp8266-restclient
#include "RestClient.h"  

// NTP ClientLib 
// https://github.com/gmag11/NtpClient
#include <NtpClientLib.h>

// File System
#include <FS.h>   
// #include <SPIFFS.h> // SPIFFS

#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

// BME280 sensor
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme; // I2C

// ADS1x15 ADC module (4 ADC input connected via I2C bus
// https://github.com/adafruit/Adafruit_ADS1X15
#include <Adafruit_ADS1015.h>

Adafruit_ADS1015 ads(0x48);

// Dust detector
// https://github.com/ricki-z/SDS011
#include <SDS011.h>

// SDS010 SoftSerial port
#define SDS_TX 0 // D3
#define SDS_RX 2 // D4

SDS011 sds;

// OLED display
// https://github.com/olikraus/u8g2/wiki/setup_tutorial
#include <U8g2lib.h>
#define I2C_SDA 5 // D1
#define I2C_SCL 4 // D2
U8X8_SSD1306_128X64_NONAME_SW_I2C lcd(I2C_SCL,I2C_SDA);
// U8G2_SSD1306_128X64_NONAME_1_SW_I2C lcd(U8G2_R0,I2C_SCL,I2C_SDA,U8X8_PIN_NONE);

// Config
struct Config {
  // WiFi config
  char wifi_essid[16];
  char wifi_password[16];
  // NTP Config
  char ntp_server[16];
  int8_t ntp_timezone;
  // Collector config
  char collector_host[32];
  char api_key[16];
  // Host config
  char hostname[16];
  char www_username[16];
  char www_password[16];
  // 
};

#define CONFIG_FILE "/config.json"

File configFile;
Config config; // Global config object

// Data queue
// Handle all data fetched from sensors before sending it to server
//
// Use Queue library from
// https://github.com/SMFSW/Queue.git
#include <cppQueue.h>

#define QUEUE_MAX_SIZE 10

typedef struct _dataRow {
  long int ts; // TimeStamp
  char temp[6]; 
  char hum[6]; 
  char pres[6];
  char pm25[6];
  char pm10[6];
  unsigned int uv;
} dataRow;

Queue dataQueue(sizeof(dataRow), QUEUE_MAX_SIZE, FIFO, true); // Instantiate queue

// Format SPIFFS if mount failed
#define FORMAT_SPIFFS_IF_FAILED 1

// Firmware data
const char BUILD[] = __DATE__ " " __TIME__;
#define FW_NAME         "aiq-station"
#define FW_VERSION      "0.0.1"

#define DEFAULT_WIFI_ESSID "aiq-station"
#define DEFAULT_WIFI_PASSWORD ""

// Web server
AsyncWebServer server(80);
AsyncEventSource events("/events");

RestClient *restClient;

// Scheduler and tasks...
Scheduler runner;

#define SDS_INTERVAL 60000*15 // Every 15 minutes...
void sdsCallback();
Task sdsTask(SDS_INTERVAL, TASK_FOREVER, &sdsCallback);

#define ENV_INTERVAL 60000*5 // Environmental sensor data fetch every 5 minutes
void envCallback();
Task envTask(ENV_INTERVAL, TASK_FOREVER, &envCallback);

#define QUEUE_INTERVAL 60000 // Queue callback, every minute
void queueCallback();
Task queueTask(QUEUE_INTERVAL, TASK_FOREVER, &queueCallback);


// NTP
NTPSyncEvent_t ntpEvent;
bool syncEventTriggered = false; // True if a time even has been triggered

char strTemp[10],strHumidity[10],strPressure[10],strPM10[10],strPM25[10],strBattery[10];
static int last;
static long sdsLast=0;
bool sdsIsWarmup=false;
unsigned int UVlevel;
bool isBME; // True if BME280 is present and active
bool isClientMode; // True if connected to an AP, false if AP mode
bool isConnected; // True if connected to collector server
IPAddress myIP;

// ==============================================
bool connectToWifi() {
  uint8_t timeout=0;

  if(strlen(config.wifi_essid) > 0) {
#ifdef __DEBUG__ 
    Serial.print("[DEBUG] Connecting to ");
    Serial.print(config.wifi_essid);
#endif 

    WiFi.begin(config.wifi_essid, config.wifi_password);

    while((WiFi.status() != WL_CONNECTED)&&(timeout < 10)) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(250);
      Serial.print(".");
      digitalWrite(LED_BUILTIN, HIGH);
      delay(250);
      timeout++;
    }
    if(WiFi.status() == WL_CONNECTED) {
#ifdef __DEBUG__ 
      Serial.print("OK. IP:");
      Serial.println(WiFi.localIP());
#endif
      if (MDNS.begin(config.hostname)) {
#ifdef __DEBUG__      
        Serial.println("[DEBUG] MDNS responder started");
#endif      
        // Add service to MDNS-SD
        MDNS.addService("http", "tcp", 80);
      }
      
      NTP.onNTPSyncEvent ([](NTPSyncEvent_t event) {
        ntpEvent = event;
        syncEventTriggered = true;
      });

      // NTP
      Serial.println("[INIT] Start sync NTP time...");
      NTP.begin (config.ntp_server, config.ntp_timezone, true, 0);
      NTP.setInterval(63);

      restClient = new RestClient(config.collector_host); 
      // Now register to Collector host...
      envRegister();
      
      return true;  
    } else {
      Serial.println("[ERROR] Failed to connect to WiFi");
      return false;
    }
  } else {
#ifdef __DEBUG__
    Serial.println("[ERROR] Please configure Wifi");
#endif    
    return false; 
  }
}

// ************************************
// runWifiAP()
//
// run Wifi AccessPoint, to let user use and configure without a WiFi AP
// ************************************
void runWifiAP() {
#ifdef __DEBUG__
  Serial.println("[DEBUG] runWifiAP() ");
#endif
  WiFi.mode(WIFI_AP_STA); 
  WiFi.softAP(DEFAULT_WIFI_ESSID,DEFAULT_WIFI_PASSWORD);  
 
  myIP = WiFi.softAPIP(); //Get IP address

  Serial.print("[INIT] WiFi Hotspot ESSID: ");
  Serial.println(DEFAULT_WIFI_ESSID);  
  Serial.print("[INIT] WiFi password: ");
  Serial.println(DEFAULT_WIFI_PASSWORD);  
  Serial.print("[INIT] Server IP: ");
  Serial.println(myIP);

  WiFi.printDiag(Serial);

  if (MDNS.begin(config.hostname)) {
    Serial.print("[INIT] MDNS responder started for hostname ");
    Serial.println(config.hostname);
    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
  }
}

// *******************************
// Setup
//
// Boot-up routine: setup hardware, display, sensors and connect to wifi...
// *******************************
void setup() {
  bool status;
  Serial.begin(115200);
  delay(10);

  // print firmware and build data
  Serial.println();
  Serial.println();
  Serial.print(FW_NAME);
  Serial.print(FW_VERSION);
  Serial.println(BUILD);

  // Initialize display
// https://github.com/olikraus/u8g2/wiki/u8x8reference
#ifdef __DEBUG__
  Serial.println("[INIT] Initializing OLED display...");
#endif    
  lcd.begin();

  lcd.setFont(u8x8_font_chroma48medium8_r);
  lcd.clearDisplay();
  lcd.drawString(0,1,".:AIQ Station:.");
  lcd.drawString(0,3,"booting...");

  // Start scheduler
  runner.init();

  // Initialize SPIFFS
  if(!SPIFFS.begin()){
#ifdef __DEBUG__    
    Serial.println("[ERROR] SPIFFS Mount Failed. Try formatting...");
#endif    
    if(SPIFFS.format()) {
      Serial.println("[INIT] SPIFFS initialized successfully");
    } else {
      Serial.println("[FATAL] SPIFFS error");
      ESP.restart();
    }
  } else {
#ifdef __DEBUG__    
    Serial.println("SPIFFS done");
#endif    
  }

  // Load configuration
  loadConfigFile();

  // Setup I2C...
#ifdef __DEBUG__
  Serial.print("[INIT] Initializing I2C bus...");
#endif
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);
#ifdef __DEBUG__
  Serial.println(Wire.status());
#endif
  // Initialize BME280 
#ifdef __DEBUG__
  Serial.print("[INIT] Initializing BME280...");
#endif    
  status = bme.begin();  
  if (!status) {
    Serial.println("ERROR");
    isBME = false;
  } else {
    isBME = true;
#ifdef __DEBUG__
    Serial.println("OK");
#endif
  }
  // Set BLE sampling for weather monitor
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF   );
  
  // Initialize ADC ADS1X15 on I2C bus...
#ifdef __DEBUG__
  Serial.println("[INIT] Initializing ADS1115 ADC...");
#endif    
  ads.begin();
  ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV (default)

  // Initialize SHS011 dust sensor
#ifdef __DEBUG__
  Serial.println("[INIT] Initializing SHS011 dust sensor...");
#endif    
  sds.begin(SDS_TX, SDS_RX); //RX, TX
  // Add sds callback for periodic measurement, every 60xN seconds...
  runner.addTask(sdsTask);
  sdsTask.enable();

  // Add queue task
  runner.addTask(queueTask);
  queueTask.enable();
  
  // Add environmental sensor data fetch task
  runner.addTask(envTask);
  envTask.enable();
  
  // Connect to WiFi network: if failed, start AP mode...
  if(!connectToWifi()) {
    runWifiAP();
    isClientMode=false;
  } else {
    isClientMode=true;
  }

  // Setup OTA
  ArduinoOTA.onStart([]() { 
    events.send("Update Start", "ota");
  });
  ArduinoOTA.onEnd([]() { 
    events.send("Update End", "ota"); 
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    char p[32];
    sprintf(p, "Progress: %u%%\n", (progress/(total/100)));
    events.send(p, "ota");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if(error == OTA_AUTH_ERROR) events.send("Auth Failed", "ota");
    else if(error == OTA_BEGIN_ERROR) events.send("Begin Failed", "ota");
    else if(error == OTA_CONNECT_ERROR) events.send("Connect Failed", "ota");
    else if(error == OTA_RECEIVE_ERROR) events.send("Recieve Failed", "ota");
    else if(error == OTA_END_ERROR) events.send("End Failed", "ota");
  });
  ArduinoOTA.setHostname(config.hostname);
  ArduinoOTA.begin();

  // Initialize web server on port 80
  initWebServer();
}

void loop() {
  // Handle OTA
  ArduinoOTA.handle();

  // Scheduler
  runner.execute();

  // NTP ?
  if(syncEventTriggered) {
    processSyncEvent(ntpEvent);
    syncEventTriggered = false;
  }
  
  if ((millis () - last) > 5100) {   // Ogni 5 secondi circa... 
    char temp[20];
    if(isClientMode) {
      // If in client mode, check if wifi connection is up and, if not, try reconnecting...
      lcd.clearLine(1);     
      if(WiFi.status() != WL_CONNECTED) {    
        isConnected = false;
        lcd.drawString(0,1,"Reconnecting...");
        connectToWifi();
      } else {
        sprintf(temp,"%s",NTP.getTimeStr().c_str());
        lcd.drawString(0,1,temp);
      }
    } else {
      // AP mode
      // Clear line
      lcd.clearLine(1);
      sprintf(temp,"IP %s",myIP.toString().c_str());
      lcd.drawString(0,1,temp);
#ifdef __DEBUG__      
      Serial.printf("[DEBUG] AP mode - Clients: %d\n", WiFi.softAPgetStationNum());
#endif      
    }
    last = millis ();
  }
}

