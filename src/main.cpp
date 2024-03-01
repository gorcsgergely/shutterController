/*
 Sonoff 4ch (ESP8285) MQTT sensor for shutters
 Generic 8285 module
 (or 8266 module,  FlashMode DOUT!!!!!)
 1M (no SPIFFS)
 Reset method: ck
 
 RX <-> RX !!!
 TX <-> TX !!!
Functionality:
 Push button for <1s - cover will go till the end. If I hold it longer, shutter will stop when I release the button.
 When I push the button during movement, it will stop
 

Tilt
 For blids, there is a separate function to control tilt. When I stop the movement, it will automatically tilt to the tilt tilt before it started moving
 For blinds with tilt, the behaviour pushing button for <1s and > 1s works differently than for roller shuuers (<1s pushes are meant for tilting)
The position is based on measuring the time for the shuuer to go fully up and down (and for tilting blades)
Internally, the program works with position 0 - shutter up (open), 100 - shutter down (closed). But for mqtt, it maps the numbers 0=closed, 100=open (can be changed through commenting _reverse_position_mapping_)
It can control 1 or 2 shutters - controlled by _two_covers_ (works with Sonoff 4ch)
Sample configuration
cover:
  - platform: mqtt
    name: "MQTT Cover"
    command_topic: "blinds/cover1/set"
    state_topic: "blinds/cover1/state"
    set_position_topic:  "blinds/cover1/position"
    payload_open: "open"
    payload_close: "close"
    payload_stop: "stop"
    state_open: 0
    state_closed: 100
    tilt_command_topic: 'blinds/cover1/tilt'
    tilt_status_topic: 'blinds/cover1/tilt-state'
 
*/
#include <Arduino.h>
//#define DEBUG 1
//#define DEBUG_updates 1

#include <WiFi.h>  // ESP8266 WiFi module
//#include <ESP8266WiFiMulti.h>
#include <ESPmDNS.h>  // Sends host name in WiFi setup
#include <PubSubClient.h> // MQTT
#include <ArduinoOTA.h>   // (Over The Air) update
#include <HTTPUpdateServer.h>

#include <ArduinoJson.h>
#include <math.h>
#include "shutter_class.h"
#include "config.h"
#include "crc.h"
#include "web.h"
#include "httpServer.h"

unsigned long lastUpdate = 0; // timestamp - last MQTT update
unsigned long lastCallback = 0; // timestamp - last MQTT callback received
unsigned long lastWiFiDisconnect=0;
unsigned long lastWiFiConnect=0;
unsigned long lastMQTTDisconnect=0; // last time MQTT was disconnected
unsigned long WiFiLEDOn=0;
unsigned long k1_up_pushed=0;
unsigned long k1_down_pushed=0;

unsigned long previousWifiAttempt = 0;
unsigned long previousMQTTAttempt = 0;
boolean wifiConnected=false;

//String lastCommand = "";
//String crcStatus="";

configuration cfg,web_cfg;
Shutter r1;
topics mqtt_topics;
float upper_stop_offset=1000;

// MQTT callback declaratiion (definition below)
void callback(char* topic, byte* payload, unsigned int length); 

WiFiClient espClient;         // WiFi
//ESP8266WiFiMulti wifiMulti;   // Primary and secondary WiFi

PubSubClient mqttClient(espClient);   // MQTT client
//PubSubClient mqttClient(_mqtt_server_,1883,callback,espClient);   // MQTT client

httpServer httpserver;
WebPage webpage=WebPage(httpserver.getServer());
HTTPUpdateServer httpUpdater;

/************************ 
* S E T U P   W I F I  
*************************/
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  #ifdef DEBUG
    Serial.println();
    Serial.print("Connecting to WiFi");
  #endif

  WiFi.hostname(cfg.host_name);
  WiFi.mode(WIFI_STA); // Pouze WiFi client!!
  WiFi.begin(cfg.wifi_ssid1, cfg.wifi_password1);
//  wifiMulti.addAP(cfg.wifi_ssid1, cfg.wifi_password1);
//  wifiMulti.addAP(cfg.wifi_ssid2, cfg.wifi_password2);
  int i=0;
//  while (wifiMulti.run() != WL_CONNECTED && i<20) {
  while (WiFi.status() != WL_CONNECTED && i<60) {
    i++;
    delay(500);
    #ifdef DEBUG
      Serial.print(".");
    #endif
  }
  //Try fallback ssid and password
  if (WiFi.status() != WL_CONNECTED && (strcmp(cfg.wifi_ssid1,_ssid1_)!=0 || strcmp(cfg.wifi_password1,_password1_)!=0))  {
    #ifdef DEBUG
       Serial.println();
       Serial.print("Loading defaults and restarting...");
    #endif
    /*defaultConfig(&cfg);
    saveConfig();
    Restart();
    delay(10000);*/
    WiFi.disconnect();
    WiFi.begin(_ssid1_, _password1_);
    while (WiFi.status() != WL_CONNECTED && i<60) {
      i++;
      delay(500);
      #ifdef DEBUG
        Serial.print(".");
      #endif
    }
    return;
  }
  
  digitalWrite(SLED, LOW);   // Turn the Status Led on
  MDNS.begin(cfg.host_name);

  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", _host_name_);

  #ifdef DEBUG
    Serial.println("");
    Serial.printf("Connected to %s\n", WiFi.SSID().c_str());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  #endif
}


/********************************************
* M A I N   A R D U I N O   S E T U P 
********************************************/
void setup() {
// HW initialization
  pinMode(GPIO_REL1, OUTPUT);
  pinMode(GPIO_REL2, OUTPUT);

  pinMode(SLED, OUTPUT);
  digitalWrite(SLED, HIGH);   // Turn the Status Led off

  digitalWrite(GPIO_REL1, LOW);  // Off
  digitalWrite(GPIO_REL2, LOW);  // Off

// Open EEPROM
  openMemory();
// Open EEPROM
  openMemory();
  if (loadConfig()){
    webpage.crcStatus+="CRC config OK! ";
  }
  else{
    webpage.crcStatus+="CRC config failed. ";
  };  
  copyConfig(&cfg,&web_cfg); // copy config to web_cfg as well
  if (loadStatus()){
    if(cfg.tilt)
    webpage.crcStatus += "CRC status OK: [("+String(r1.getPosition())+","+String(r1.getTilt())+")] loaded. ";
    else   webpage.crcStatus += "CRC status OK: [("+String(r1.getPosition())+")] loaded. ";
  }
  else {
    webpage.crcStatus += "CRC status failed!";
  };

  pinMode(GPIO_KEY1, INPUT);
  pinMode(GPIO_KEY2, INPUT);

  r1.setup("Shutter1",cfg.Shutter1_duration_down,cfg.Shutter1_duration_up,cfg.Shutter1_duration_tilt,GPIO_REL1,GPIO_REL2);

  #ifdef DEBUG
    Serial.begin(115200);
  #endif

  setup_wifi();
  
  mqttClient.setServer(cfg.mqtt_server,1883);
  mqttClient.setCallback(callback);
  
// Over The Air Update
  ArduinoOTA.setHostname(cfg.host_name);
  //ArduinoOTA.setPassword(OTA_password);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin(); 

  httpserver.setup();
  webpage.setup();
  httpUpdater.setup(httpserver.getServer(),"/upgrade",WEB_UPGRADE_USER, WEB_UPGRADE_PASS);
  httpserver.begin(); //Start the server

  #ifdef DEBUG
    Serial.println("Server listening");
  #endif

  // setup done
  #ifdef DEBUG
    Serial.println("Ready");
  #endif
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

/********************************
* R E C O N N E C T   M Q T T 
********************************/
void mqtt_reconnect() {
  #ifdef DEBUG
    Serial.print("Attempting MQTT connection...");
  #endif

  unsigned long now = millis();
 // if (lastMQTTDisconnect!=0 && lastMQTTDisconnect<now && (unsigned long)(now-lastMQTTDisconnect)<10000) return;
  if (lastMQTTDisconnect!=0 && (unsigned long)(now-lastMQTTDisconnect)<10000) return;
  lastMQTTDisconnect=now;
  // Attempt to connect
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  String clientName(cfg.host_name);
  clientName += "-";
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);
  
  if (mqttClient.connect((char*)clientName.c_str(),cfg.mqtt_user,cfg.mqtt_password)) {
    #ifdef DEBUG
      Serial.println("connected");
    #endif

    // Once connected, publish an announcement...
    //digitalWrite(SLED, LOW);   // Turn the Status Led on

    // lastUpdate=0;
    // checkSensors(); // send current sensors
    // publishSensor();

    // resubscribe
    mqttClient.subscribe(mqtt_topics.subscribe_command);  // listen to control for cover 1
    mqttClient.subscribe(mqtt_topics.subscribe_position);  // listen to cover 1 postion set
    mqttClient.subscribe(mqtt_topics.subscribe_reboot);  // listen for reboot command
    mqttClient.subscribe(mqtt_topics.subscribe_calibrate);  // listen for calibration command
    if (cfg.tilt) {
      mqttClient.subscribe(mqtt_topics.subscribe_tilt);  // listen for cover 1 tilt position set
    }
  } else {
   // digitalWrite(SLED, HIGH);   // Turn the Status Led off
    #ifdef DEBUG
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
    #endif
  }
}

void Restart() {
    mqttClient.publish(mqtt_topics.subscribe_command , "" , true);
    mqttClient.publish(mqtt_topics.subscribe_position , "" , true);
    mqttClient.publish(mqtt_topics.publish_position , "" , true);
    if (cfg.tilt) {
      mqttClient.publish(mqtt_topics.subscribe_tilt , "" , true);
      mqttClient.publish(mqtt_topics.publish_tilt , "" , true);
    }       
    ESP.restart();  
}

// Callback for processing MQTT message
void callback(char* topic, byte* payload, unsigned int length) {

  char* payload_copy = (char*)malloc(length+1);
  memcpy(payload_copy,payload,length);
  payload_copy[length] = '\0';

  #ifdef DEBUG
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(payload_copy);
  #endif

  webpage.lastCommand="Topic:";
  webpage.lastCommand+=topic;
  webpage.lastCommand+=",  Payload:";
  webpage.lastCommand+=payload_copy;
  
  lastCallback= millis();

  if (strcmp(topic, mqtt_topics.subscribe_command) == 0) {

    if (strcmp(payload_copy,payload_open) == 0) {
      r1.Start_up();
    } else if (strcmp(payload_copy,payload_close) == 0) {
      r1.Start_down();
    } else if (strcmp(payload_copy,payload_stop) == 0) {
      r1.Stop(); 
    }
  } else if (strcmp(topic, mqtt_topics.subscribe_position) == 0) {
      #ifdef _reverse_position_mapping_
        int p = map(constrain(atoi(payload_copy),0,100),0,100,100,0);
      #else
        int p = constrain(atoi(payload_copy),0,100);
 
      #endif
      if (p!=r1.getPosition()) {
        r1.Go_to_position(p);   
      } else {
        r1.force_update=true;
      }    
  } else if (cfg.tilt && strcmp(topic, mqtt_topics.subscribe_tilt) == 0) {
      int tilt = constrain(atoi(payload_copy),0,100);
      if (tilt!=r1.getTilt()) {      
        r1.tilt_it(tilt);
      } else {
        r1.force_update=true;
      }
  }  else if (strcmp(topic, mqtt_topics.subscribe_calibrate) == 0) {
    r1.Calibrate();
  } else if (strcmp(topic, mqtt_topics.subscribe_reboot) == 0) {    
     Restart();
  }
  free(payload_copy);
}

/*****************************************************************
* S E N D   S E N S O R S   M Q Q T  ( J S O N ) 
******************************************************************/
void publishSensor() {
  char message1[10];
  char message2[10];
  unsigned long now = millis();

  unsigned long interval = (r1.movement==stopped)?update_interval_passive:update_interval_active;
  
  if ( lastUpdate==0 || lastUpdate>now || (unsigned long)(now-lastUpdate)>interval || r1.force_update ) {  
    // INFO: the data must be converted into a string; a problem occurs when using floats...

  #ifdef DEBUG
    if(r1.movement==stopped) Serial.print("R1 stopped"); else
     if(r1.movement==up) Serial.print("R1 up"); else
      if(r1.movement==down) Serial.print("R1 down");
  #endif

    #ifdef _reverse_position_mapping_    
      snprintf(message1,10,"%d",map(r1.getPosition(),0,100,100,0));
    #else
      snprintf(message1,10,"%d",r1.getPosition());
    #endif

    mqttClient.publish(mqtt_topics.publish_position, message1 , false);

    if (cfg.tilt) {
      snprintf(message2,10, "%d", r1.getTilt());
      mqttClient.publish(mqtt_topics.publish_tilt, message2 , false);
    }

    if(r1.movement==stopped) //no need to save when it is moving
    {
      saveStatus(); 
      webpage.lastCommand="status saved";
    }
   
    r1.force_update=false;

    lastUpdate=now;
  }
}

/***********************************************************************************************
LOOP - CHECK SENSOR AND SEND MQTT UPDATE
************************************************************************************************/
void checkSensors() {

  unsigned long now = millis();

  boolean k1_up= (digitalRead(GPIO_KEY1)==KEY_PRESSED);
  boolean k1_down= (digitalRead(GPIO_KEY2)==KEY_PRESSED);

  if (button_press_delay) { // Ignore presses shorter than 100 ms
    if(k1_up) {
      if (k1_up_pushed==0 || ((unsigned long)(now - k1_up_pushed) < _button_delay_ )) {
        k1_up=false;
        if (k1_up_pushed==0)
          k1_up_pushed=now;
      }
    } else {
      k1_up_pushed=0;
    }
    if(k1_down) {
      if (k1_down_pushed==0 || ((unsigned long)(now - k1_down_pushed) < _button_delay_ )) {
        k1_down=false;
        if (k1_down_pushed==0)
          k1_down_pushed=now;
      }
    } else {
      k1_down_pushed=0;
    }

  }
  
  r1.Process_key(k1_up,k1_down);

}  

/****************************************************
* L O O P - TIME CONTROL
****************************************************/
void checkTimers() {
  r1.Update_position();
}

/********************************************************
CONVERTS SIGNAL FROM dB TO %
*********************************************************/
int WifiGetRssiAsQuality(int rssi)
{
  int quality = 0;

  if (rssi <= -100) {
    quality = 0;
  } else if (rssi >= -50) {
    quality = 100;
  } else {
    quality = 2 * (rssi + 100);
  }
  return quality;
}

void timeDiff(char *buf,size_t len,unsigned long lastUpdate){
    //####d, ##:##:##0
    unsigned long t = millis();
    if(lastUpdate>t) {
      snprintf(buf,len,"N/A");
      return;
    }
    t=(t-lastUpdate)/1000;  // Converted to difference in seconds

    int d=t/(60*60*24);
    t=t%(60*60*24);
    int h=t/(60*60);
    t=t%(60*60);
    int m=t/60;
    t=t%60;
    if(d>0) {
      snprintf(buf,len,"%dd, %02d:%02d:%02d",d,h,m,t);
    } else if (h>0) {
      snprintf(buf,len,"%02d:%02d:%02d",h,m,t); 
    } else {
      snprintf(buf,len,"%02d:%02d",m,t); 
    }
}


/***************************************
* M A I N   A R D U I N O   L O O P  
***************************************/
void loop() {
  unsigned long now = millis();
  checkTimers();
  checkSensors();
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(SLED, LOW);   // Turn the Status Led on
    if (!wifiConnected) //just (re)connected
    {
      wifiConnected=true;
      //MDNS.begin(cfg.host_name);
      //MDNS.addService("http", "tcp", 80);
    }
    //lastWiFiConnect=now;  // Not used at the moment
    ArduinoOTA.handle(); // OTA first
    if (mqttClient.loop()) {
       publishSensor();
    } else {
      if((unsigned long)(now - previousMQTTAttempt) > MQTT_RETRY_INTERVAL)//every 10 sec
      {
        mqtt_reconnect();  
        previousMQTTAttempt = now;    
      }
    }   
    httpserver.handleClient();         // Web handling
  } else{ 
    if(wifiConnected) //just disconnected
    {
      wifiConnected=false;
      //MDNS.end();
    }
    if ((WiFi.status() != WL_CONNECTED) && ((unsigned long)(now - previousWifiAttempt) > WIFI_RETRY_INTERVAL)) {//every 30 sec
      digitalWrite(SLED, HIGH);   // Turn the Status Led off
      WiFi.disconnect();
      WiFi.begin(cfg.wifi_ssid1, cfg.wifi_password1);
      previousWifiAttempt = now;
    } 
  }
  //delay(update_interval_loop); // 25 ms (short because of tilt) (1.5 degrees in 25 ms)
}
