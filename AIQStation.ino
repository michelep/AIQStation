// Air Quality Station 
// Temperature, Humidity and Pressure
// with Lolin ESP8266
//
// Bosch BME820 Temp-Hum-Pressure
// SDS011 PM2.5/PM10 Laser sensor
// MQ135 CO2 sensor on A0
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty
//
// A part of iHot project - http://ihot.zerozone.it
// 
// Program using: NodeMCU 1.0 (ESP12E) Module

// Enable diagnostic messages to serial or rsyslog
#define __DEBUG__

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266httpUpdate.h>
#include <EEPROM.h>

// Syslog logger
// https://github.com/arcao/Syslog/
#include <WiFiUdp.h>
#include <Syslog.h>

WiFiUDP udpClient;

// Create a new syslog instance with LOG_KERN facility
Syslog syslog(udpClient, SYSLOG_PROTO_IETF);

// Task Scheduler
#include <TaskScheduler.h>

// Arduino Json
#include <ArduinoJson.h>

// REST Client
// https://github.com/michelep/esp8266-restclient
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

// MQ135 Air Quality Sensor
#include <MQ135.h>

MQ135 mq135 = MQ135(A0);

// Dust detector
//
#include <SDS011.h>

// SDS010 SoftSerial port
#define SDS_RX 0 // D3
#define SDS_TX 2 // D4

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
  // Syslog config
  char syslog_server[16];
  unsigned int syslog_port;
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
  char temp[10]; 
  char hum[10]; 
  char pres[10];
  char pm25[10];
  char pm10[10];
  char aq[10];
} dataRow;

Queue dataQueue(sizeof(dataRow), QUEUE_MAX_SIZE, FIFO, true); // Instantiate queue

// Format SPIFFS if mount failed
#define FORMAT_SPIFFS_IF_FAILED 1

// Firmware data
const char BUILD[] = __DATE__ " " __TIME__;
#define FW_NAME         "aiq-station"
#define FW_VERSION      "0.0.3"

#define DEFAULT_WIFI_ESSID "aiq-station"
#define DEFAULT_WIFI_PASSWORD ""

// Web server
AsyncWebServer server(80);

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

#define MQ135_WARMUP 60000*15 // Quarter of hour
void mq135Callback();
Task mq135Task(MQ135_WARMUP, TASK_ONCE, &mq135Callback);

// NTP
NTPSyncEvent_t ntpEvent;
bool syncEventTriggered = false; // True if a time even has been triggered

// Environmental values
float lastTemp=0,lastHumidity=0,lastPressure=0,lastPM10=0,lastPM25=0,lastAQ=0;
//
static int dataCounter=0,dataErrorCounter=0;
//
static long sdsCount=0,last,lastDataTS;
bool sdsIsWarmup=false;

bool isBME; // True if BME280 is present and active
bool isMQ135; // True if MQ135 Air Quality sensor is active and ready (after some hours of power up...)
bool isClientMode; // True if connected to an AP, false if AP mode
bool isConnected; // True if connected to collector server

IPAddress myIP;

// ************************************
// DEBUG_PRINT(message)
//
// ************************************
void DEBUG_PRINT(String message) {
#ifdef __DEBUG__
  if(strlen(config.syslog_server) > 0) {
    if(!syslog.log(LOG_INFO,message)) {
      Serial.print("[Syslog] ");
      Serial.println(message);
    }
  } else {
    Serial.println(message);    
  }
#endif
}

// ************************************
// connectToWifi()
//
// ************************************
bool connectToWifi() {
  uint8_t timeout=0;

  if(strlen(config.wifi_essid) > 0) {
    DEBUG_PRINT("[DEBUG] Connecting to "+String(config.wifi_essid));

    WiFi.begin(config.wifi_essid, config.wifi_password);
  
    while((WiFi.status() != WL_CONNECTED)&&(timeout < 10)) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(250);
      DEBUG_PRINT(".");
      digitalWrite(LED_BUILTIN, HIGH);
      delay(250);
      timeout++;
    }
    if(WiFi.status() == WL_CONNECTED) {
      DEBUG_PRINT("OK. IP:"+String(WiFi.localIP()));
      if (MDNS.begin(config.hostname)) {
        DEBUG_PRINT("[DEBUG] MDNS responder started");
        // Add service to MDNS-SD
        MDNS.addService("http", "tcp", 80);
      }
      
      NTP.onNTPSyncEvent ([](NTPSyncEvent_t event) {
        ntpEvent = event;
        syncEventTriggered = true;
      });

      // NTP
      DEBUG_PRINT("[INIT] Start sync NTP time...");
      NTP.begin (config.ntp_server, config.ntp_timezone, true, 0);
      NTP.setInterval(63);
       
      return true;  
    } else {
      DEBUG_PRINT("[ERROR] Failed to connect to WiFi");
      return false;
    }
  } else {
    DEBUG_PRINT("[ERROR] Please configure Wifi");
    return false; 
  }
}

// ************************************
// runWifiAP()
//
// run Wifi AccessPoint, to let user use and configure without a WiFi AP
// ************************************
void runWifiAP() {
  DEBUG_PRINT("[DEBUG] runWifiAP() ");
  WiFi.mode(WIFI_AP_STA); 
  WiFi.softAP(DEFAULT_WIFI_ESSID,DEFAULT_WIFI_PASSWORD);  
 
  myIP = WiFi.softAPIP(); //Get IP address

  DEBUG_PRINT("[INIT] WiFi Hotspot ESSID: "+String(DEFAULT_WIFI_ESSID));
  DEBUG_PRINT("[INIT] WiFi password: "+String(DEFAULT_WIFI_PASSWORD));
  DEBUG_PRINT("[INIT] Server IP: "+String(myIP));

  WiFi.printDiag(Serial);

  if (MDNS.begin(config.hostname)) {
    DEBUG_PRINT("[INIT] MDNS responder started for hostname "+String(config.hostname));
    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
  }
}

enum i2c_status   {I2C_WAITING,     // stopped states
                   I2C_TIMEOUT,     //  |
                   I2C_ADDR_NAK,    //  |
                   I2C_DATA_NAK,    //  |
                   I2C_ARB_LOST,    //  |
                   I2C_BUF_OVF,     //  |
                   I2C_NOT_ACQ,     //  |
                   I2C_DMA_ERR,     //  V
                   I2C_SENDING,     // active states
                   I2C_SEND_ADDR,   //  |
                   I2C_RECEIVING,   //  |
                   I2C_SLAVE_TX,    //  |
                   I2C_SLAVE_RX};

bool i2c_status() {
  switch(Wire.status()) {
    case I2C_WAITING:  DEBUG_PRINT("I2C waiting, no errors"); return true;
    case I2C_ADDR_NAK: DEBUG_PRINT("Slave addr not acknowledged"); break;
    case I2C_DATA_NAK: DEBUG_PRINT("Slave data not acknowledged\n"); break;
    case I2C_ARB_LOST: DEBUG_PRINT("Bus Error: Arbitration Lost\n"); break;
    case I2C_TIMEOUT:  DEBUG_PRINT("I2C timeout\n"); break;
    case I2C_BUF_OVF:  DEBUG_PRINT("I2C buffer overflow\n"); break;
    default:           DEBUG_PRINT("I2C busy\n"); break;
  }
  return false;
}

bool BME280Init() {
  bool status;
  DEBUG_PRINT("[INIT] Initializing BME280...");
  status = bme.begin();  
  if (!status) {
    DEBUG_PRINT("[INIT] BME280: ERROR");
    return false;
  } 
  // Set BLE sampling for weather monitor
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF   );
  DEBUG_PRINT("[INIT] BME280: OK");
  return true;
}

// *******************************
// Setup
//
// Boot-up routine: setup hardware, display, sensors and connect to wifi...
// *******************************
void setup() {

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
  DEBUG_PRINT("[INIT] Initializing OLED display...");
  lcd.begin();

  lcd.setFont(u8x8_font_chroma48medium8_r);
  lcd.clearDisplay();
  lcd.drawString(0,1,".:AIQ Station:.");
  lcd.drawString(0,3,"booting...");

  // Start scheduler
  runner.init();

  // Initialize SPIFFS
  if(!SPIFFS.begin()){
    DEBUG_PRINT("[ERROR] SPIFFS Mount Failed. Try formatting...");
    if(SPIFFS.format()) {
      DEBUG_PRINT("[INIT] SPIFFS initialized successfully");
    } else {
      DEBUG_PRINT("[FATAL] SPIFFS error");
      ESP.restart();
    }
  } else {
    DEBUG_PRINT("[INIT] SPIFFS done");
  }

  // Load configuration
  loadConfigFile();

  // Initialize Syslog
  if(strlen(config.syslog_server) > 0) {
    DEBUG_PRINT("[INIT] Syslog to "+String(config.syslog_server)+":"+String(config.syslog_port));
    syslog.server(config.syslog_server, config.syslog_port);
    syslog.deviceHostname(config.hostname);
    syslog.appName(FW_NAME);
    syslog.defaultPriority(LOG_KERN);
  }
      
  // Setup I2C...
  DEBUG_PRINT("[INIT] Initializing I2C bus...");
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  i2c_status();
  // Initialize BME280 
  isBME = BME280Init();
  
  // Initialize SHS011 dust sensor
  DEBUG_PRINT("[INIT] Initializing SHS011 dust sensor...");
  sds.begin(SDS_RX,SDS_TX);
  
  // Add sds callback for periodic measurement, every 60xN seconds...
  runner.addTask(sdsTask);
  sdsTask.enable();

  // Add queue task
  runner.addTask(queueTask);
  queueTask.enable();
  
  // Add environmental sensor data fetch task
  runner.addTask(envTask);
  envTask.enable();

  // Add MQ135 timeout to enable after warmup
  runner.addTask(mq135Task);
  mq135Task.enableDelayed(MQ135_WARMUP);
  
  // Connect to WiFi network: if failed, start AP mode...
  if(!connectToWifi()) {
    runWifiAP();
    isClientMode=false;
  } else {
    isClientMode=true;
  }


  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUG_PRINT("[OTA] Progress: "+String(progress/(total/100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if(error == OTA_AUTH_ERROR) {
       DEBUG_PRINT("[OTA] Auth Failed");
    }
    else if(error == OTA_BEGIN_ERROR) DEBUG_PRINT("[OTA] Begin Failed");
    else if(error == OTA_CONNECT_ERROR) DEBUG_PRINT("[OTA] Connect Failed");
    else if(error == OTA_RECEIVE_ERROR) DEBUG_PRINT("[OTA] Receive Failed");
    else if(error == OTA_END_ERROR) DEBUG_PRINT("[OTA] End Failed");
  });
  ArduinoOTA.setHostname(config.hostname);
  ArduinoOTA.begin();

  // Initialize web server on port 80
  initWebServer();
}

// *******************************
// displayUpdate()
//
// I2C LCD display update with data
// *******************************
void displayUpdate() {
  char temp[32],val[16];
  
  lcd.clearLine(1);
  if(isClientMode) {
    sprintf(temp,"%s",NTP.getTimeStr().c_str());
    lcd.drawString(0,1,temp);
  } else {
    sprintf(temp,"IP %s",myIP.toString().c_str());
    lcd.drawString(0,1,temp);
  }
  
  lcd.clearLine(2);
  dtostrf(lastTemp,6,2,val);
  sprintf(temp,"Temp: %s C",val);
  lcd.drawString(0,2,temp);

  lcd.clearLine(3);
  dtostrf(lastPressure,6,2,val);
  sprintf(temp,"Pres: %s hPa",val);
  lcd.drawString(0,3,temp);        

  lcd.clearLine(4);
  dtostrf(lastHumidity,6,2,val);
  sprintf(temp,"Hum: %s%%",val);
  lcd.drawString(0,4,temp);

  lcd.clearLine(5);
  dtostrf(lastAQ,6,2,val);
  sprintf(temp,"VOCs: %s PPM",val);
  lcd.drawString(0,5,temp);

  lcd.clearLine(6);
  if(lastPM10 > 0) {
    dtostrf(lastPM10, 6, 2, val);
    sprintf(temp,"PM10: %s",val);
  } else {
    sprintf(temp,"PM10...");
  }
  lcd.drawString(0,6,temp); 
  
  lcd.clearLine(7);
  if(lastPM25 > 0) {
    dtostrf(lastPM25, 6, 2, val);
    sprintf(temp,"PM25: %s",val);  
  } else {
    sprintf(temp,"PM25...");
  }
  lcd.drawString(0,7,temp);
}

// ************************************
// Loop()
//
// ************************************
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
    char temp[30];
    if(isClientMode) {
      // If in client mode, check if wifi connection is up and, if not, try reconnecting...
      lcd.clearLine(1);     
      if(WiFi.status() != WL_CONNECTED) {    
        isConnected = false;
        connectToWifi();
      } 
    } else {
      // AP mode     
      DEBUG_PRINT("[DEBUG] AP mode - Clients: "+String(WiFi.softAPgetStationNum()));
    }
    // Display it on LCD
    displayUpdate();
    // Prepare for next loop
    last = millis();
  }
}
