#ifndef WEB_H
#define WEB_H

#include <WebServer.h>
#include <HTTPUpdateServer.h>

class WebPage{
  public:
    WebPage(WebServer *server);
    void readMain();
    void readConfig();
    void pressButton();
    void handleRootPath();
    void handleConfigurePath();
    void handleUpgradePath();
    void updateConfig();
    void setup();

  public:
    unsigned long lastUpdate = 0; // timestamp - last MQTT update
    unsigned long lastCallback = 0; // timestamp - last MQTT callback received
    unsigned long lastWiFiDisconnect=0;
    unsigned long lastWiFiConnect=0;
    unsigned long lastMQTTDisconnect=0; // last time MQTT was disconnected
    unsigned long WiFiLEDOn=0;
    unsigned long k1_up_pushed=0;
    unsigned long k1_down_pushed=0;

    String lastCommand = "";
    String crcStatus="";

  private:
    void Restart();
    int WifiGetRssiAsQuality(int rssi);
    void timeDiff(char *buf,size_t len,unsigned long lastUpdate);
  private:
    WebServer* _server;
};


/***************/
/*             */
/*   M A I N   */
/*             */
/***************/
//const char MAIN_page[] PROGMEM = R"#(
const char MAIN_page[] = R"#(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
.tilt {
  display: none;
}
.remote_control {
  display:grid;
  width: 40em;
  grid-template-columns: 1fr 1fr 10px 1fr 1fr;
  grid-template-rows:   2.5em 2.5em 3.5em;
  grid-template-areas: 
    "h1 h2 . h3 h4"
    "k1 k2 . k3 k4"
    "b1 b2 . b3 b4";
}
.status {
  display:grid;
  width: 20em;
  grid-template-columns: 6.3em 1fr 1fr 5px;
  grid-template-rows: 2.5em 2.5em 2.5em 2.5em;
}
.commands {
  display:grid;
  grid-template-columns: 160px 160px 160px 160px;
}
@media only screen and (max-width: 700px) {
  .remote_control {
    display:grid;
    width: 20em;
    grid-template-columns: 1fr 1fr;
    grid-template-rows:   2.5em 2.5em 3.5em 10px 2.5em 2.5em 3.5em;
    grid-template-areas: 
      "h1 h2"
      "k1 k2"
      "b1 b2"
      ". ."
      "h3 h4"
      "k3 k4"
      "b3 b4";    
  }
  .status {
    display:grid;
    width: 20em;
    grid-template-columns: 6.3em 1fr 1fr;
    grid-template-rows: 2.5em 2.5em 2.5em 2.5em;
  }
  .commands {
    display:grid;
    grid-template-columns: 1fr 1fr;
  }
}
html {
  box-sizing: border-box;
}
*, *:before, *:after {
  box-sizing: inherit;
}
body {
  padding: 20px; 
  background-color: #232323;
  font-family: Verdana, "Times New Roman", Times, serif;
  font-size: 100%; 
  color: white;
}
input, label, div {
  display: flex;
  align-items: center;
  justify-content: center;
  height:100%;
  width: 100%;
  border: 1px solid grey;
  font-size: 1em;
}
.description { grid-column: 1/2; }
.s1 { grid-column: 2/3; }
.s2 { grid-column: 3/4; }
.h1 { grid-area: h1; }
.h2 { grid-area: h2; }
.h3 { grid-area: h3; }
.h4 { grid-area: h4; }
.k1 { grid-area: k1; }
.k2 { grid-area: k2; }
.k3 { grid-area: k3; }
.k4 { grid-area: k4; }
.b1 { grid-area: b1; }
.b2 { grid-area: b2; }
.b3 { grid-area: b3; }
.b4 { grid-area: b4; }
p {
  font-size: 0.875em;
}
h1, h2 { 
  font-family: "Bahnschrift Condensed", sans-serif; 
}
h1 { 
  color: #1fa2ec;
  font-size: 2em; 
}
h2 { 
  color:khaki;
  margin-top: 29px; 
  margin-bottom: 5px;
  font-size: 1.5 em;  
}
.topic { font-weight: bold; }
.button {
  background-color: #1fa3ec;
  border-radius: 0.3rem;
  transition-duration: 0.4s;
  cursor: pointer;
  margin: 5px;
  border: 0px;
  color: white;
  padding: 7px 15px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
}
.reset{
  background-color: #d43535;
  border-radius: 0.3rem;
  transition-duration: 0.4s;
  cursor: pointer;
  margin: 5px;
  border: 0;
  color: white;
  padding: 7px 15px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
}

</style>

<script>
function pushButton(b) {
  var request = new XMLHttpRequest();
  request.onreadystatechange = function() { 
    if (this.readyState == 4 && this.status == 200) {
    }
  };
  request.open("GET", "pressButton?button="+b, true);
  request.send();
}

setInterval(function() {
  // Call a function repetatively with 0.5 Second interval
  readMain();
}, 1000); //500mSeconds update rate

function enableStyle(unique_title) {
  var css=document.styleSheets[0];
  for(var i=0; i<css.cssRules.length; i++) {
    var rule = css.cssRules[i];
      if (css.cssRules[i].cssText.includes(unique_title)) {
        return;
      }
  }
  css.insertRule(unique_title+' {display:none;}',0);
}
function disableStyle(unique_title) {
  var css=document.styleSheets[0];
  for(var i=0; i<css.cssRules.length; i++) {
    var rule = css.cssRules[i];
      if (css.cssRules[i].cssText.includes(unique_title)) {
        css.deleteRule(i);
        return;
      }
  }
}

function readMain() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {        
      var resp= JSON.parse(this.responseText);
      document.title=resp.device;
      document.getElementById("device").innerHTML=resp.device;
      document.getElementById("mqtt").innerHTML=resp.mqtt;
      document.getElementById("mqttmsg").innerHTML=resp.mqttmsg;
      document.getElementById("disconnect").innerHTML=resp.disconnect;
      document.getElementById("crc").innerHTML=resp.crc;
      document.getElementById("mem").innerHTML=resp.mem;
      document.getElementById("wifi").innerHTML=resp.wifi;
      document.getElementById("strength").innerHTML=resp.strength+" %";
      document.getElementById("ip").innerHTML=resp.ip;
      document.getElementById("update").innerHTML=resp.update;
      for(i=1;i<=2;i++) {
        document.getElementById("key"+i).innerHTML = resp.keys[i-1];
        if(resp.keys[i-1]=='Pressed') {
          document.getElementById("key"+i).style.background = '#f8aaaa';          
          document.getElementById("key"+i).style.color= 'black';
        } else {
          document.getElementById("key"+i).style.background = '#76ec76';
          document.getElementById("key"+i).style.color = 'black';
        }
      }
      for(i=1;i<=1;i++) {
        if(resp.movement[i-1]=='stopped') {
          document.getElementById("movement"+i).style.background = '#76ec76';
          document.getElementById("movement"+i).style.color = 'black';
        } else {
          document.getElementById("movement"+i).style.background = '#f8aaaa';
          document.getElementById("movement"+i).style.color= 'black';
        }
        document.getElementById("movement"+i).innerHTML=resp.movement[i-1];
        document.getElementById("position"+i).innerHTML=resp.position[i-1]+" %";
        document.getElementById("tilt"+i).innerHTML=resp.tilt[i-1]+" °";
      }

      if (resp.tilting=="true")
        disableStyle(".tilt");
      else
        enableStyle(".tilt");
    }
  };
  xhttp.open("GET", "readMain", true);
  xhttp.send();
}
</script>


</head>
<html>
<body>
<header><h1 id="device"></h1></header>

<h2>Sensors</h2>
<section class="remote_control"> 
  <div class="h1">Shutter 1 UP</div>
  <div class="h2">Shutter 1 DOWN</div>
  <div class="k1" id="key1"></div>
  <div class="k2" id="key2"></div>
  <div class="b1"><button type="button" class="button" onmousedown="pushButton(1)" onmouseup="pushButton(11)">▲</button></div>
  <div class="b2"><button type="button" class="button" onmousedown="pushButton(2)" onmouseup="pushButton(12)">▼</button></div>
</section>

<h2>Shutters</h2>  
<section class="status">  
  <div class="description"></div>
  <div class="s1">Shutter 1</div>
  <div class="description">movement</div>
  <div id="movement1" class="s1"></div>
  <div class="description">position</div>
  <div id="position1" class="s1"></div>
  <div class="description tilt">Tilt</div>
  <div id="tilt1" class="s1 tilt"></div>
</section>

<h2>Connectivity</h2>
<section>
  <p><span class="topic">SSID:</span> <span id="wifi"></span></p>
  <p><span class="topic">Signal strenght:</span> <span id="strength"></span></p>
  <p><span class="topic">IP address:</span> <span id="ip"></span></p>
</section>

<h2>MQTT</h2>
<section>
  <p><span class="topic">Status:</span> <span id="mqtt"></span></p>
  <p><span class="topic">Last received message:</span> <span id="mqttmsg"></span></p>
  <p><span class="topic">Last update:</span> <span id="update"></span></p>
  <p><span class="topic">Last loss of WiFi,MQTT:</span> <span id="disconnect"></span></p>
  <p><span class="topic">Boot CRC check:</span> <span id="crc"></span></p>
  <p><span class="topic">Free memory:</span> <span id="mem"></span></p>
</section>

<h2>Commands</h2>
<section class="commands">
  <button type="button" class="reset" onclick="location.href='/configure';">Configure</button>
  <button type="button" class="reset" onclick="location.href='/upgrade';">Upgrade</button>
  <button type="button" class="reset" onmouseup="pushButton(55)">Calibrate</button>
  <button type="button" class="reset" onmouseup="pushButton(66)">Restart</button>
</section><br />
 
<footer><h6>Last code change: )#" __DATE__ " " __TIME__  R"#(</h6></footer>

</body>
</html>
)#";

/*************************/
/*                       */
/*   C O N F I G U R E   */
/*                       */
/*************************/
const char CONFIGURE_page[] PROGMEM = R"#(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
.tilt { 
  display: none;
}

.wifi_multi { 
  display: none;
}
.container {
  display:grid;
  grid-template-columns: 1em 25px 10em96em 7em 3em 7em;
  grid-auto-rows: auto;
}
label.description,label.first,label.second,.checkbox {
  align-self: center;
}
.checkbox {
  grid-column: 4/4;
  height:18px;
}
.description {
  grid-column: 3/4;
}
.first, .full {
  grid-column: 4/6;
}
.second { 
  grid-column: 6/8;
}
.header {
  grid-column: 2/-1;
}
.commands {
  display:grid;
  grid-template-columns: 160px 160px 160px 160px;
}
input, select {
    height: 2.2em;
    border: none;
    border-radius: 4px;
    font-size: 15px;
    margin: 0;
    outline: 0;
    padding: 10px;
    width: 85%;
    background-color: #e8eeef;
    color: #444444;
    margin-bottom: 10px;
}
label {
  height: 1.2em;
}
@media only screen and (max-width: 400px) {
  .container {
    grid-template-columns: 0px 22px 8em 1em 1fr 1em;
  }
  .full {
    grid-column: 4/6;
  }
  .first {
    grid-column: 4/6;
  }
  .second { 
    grid-column: 5/7;
  }
  .commands {
    grid-template-columns: 1fr 1fr;
  }
}
@media only screen and (min-width: 401px) {
  .container {
    grid-template-columns: 0px 22px 8em 1em 12em 1em;
  }
  .full {
    grid-column: 4/6;
  }
  .first {
    grid-column: 4/6;
  }
  .second { 
    grid-column: 5/7;
  }
  .commands {
    grid-template-columns: 1fr 1fr;
  }
}
html {
  box-sizing: border-box;
}
*, *:before, *:after {
  box-sizing: inherit;
}
body {
  padding: 20px; 
  background-color: #232323;
  font-family: Verdana, "Times New Roman", Times, serif;
  font-size: 100%; 
  color: white;
} 

h1 { 
  color: #1fa2ec;
  font-size: 2em; 
}
h2 { 
  color:khaki;
  margin-top: 29px; 
  margin-bottom: 5px;
  font-size: 1.5 em;  
}
.topic { font-weight: bold; }
.button {
  background-color: #1fa3ec;
  border-radius: 0.3rem;
  transition-duration: 0.4s;
  cursor: pointer;
  margin: 5px;
  border: 0;
  color: white;
  padding: 7px 15px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
}
.reset{
  background-color: #d43535;
  border-radius: 0.3rem;
  transition-duration: 0.4s;
  cursor: pointer;
  border: 0;
  margin: 5px;
  color: white;
  padding: 7px 15px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
}
</style>

<script>
function pushButton(b) {
  var request = new XMLHttpRequest();
  request.onreadystatechange = function() { 
    if (this.readyState == 4 && this.status == 200) {
    }
  };
  request.open("GET", "pressButton?button="+b, true);
  request.send();
  if(b==77)
    readConfig();
  if(b==88) {
    location.href='/';
  }
}

function enableStyle(unique_title) {
  var css=document.styleSheets[0];
  for(var i=0; i<css.cssRules.length; i++) {
    var rule = css.cssRules[i];
      if (css.cssRules[i].cssText.includes(unique_title)) {
        return;
      }
  }
  css.insertRule(unique_title+' {display:none;}',0);
}
function disableStyle(unique_title) {
  var css=document.styleSheets[0];
  for(var i=0; i<css.cssRules.length; i++) {
    var rule = css.cssRules[i];
      if (css.cssRules[i].cssText.includes(unique_title)) {
        css.deleteRule(i);
        return;
      }
  }
}

function sendData(field,value) {
  var request = new XMLHttpRequest();
  request.onreadystatechange = function() { 
    if (this.readyState == 4 && this.status == 200) {
    }
  };
  
  if (field=="tilt") {
    if (value)
      disableStyle(".tilt");
    else
      enableStyle(".tilt");
  }
  request.open("GET", "updateField?field="+field+"&value="+value, true);
  request.send();
}

function sendConfig()
{
  var data={
      "host_name":"",
      "tilt":"true",
      "auto_hold_buttons":"true",
      "wifi_ssid1":"",
      "wifi_password1":"",
      "mqtt_server":"",
      "mqtt_user":"",
      "mqtt_password":"",
      "shutter_duration_down":"",
      "shutter_duration_up":"",
      "shutter_duration_tilt":""};

  data.host_name= document.getElementById("host_name").value;
  data.tilt=document.getElementById("tilt").checked;
  data.auto_hold_buttons=document.getElementById("auto_hold_buttons").checked;
  data.wifi_ssid1= document.getElementById("wifi_ssid1").value;
  data.wifi_password1= document.getElementById("wifi_password1").value;
  data.mqtt_server= document.getElementById("mqtt_server").value;
  data.mqtt_user= document.getElementById("mqtt_user").value;
  data.mqtt_password= document.getElementById("mqtt_password").value;
  data.shutter_duration_down= document.getElementById("shutter_duration_down").value;
  data.shutter_duration_up= document.getElementById("shutter_duration_up").value;
  data.shutter_duration_tilt= document.getElementById("shutter_duration_tilt").value;
  
  var request = new XMLHttpRequest();
  request.onreadystatechange = function() { 
    if (this.readyState == 4 && this.status == 200) {
    }
  };
  request.open("POST", "updateConfig");
  request.setRequestHeader("Content-Type", "application/json;charset=UTF-8");
  request.send(JSON.stringify(data));
  location.href='/';
}

function readConfig() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var resp= JSON.parse(this.responseText);
      document.getElementById("host_name").value = resp.host_name;
      document.getElementById("auto_hold_buttons").checked = resp.auto_hold_buttons=="true";
      document.getElementById("tilt").checked = resp.tilt=="true";
      document.getElementById("wifi_ssid1").value = resp.wifi_ssid1;
      document.getElementById("wifi_password1").value = resp.wifi_password1;
      document.getElementById("mqtt_server").value = resp.mqtt_server;
      document.getElementById("mqtt_user").value = resp.mqtt_user;
      document.getElementById("mqtt_password").value = resp.mqtt_password;
      document.getElementById("shutter_duration_down").value = resp.shutter_duration_down;
      document.getElementById("shutter_duration_up").value = resp.shutter_duration_up;
      document.getElementById("shutter_duration_tilt").value = resp.shutter_duration_tilt;
    
      if (resp.tilt=="true")
        disableStyle(".tilt");
      else
        enableStyle(".tilt");         
      
      if (resp.wifi_multi=="true")
        disableStyle(".wifi_multi");
      else
        enableStyle(".wifi_multi");      
    }
  };  
  xhttp.open("GET", "readConfig", true);
  xhttp.send();
}
</script>


</head>
<html>
<body onload="readConfig();">
<header><h1 class="header" id="device">Configuration</h1></header>
<section class="container">
  <label class="description" for="host_name">Name</label> <input class="full" type="text" name="host_name" id="host_name">
</section>

<h2>Shutter type</h2>
<section class="container">
<label class="description" for="tilt">Tilt</label><input class="checkbox" type="checkbox" name="tilt" id="tilt"> 
 <label class="description" for="auto_hold_buttons">Auto hold buttons</label> <input class="checkbox" type="checkbox" name="auto_hold_buttons" id="auto_hold_buttons">
</section>

<h2>WiFi</h2>
<section class="container">
  <label class="description" for="wifi_ssid1">SSID 1</label> <input class="full" type="text" maxlength="24" name="wifi_ssid1" id="wifi_ssid1">
  <label class="description" for="wifi_password1">password 1</label> <input class="full" type="text" maxlength="24" name="wifi_password1" id="wifi_password1">
</section>
  
<h2>MQTT</h2>
<section class="container">
  <label class="description" for="mqtt_server">Server</label> <input class="full" type="text" maxlength="24" name="mqtt_server" id="mqtt_server">
  <label  class="description" for="mqtt_user">User</label> <input class="full" type="text" maxlength="24" name="mqtt_user" id="mqtt_user">
  <label  class="description" for="mqtt_password">Password</label> <input class="full" type="text" maxlength="24" name="mqtt_password" id="mqtt_password">
</section>  

<h2>Parameters</h2>
<section class="container">
  
  <span class="description">Duration down</span>
  <div class="first"><input type="number" min="0" max="120000" name="shutter_duration_down" id="shutter_duration_down"> ms</div>
  
  <span class="description">Duration up</span>
  <div class="first"><input type="number" min="0" max="120000" name="shutter_duration_up" id="shutter_duration_up"> ms</div>
  
  <span class="description tilt">Duration tilt</span>
  <div class="first tilt"><input type="number" min="0" max="120000" name="shutter_duration_tilt" id="shutter_duration_tilt"> ms</div>
</section>
  
<br />

<section class="commands">
<button type="button" class="reset" onclick="location.href='/';">Back</button>
<button type="button" class="reset" onmouseup="pushButton(77)">Load defaults</button>
<button type="button" class="reset" onmouseup="sendConfig()">Save and restart</button>
</section>
</body>
</html>
)#";

#endif