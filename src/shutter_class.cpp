#include <Arduino.h>
#include "shutter_class.h"

Button::Button() {
  this->pressed=false;
  this->web_key_pressed=false;
  this->counter=0;
}

/*****************************
 *  C O N S T R U C T O R
****************************/

void Shutter::setup(String name,unsigned long d_down,unsigned long d_up, unsigned long t,byte p_up,byte p_down) {
  this->Name=name;
  this->duration_down = d_down;
  this->duration_up = d_up;
  this->pin_up=p_up;
  this->pin_down=p_down;
  // if not auto restore
  // this->Tilt=0;
  // this->Position=0;
  if (cfg.tilt) {
    this->duration_tilt=t;
    upper_stop_offset=-1000; // how far (ms) to stop the shutter before the top position (negative means go beyond)
  } else {
    this->duration_tilt=1;
    upper_stop_offset=100;  // how far (ms) to stop the shutter before the top position
  } 
}

Shutter::Shutter() {
  this->Name="";
  this->duration_down = 1;
  this->duration_up = 1;
  this->pin_up = 0;
  this->pin_down = 0; 
  this->Tilt=0;
  this->Position = 0;
  this->duration_tilt=1;
  
  this->movement=stopped;
  this->force_update=false;
  this->calibrating=false;
  this->semafor=false;
}

int Shutter::getPosition() {
  return this->Position;
}
void Shutter::setPosition(int p) {
  this->Position=p;
}
int Shutter::getTilt() {
  if (cfg.tilt) {
     return this->Tilt;
  } else {
    return 0;
  }
}
void Shutter::setTilt(int tlt) {
  this->Tilt=tlt;
}

/***************
 *  G O   U P
 ***************/
void Shutter::Start_up() {
  unsigned long now = 0;
  float incr_stop, incr;

  if (this->semafor){
    delay(500);
    if (this->semafor) return;
  }
    if (this->movement == up || this->movement == down) {  // Stop it, if it goes down or all the way up
    this->Stop();
    return;
  }

  this->semafor=true;
  digitalWrite(this->pin_down, LOW);  // Make sure the down relay is off!!!
  this->start_position=this->Position;  
  
  if (cfg.tilt) {
    this->tilt_start=this->Tilt;
    this->tilting=false;
  }

  incr_stop=(float)this->duration_up*((float)(this->Position/100.0))-upper_stop_offset;
  now = millis();
  this->timestamp_start=now;
  this->full_stop=now+(unsigned long) incr_stop;

  // Go all the way up
  incr=incr_stop;    
  this->movement=up;
  #ifdef DEBUG
    Serial.printf("%s go all the way up\n",this->Name.c_str());
  #endif

  this->auto_stop=now+(unsigned long)incr;
  digitalWrite(this->pin_up, HIGH);  // Turn on up relay
  this->semafor=false;
}


/************************
 *    G O   D O W N
 ************************/
void Shutter::Start_down() {
    unsigned long now = 0;
    float incr_stop,incr;

  if (this->semafor){
    delay(500);
    if (this->semafor) return;
  }
  if (this->movement == down || this->movement == up) {  // Stop it if it goes up or all the way down
    this->Stop();
    return;
  }

  this->semafor=true;
  digitalWrite(this->pin_up, LOW);  // Make sure the up relay is off
  this->start_position=this->Position;

  if (cfg.tilt) {
    this->tilt_start=this->Tilt;
    this->tilting=false;
  }
  incr_stop=(float)this->duration_down*((100.0-(float)this->Position)/100.0)+lower_stop_offset;
  now = millis();
  this->timestamp_start=now;
  this->full_stop=now+(unsigned long) incr_stop;

 // Go all the way down
    incr=incr_stop;    
    this->movement=down;
    #ifdef DEBUG
      Serial.printf("%s go all the way down\n",this->Name.c_str());
    #endif
  
  this->auto_stop=now+(unsigned long)incr;

  digitalWrite(this->pin_down, HIGH);  // Turn on down relay
  this->semafor=false;
}

/*********************************
 *  C A L I B R A T E
 **********************************/
void Shutter::Calibrate() {

  unsigned long now = 0;  
  
  if (this->semafor){
    delay(500);
    if (this->semafor) return;
  }

  if (this->movement != stopped ) {  // Stop if it is not stopped already
    this->Stop();
  }

  this->semafor=true;
  digitalWrite(this->pin_up, LOW);  // Make sure relay up is off
  this->start_position=this->Position;
  this->movement=down;
  now = millis();
  this->timestamp_start=now;
  this->auto_stop=now+(unsigned long)duration_down+5000; // Go all the way down (ignoring current position)  + 5 second
  this->calibrating=true;
  digitalWrite(this->pin_down, HIGH);  // Turn on down relay
  this->semafor=false;
}


/*********************************
 *  G O   T O   P O S I T I O N
**********************************/
void Shutter::Go_to_position(int p) {
  int po;
  float incr;
  unsigned long now = 0;

  if (this->semafor){
    delay(500);
    if (this->semafor) return;
  }
  po=constrain(p,0,100);

  if (po==this->Position) return;
  if (this->movement != stopped) this->Stop();

  this->semafor=true;
  this->start_position=this->Position;
  now = millis();
  this->timestamp_start=now;
  if (cfg.tilt) {
    this->tilt_start=this->Tilt;
    this->tilting=false;
  }

  if (po<this->Position) { // will go up
    incr=(float)this->duration_up*(((float)this->Position-(float)po)/100.0);  
    this->movement=up;
    this->auto_stop=now+(unsigned long)incr;
    digitalWrite(this->pin_down, LOW);  // Make sure relay down is off
    digitalWrite(this->pin_up, HIGH);  // Turn on up relay
  } else {
    incr=(float)this->duration_down*(((float)po-(float)this->Position)/100.0);
    if (po==100) incr+=lower_stop_offset;    
    this->movement=down;    
    this->auto_stop=now+(unsigned long)incr;
    digitalWrite(this->pin_up, LOW);  // Make sure relay up is off
    digitalWrite(this->pin_down, HIGH);  // Turn on down relay
  }
  this->semafor=false;
}


/***********************************
 *  U P D A T E   P O S I T I O N
 ***********************************/
void Shutter::Update_position() {

  if (this->movement==stopped) return;
  unsigned long now = millis();
  if (this->semafor) return;

  if (this->movement==up ) { // G O I N G   U P
    float change_percentage = (float)(now-this->timestamp_start)/ (float)this->duration_up * 100.0;
    int current_percentage = this->start_position - (int)change_percentage;
    this->Position=constrain(current_percentage,0,100);
    if (cfg.tilt) {
      // tilt 0 (open), 90 (closed)
      // Going up, tilt openning
      change_percentage = (float)(now-this->timestamp_start)/ (float)this->duration_tilt * 100.0;
      current_percentage= this->tilt_start-int(change_percentage);
      this->Tilt=constrain(current_percentage,0,90);
    }
    if (( (now >= this->auto_stop) || (now < this->timestamp_start)) && !btnUp.pressed) {
        this->Stop();
        #ifdef DEBUG
          Serial.printf("%s Auto stop timer checkpoint 1\n",this->Name.c_str());
        #endif
    }        
  } else { // G O I N G   D O W N
    float change_percentage = (float)(now-this->timestamp_start) / (float)this->duration_down * 100.0;

    int current_percentage = this->start_position + (int)change_percentage;
    this->Position=constrain(current_percentage,0,100);    
    if (cfg.tilt) {
      // tilt 0 (open), 90 (closed)
      // Going down, closing tilt
      change_percentage = (float)(now-this->timestamp_start)/ (float)this->duration_tilt * 100.0;
      current_percentage= this->tilt_start+int(change_percentage);
      this->Tilt=constrain(current_percentage,0,90);
    }
    if (( (now >= this->auto_stop) || (now < this->timestamp_start)) && !btnDown.pressed) {
        this->Stop();
        #ifdef DEBUG
          Serial.printf("%s Auto stop timer checkpoint 2\n",this->Name.c_str());
        #endif
       if (this->calibrating) {
          this->calibrating=false;
          this->Go_to_position(this->start_position);
       }
    }
  }
}


/*********************************
 *  T I L T
**********************************/
void Shutter::tilt_it(int tlt) {
  int t;
  float change_duration;
  unsigned long now = 0;

  if (this->semafor){
    delay(500);
    if (this->semafor) return;
  }
  
  t=constrain(tlt,0,90);
  if (t==this->Tilt) return;
  if (this->movement != stopped) this->Stop();

  this->semafor=true;
  this->start_position=this->Position;
  this->tilt_start=this->Tilt;
  this->tilting=true;
  now = millis();
  this->timestamp_start=now;
    
  if (t<this->Tilt) { // Need to open - go up
    digitalWrite(this->pin_down, LOW);  // Make sure relay down is off
    change_duration=(float)this->duration_tilt*(((float)this->Tilt-(float)t)/100.0);
    this->movement=up;
    this->auto_stop=now+(unsigned long)change_duration;
    digitalWrite(this->pin_up, HIGH);  // Turn on up relay
  } else { // Need to close - go down
    digitalWrite(this->pin_up, LOW);  // Make sure relay up is off
    change_duration=(float)this->duration_tilt*(((float)t-(float)this->Tilt)/100.0);
    this->movement=down;
    this->auto_stop=now+(unsigned long)change_duration;
    digitalWrite(this->pin_down, HIGH);  // Turn on down relay
  }  
  this->semafor=false;
}

/*********************************
 *  S T O P
**********************************/
// Stop and set tilt to start tilt (if position is not 0 or 100, with sume tollerance)
void Shutter::Stop() {
  unsigned long now = 0;
  if (this->semafor){
    delay(500);
    if (this->semafor) return;
  }
  this->semafor=true;
  now = millis();
  digitalWrite(this->pin_up, LOW);  // Turn off
  digitalWrite(this->pin_down, LOW);  // Turn off
  this->force_update=true; // Make sure the MQTT last position is sent before it goes to long updating period
  this->movement=stopped;
  this->semafor=false;
  if (cfg.tilt && this->Position != 0 && this->Position!=100 && !this->tilting && this->Tilt != this->tilt_start && ((unsigned long)(now-btnUp.pressed_timestamp)>this->duration_tilt) && ((unsigned long)(now-btnDown.pressed_timestamp)>this->duration_tilt) ) {
    this->tilt_it(tilt_start);
  }
}


void Shutter::Process_key(boolean key_up, boolean key_down) {
  unsigned long now = millis();

  // If the button is pressed longer than 1 minute, release it (will probably work mainly for web)
  if (this->btnUp.pressed && ((unsigned long)(now-btnUp.pressed_timestamp)>max_hold_time)) {
    key_up=false;
    btnUp.web_key_pressed=false;
  }
  if (this->btnDown.pressed && ((unsigned long)(now-btnDown.pressed_timestamp)>max_hold_time)) {
    key_down=false;
    btnDown.web_key_pressed=false;
  }
  
  if ((key_up || btnUp.web_key_pressed)!=this->btnUp.pressed) { //a new press or release event on UP button
    if (key_up || btnUp.web_key_pressed) { // Button up pressed
      // need to go up, new button pressed state
      this->btnUp.counter++;
      this->btnUp.pressed_timestamp=now;
      this->btnUp.pressed=true;
      this->Start_up();
      return; // Ignore if button down is pressed
    } else { // Button up released
      if (this->movement != stopped) {// shutter is moving
        //if no autohold, or tilt with shorter press than tilt duration, or no tilt and longer press than 1s will stop the blind movement
        if ( !cfg.auto_hold_buttons || (cfg.tilt && ((unsigned long)(now - this->btnUp.pressed_timestamp)<this->duration_tilt)) || (!cfg.tilt && ((unsigned long)(now - this->btnUp.pressed_timestamp)>hold_button_delay)) ) {
          this->Update_position();
          this->Stop();
          #ifdef DEBUG
            Serial.printf("%s Stop release button checkpoint 1\n",this->Name.c_str());
          #endif
        }
      }
      this->btnUp.pressed=false;
    }
  }
  if ((key_down || btnDown.web_key_pressed) !=this->btnDown.pressed) { //a new press or release event on DOWN button
    if (key_down || btnDown.web_key_pressed) { // Button down pressed
        // pojedeme dolu
        //if (btnDown.web_key_pressed) this->btnDown.counter++;
        this->btnDown.counter++;
        this->btnDown.pressed_timestamp=now;
        this->btnDown.pressed=true;
        this->Start_down();
    } else { // Button down released
      if (this->movement != stopped) {
        //if no autohold, or tilt with shorter press than tilt duration, or no tilt and longer press than 1s will stop the blind movement
        if (!cfg.auto_hold_buttons || (cfg.tilt && ((unsigned long)(now - this->btnDown.pressed_timestamp)<this->duration_tilt)) || ( !cfg.tilt && ((unsigned long)(now - this->btnDown.pressed_timestamp)>hold_button_delay))) { 
          this->Update_position();//update position before stop
          this->Stop();
          #ifdef DEBUG
            Serial.printf("%s Stop release button checkpoint 2\n",this->Name.c_str());
          #endif
        }
      }
      this->btnDown.pressed=false;
    }
  }
}

const char *Shutter::Movement() {
  switch (this->movement) {
    case up:
      return(movementUp);
    case down:
      return(movementDown);
    default:
      return(movementStopped);
  }  
}