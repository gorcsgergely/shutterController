#include "web.h"
#include "shutter_class.h"
#include <ArduinoJson.h>
#include "config.h"
#include "crc.h"

WebPage::WebPage(WebServer* server){
  _server = server;
}

/********************************************************
CONVERTS SIGNAL FROM dB TO %
*********************************************************/
int WebPage::WifiGetRssiAsQuality(int rssi)
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

void WebPage::timeDiff(char *buf,size_t len,unsigned long lastUpdate){
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

void WebPage::setup(){
  _server->on("/readMain", std::bind(&WebPage::readMain, this));
  _server->on("/readConfig",std::bind(&WebPage::readConfig, this));
  _server->on("/pressButton",std::bind(&WebPage::pressButton, this));
 // _server->on("/updateField",std::bind(&WebPage::updateField, this));
  _server->on("/updateConfig", HTTP_POST, std::bind(&WebPage::updateConfig, this));
}


//Sends the complete data package as response to an ajax request, currently 500ms update rate
void WebPage::readMain() {
  String mqttResults[10] = { F("-4: server didn't respond within the keepalive time"), F("-3: network connection was broken"), F("-2: network connection failed"), F("-1: client is disconnected cleanly"), F("0: client is connected"), F("1: server doesn't support the requested version of MQTT"), F("2: server rejected the client identifier"), F("3: server was unable to accept the connection"), F("4: username/password were rejected"), F("5: client was not authorized to connect") };
  char buf1[25];
  char buf2[25];
  
  JsonDocument root;  

  JsonArray keys = root["keys"].to<JsonArray>();

  keys.add(r1.btnUp.pressed?"Pressed":String(r1.btnUp.counter));
  keys.add(r1.btnDown.pressed?"Pressed":String(r1.btnDown.counter));

  JsonArray movement = root["movement"].to<JsonArray>();
  movement.add(r1.Movement());

  JsonArray position = root["position"].to<JsonArray>();
  position.add(r1.getPosition());
  //position.add(map(r1.getPosition(),0,100,100,0));
  //position.add(map(r2.getPosition(),0,100,100,0));

  JsonArray tilt= root["tilt"].to<JsonArray>();
  tilt.add(r1.getTilt());
  
  root["device"]=String(cfg.host_name);
  root["tilting"]=cfg.tilt?"true":"false";
  root["wifi"]=String(WiFi.SSID());  
  /*if (r1.semafor) {
    root["mqtt"]= mqttResults[mqttClient.state()+4]+"(S1)";
  } else {
    root["mqtt"]= mqttResults[mqttClient.state()+4];
  }*/
  timeDiff(buf1,25,lastWiFiDisconnect);
  timeDiff(buf2,25,lastMQTTDisconnect);
  root["disconnect"]=((lastWiFiDisconnect==0)?"N/A":(String(buf1)+" ago"))+","+((lastMQTTDisconnect==0)?"N/A":(String(buf2)+" ago"));
  timeDiff(buf1,25,0);
  root["crc"]=crcStatus + " ("+String(buf1)+" ago)";
  root["mem"]="program: "+String(ESP.getFreeSketchSpace()/1024)+" kB | heap: "+String(ESP.getFreeHeap()/1024)+" kB";
  if(lastCommand=="")
    root["mqttmsg"]= "N/A";
  else
    timeDiff(buf1,25,lastCallback);
    root["mqttmsg"]= lastCommand + " ("+String(buf1)+" ago)";
  root["strength"]=String(WifiGetRssiAsQuality(WiFi.RSSI()));
  root["ip"]=WiFi.localIP().toString();
  
  if(lastUpdate==0) {
    root["update"]="N/A";
  } else {
    timeDiff(buf1,25,lastUpdate);
    root["update"]=String(buf1);
  }

  String out;
  serializeJson(root,out);
#ifdef DEBUG_updates
  Serial.println(out);
#endif  
  _server->send(200, "text/plane", out); //Send values to client ajax request
}


void WebPage::pressButton() {
  String t_state = _server->arg("button"); //Refer  request.open("GET", "pressButton?button="+button, true);
  int btn=t_state.toInt();
  #ifdef DEBUG
    Serial.print("Web button: ");
    Serial.println(t_state);
  #endif

 //1,2 - pushed
 //11,12 - released
 switch(btn) {
  case 1:
    r1.btnUp.web_key_pressed=true; break;
  case 2:
    r1.btnDown.web_key_pressed=true; break;
  case 3: break;
  case 4: break;
  case 11:
    r1.btnUp.web_key_pressed=false; break;
  case 12:
    r1.btnDown.web_key_pressed=false; break;
  case 13: break;
  case 14:break;
  case 55:
    r1.Calibrate();
    break;
  case 66:
    Restart();
    break;
  case 77:
    defaultConfig(&web_cfg);
    break;
  case 88:
    copyConfig(&web_cfg,&cfg);
    saveConfig();    
    Restart();
    break;
 }
 _server->send(200, "text/plane", t_state); //Send web page
}

void WebPage::readConfig() {
  
  JsonDocument root;   // normal 1279 (but can configure longer strings, so leave it)
  root["host_name"] = web_cfg.host_name;
  root["tilt"] = web_cfg.tilt?"true":"false";
  root["auto_hold_buttons"] = web_cfg.auto_hold_buttons?"true":"false";
  root["wifi_ssid1"] = web_cfg.wifi_ssid1;
  root["wifi_password1"] = web_cfg.wifi_password1;
  root["mqtt_server"] = web_cfg.mqtt_server;
  root["mqtt_user"] = web_cfg.mqtt_user;
  root["mqtt_password"] = web_cfg.mqtt_password;
 /* root["publish_position1"] = web_cfg.publish_position1;
  root["publish_tilt1"] = web_cfg.publish_tilt1;
  root["subscribe_command1"] = web_cfg.subscribe_command1;
  root["subscribe_position1"] = web_cfg.subscribe_position1;
  root["subscribe_tilt1"] = web_cfg.subscribe_tilt1;
  root["subscribe_calibrate"] = web_cfg.subscribe_calibrate;
  root["subscribe_reboot"] = web_cfg.subscribe_reboot;
  root["subscribe_reset"] = web_cfg.subscribe_reset;*/
  root["shutter_duration_down"] = web_cfg.Shutter1_duration_down;
  root["shutter_duration_up"] = web_cfg.Shutter1_duration_up;
  root["shutter_duration_tilt"] = web_cfg.Shutter1_duration_tilt;
  
  String out;
  serializeJson(root,out);
#ifdef DEBUG_updates
  Serial.println(out);
#endif  
  _server->send(200, "text/plane", out); //Send values to client ajax request
}

void  WebPage::Restart() { 
    ESP.restart(); 
}

void WebPage::updateConfig() {

  JsonDocument doc;

  if (_server->hasArg("plain") == false) {
    return;
  }
  String body = _server->arg("plain");

  DeserializationError error = deserializeJson(doc, body);

  if (error) {
 // Serial.print("deserializeJson() failed: ");
 // Serial.println(error.c_str());
    return;
  }
  
  String hname = doc["host_name"];
  strncpy(web_cfg.host_name,hname.c_str(),24);
  String tilt = doc["tilt"];
  web_cfg.tilt = tilt.equals("true");
  String ahold = doc["auto_hold_buttons"];
  web_cfg.auto_hold_buttons = ahold.equals("true");
  String ssid = doc["wifi_ssid1"];
  strncpy(web_cfg.wifi_ssid1,ssid.c_str(),24);
  String pwd = doc["wifi_password1"];
  strncpy(web_cfg.wifi_password1,pwd.c_str(),24);
  String mqttserver = doc["mqtt_server"];
  strncpy(web_cfg.mqtt_server,mqttserver.c_str(),24);
  String mqttuser = doc["mqtt_user"];
  strncpy(web_cfg.mqtt_user,mqttuser.c_str(),24);
  String mqttpass = doc["mqtt_password"];
  strncpy(web_cfg.mqtt_password,mqttpass.c_str(),24);
  String ddown = doc["shutter_duration_down"];
  web_cfg.Shutter1_duration_down=constrain(ddown.toInt(),0,120000);
  String dup = doc["shutter_duration_up"];
  web_cfg.Shutter1_duration_up=constrain(dup.toInt(),0,120000);
  String dtilt = doc["shutter_duration_tilt"];
  web_cfg.Shutter1_duration_tilt=constrain(dtilt.toInt(),0,120000);

  _server->send(200, "application/json", "{}");

  copyConfig(&web_cfg,&cfg);
  saveConfig();    
  Restart();
}