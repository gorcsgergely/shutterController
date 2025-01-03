#include "config.h"
#include "crc.h"
#include "shutter_class.h"

/*****************************
 *  EEPROM MAP
 *
 *  CONFIGURATION | CRC | POSITION | TILT | CRC
 *
****************************/

struct shutter_position {
    int r1_position;
    int r1_tilt;
} p;

/**** Calculation of CRC of stored values ****/ 

unsigned long eeprom_crc(int o,int s) {
  const unsigned long crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

  unsigned long crc = ~0L;
  byte value;

  for (int index = 0 ; index < s  ; ++index) {
    value = EEPROM.read(o+index);
    crc = crc_table[(crc ^ value) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (value >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc;
}

void openMemory() {
    EEPROM.begin(sizeof(configuration)+sizeof(unsigned long)+sizeof(shutter_position)+sizeof(unsigned long));
}

void saveConfig() {
  EEPROM.put(0,cfg);
  EEPROM.commit();
  unsigned long check=eeprom_crc(0,sizeof(configuration));
  EEPROM.put(sizeof(configuration),check);
  EEPROM.commit();  
}

boolean loadConfig() {
  unsigned long check1;
  unsigned long check2;
  
  EEPROM.get(0,cfg);

  //Set up topics by the name automatically
  char temp_prefix[40];
  strcpy(temp_prefix,"blinds/");
  strcat(temp_prefix,cfg.host_name);

  strcpy(mqtt_topics.publish_position,temp_prefix);
  strcat(mqtt_topics.publish_position,"/state/position");

  strcpy(mqtt_topics.publish_tilt,temp_prefix);
  strcat(mqtt_topics.publish_tilt,"/state/tilt");

  strcpy(mqtt_topics.subscribe_position,temp_prefix);
  strcat(mqtt_topics.subscribe_position,"/position");

  strcpy(mqtt_topics.subscribe_tilt,temp_prefix);
  strcat(mqtt_topics.subscribe_tilt,"/tilt");

  strcpy(mqtt_topics.subscribe_command,temp_prefix);
  strcat(mqtt_topics.subscribe_command,"/set");

  strcpy(mqtt_topics.subscribe_calibrate,temp_prefix);
  strcat(mqtt_topics.subscribe_calibrate,"/calibrate");

  strcpy(mqtt_topics.subscribe_reboot,temp_prefix);
  strcat(mqtt_topics.subscribe_reboot,"/reboot");

  strcpy(mqtt_topics.subscribe_scenes,"blinds/scene");

  /*strcpy(mqtt_topics.subscribe_all, temp_prefix);
  strcat(mqtt_topics.subscribe_all,"/+");*/

  EEPROM.get(sizeof(configuration),check2);
  check1=eeprom_crc(0,sizeof(configuration));
  if (check1!=check2) {  
    defaultConfig(&cfg);
    saveConfig();
    //crcStatus+="CRC config failed. ";
    return false;
  } else {
    //crcStatus+="CRC config OK! ";
    return true;
  }
}

void defaultConfig(configuration* c) {
  #if defined(_tilt_)
    c->tilt=true;
  #else
    c->tilt=false;    
  #endif

  #ifdef _auto_hold_buttons_
    c->auto_hold_buttons=true;
  #else
    c->auto_hold_buttons=false;
  #endif
  
  strncpy(c->host_name,_host_name_,24);
  strncpy(c->wifi_ssid1,_ssid1_,24);
  strncpy(c->wifi_password1,_password1_,24);
  strncpy(c->mqtt_server,_mqtt_server_,24);
  for(int i= 0; i<8; i++){
    c->scenes[i][0]=0; c->scenes[i][1]=0;
  }
  c->Shutter1_duration_down=_Shutter1_duration_down_;
  c->Shutter1_duration_up=_Shutter1_duration_up_;

  #if defined(_tilt_)
    c->Shutter1_duration_tilt=_Shutter1_duration_tilt_;
  #else
    c->Shutter1_duration_tilt=1;
    strncpy(c->publish_tilt1,"",49);
    strncpy(c->subscribe_tilt1,"",49);
  #endif
}

void copyConfig(configuration* from,configuration* to) {
  to->auto_hold_buttons=from->auto_hold_buttons;
  to->tilt=from->tilt;
  strncpy(to->host_name,from->host_name,24);
  strncpy(to->wifi_ssid1,from->wifi_ssid1,24);
  strncpy(to->wifi_password1,from->wifi_password1,24);
  strncpy(to->mqtt_server,from->mqtt_server,24);
  for(int i= 0; i<8; i++){
    to->scenes[i][0]=from->scenes[i][0];
    to->scenes[i][1]=from->scenes[i][1];
  }
  /*strncpy(to->publish_position1,from->publish_position1,49);
  strncpy(to->subscribe_command1,from->subscribe_command1,49);
  strncpy(to->subscribe_position1,from->subscribe_position1,49);
  strncpy(to->subscribe_calibrate,from->subscribe_calibrate,49);
  strncpy(to->subscribe_reboot,from->subscribe_reboot,49);*/
  to->Shutter1_duration_down=from->Shutter1_duration_down;
  to->Shutter1_duration_up=from->Shutter1_duration_up;
  to->Shutter1_duration_tilt=from->Shutter1_duration_tilt; 
  /*strncpy(to->publish_tilt1,from->publish_tilt1,49);
  strncpy(to->subscribe_tilt1,from->subscribe_tilt1,49);*/
}

void saveStatus() {
  if (p.r1_position==r1.getPosition() and p.r1_tilt==r1.getTilt())
    return;
  p.r1_position=r1.getPosition();
  p.r1_tilt=r1.getTilt();
  EEPROM.put(sizeof(configuration)+sizeof(unsigned long),p);
  EEPROM.commit();
  unsigned long check=eeprom_crc(sizeof(configuration)+sizeof(unsigned long),sizeof(shutter_position));
  EEPROM.put(sizeof(configuration)+sizeof(unsigned long)+sizeof(shutter_position),check);
  EEPROM.commit();
}

boolean loadStatus() {
  unsigned long check1;
  unsigned long check2;
  
  EEPROM.get(sizeof(configuration)+sizeof(unsigned long),p); // read shutter position and tilt from memory
  EEPROM.get(sizeof(configuration)+sizeof(unsigned long)+sizeof(shutter_position),check2); //read CRC from memory

  check1=eeprom_crc(sizeof(configuration)+sizeof(unsigned long),sizeof(shutter_position));
  if (check1==check2) {
    #ifdef DEBUG
      Serial.println("EEPROM CRC check OK, reading stored values.");
    #endif
    r1.setPosition(p.r1_position);
    if (cfg.tilt) r1.setTilt(p.r1_tilt);
    return true;
    #ifdef DEBUG
      Serial.printf("shutter position %d\n",p.r1_position);
    #endif
  } else {
    #ifdef DEBUG
      Serial.println("EEPROM CRC check failed, using defaults.");
    #endif
    r1.setPosition(0);
    if (cfg.tilt) {
      r1.setTilt(0);
    }
    return false;
//    r1.Calibrate();   Ideally, I woudl need to calibrate here. But what if it restarts at night? Disable for now!
//    r2.Calibrate();
  }
}
