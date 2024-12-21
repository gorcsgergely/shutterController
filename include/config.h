#include <Arduino.h>
#ifndef CONFIG_H
#define CONFIG_H

#define _WEB_ 1

//#define SHELLY 1
#define SONOFF 1

// Constants (default values, stored to EEPROM during the first run, can be configured from the web interface)
const char _host_name_[] = "blind0";  // Has to be unique for each device

//comment for covers with no tilting blades
#define _tilt_ 1

// filters out noise (100 ms)
const boolean button_press_delay = true;   
#define _button_delay_ 100  

//MQTT parameters (has to be unique for each device)
/*const char _publish_position1_[] = "blinds/blind0/state";
const char _publish_tilt1_[] = "blinds/blind0/tilt-state";
const char _subscribe_command1_[] = "blinds/blind0/set";
const char _subscribe_position1_[] = "blinds/blind0/position";
const char _subscribe_tilt1_[] = "blinds/blind0/tilt";

const char _subscribe_calibrate_[] = "blinds/blind0/calibrate";
const char _subscribe_reset_[] = "blinds/blind0/reset";
const char _subscribe_reboot_[] = "blinds/blind0/reboot";*/

// Time for each rolling shutter to go down and up - you need to measure this and configure - default values - can be changed via web (if enabled)
#define _Shutter1_duration_down_ 51200
#define _Shutter1_duration_up_ 51770
#define _Shutter1_duration_tilt_ 1650

//#define _reverse_position_mapping_ 1
#define _auto_hold_buttons_ 1

const char payload_open[] = "open";
const char payload_close[] = "close";
const char payload_stop[] = "stop";


// Change these for your WIFI, IP and MQTT
//char _ssid1_[] = "GG-2.4G";
//char _password1_[] = "FhmX8rjjezmd";
const char _ssid1_[] = "home";
const char _password1_[] = "12345678";
const char WEB_UPGRADE_USER[] = "admin";
const char WEB_UPGRADE_PASS[] = "admin";
//char OTA_password[] = "admin"; // Only accepts [a-z][A-Z][0-9]
const char _mqtt_server_[] = "192.168.0.89";
const char _mqtt_user_[] = "user";
const char _mqtt_password_[] = "pass";

#define WIFI_RETRY_INTERVAL 20000
#define MQTT_RETRY_INTERVAL 10000


const char movementUp[] ="up";
const char movementDown[] = "down";
const char movementStopped[] = "stopped";

#define update_interval_loop 25
//1 seconds
#define update_interval_active 1000
//15 miunutes
#define update_interval_passive 900000

/********************************
SONOFF DUAL R3 PCB ver 1.x, 2.x
GPIO13	Status LED (blue/inverted)
GPIO00	Push Button (inverted)
GPIO27	Relay 1 / LED 1 (red)
GPIO14	Relay 2 / LED 2 (red)
GPIO32	Switch 1 (inverted)
GPIO33	Switch 2 (inverted)
GPIO25	power sensor UART Tx
GPIO26	power sensor UART Rx
**********************************/

#ifdef SHELLY //SHELLY PLUS 2PM
  // Relay GPIO ports
  #define GPIO_REL1 13  // r1 up relay
  #define GPIO_REL2 12   // r1 down relay

  // Buttons GPIO ports
  #define GPIO_KEY1 5  // up button
  #define GPIO_KEY2 18  // down button

  #define BUTTON_PIN 4  //button on the device, inverted, pullup

  #define SLED 0  // Blue light (inverted)

  #define I2C_SDL1_PIN 26 // for external sensors - not used
  #define I2C_SCL1_PIN 25 // for external sensors - not used

  #define KEY_PRESSED  HIGH

#else //SONOFF DUAL R3

  #define GPIO_REL1 27  // r1 up relay
  #define GPIO_REL2 14   // r1 down relay

  #define BUTTON_PIN 4  //button on the device, inverted, pullup

  // Buttons GPIO ports
  #define GPIO_KEY1 32  // up button (inverted)
  #define GPIO_KEY2 33  // down button (inverted)

  #define SLED 13  // Blue light (inverted)

  #define RX_PIN 26 // for external sensors - not used
  #define TX_PIN 25 // for external sensors - not used

  #define ADE7953_IRQ 27 //(power meter)

  #define TEMP_PIN 35 //Internal Temperature

  #define KEY_PRESSED  LOW

#endif

struct topics {
  char publish_position[50];
  char publish_tilt[50];
  char subscribe_command[50];
  char subscribe_position[50];
  char subscribe_tilt[50]; 
  char subscribe_calibrate[50];
  char subscribe_reboot[50];
  char subscribe_reset[50]; // currently not used
};

extern topics mqtt_topics;

struct configuration {
  boolean tilt;
  boolean reverse_position_mapping; // currently not used
  boolean auto_hold_buttons;
  char host_name[25];
  char wifi_ssid1[25];
  char wifi_password1[25];
  char mqtt_server[25];
  char mqtt_user[25];
  char mqtt_password[25];
 /* char publish_position1[50];
  char publish_tilt1[50];
  char subscribe_command1[50];
  char subscribe_position1[50];
  char subscribe_tilt1[50]; 
  char subscribe_calibrate[50];
  char subscribe_reboot[50];
  char subscribe_reset[50]; */// currently not used
  unsigned long Shutter1_duration_down;
  unsigned long Shutter1_duration_up;
  unsigned long Shutter1_duration_tilt;
};

extern configuration cfg,web_cfg;

#endif