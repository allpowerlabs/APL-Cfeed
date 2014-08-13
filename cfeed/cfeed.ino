 /*
APL Continuous Feed Controller

This code lives on an Arduino Uno that operates a prototype automatic valve for maintaining a constant
level of solid fuel in the hopper of a power pallet. 
*/

#include <EEPROM.h>

//{ CFeed2 board pinout 
#define ValveOpenPin	A0
#define ValveClosePin	A2
#define FillPin       A5
#define AlarmPin      2

#define JamLED     11
#define NoValveLED 13
#define NoFillLED  12
#define BridgeLED  A4
#define LockedLED  A3

#define UpperSens  6
#define LowerSens  5
#define OpenSens   7
#define ClosedSens 8
#define ValveBTN   10
#define ClearBTN   9
#define CurrentSens A1
//}

//{ State Enumeration
#define Closed_state 0
#define Opening_state 1
#define Open_state 2
#define Closing_state 3
#define Filling_state 4
#define WaitingToClose_state 5
#define TryToClear_valve 6
//}

//{ Timings in milliseconds
#define MaxMotorTime 26000
#define OpenInrushTime 150
#define CloseInrushTime 50
#define MaxFillTime  1200000  // 1200s = 20m
#define WaitToCloseTime 2000
#define DebounceTime 50
//}

//{ Other #defines
#define CurrentThreshold 70
#define HighPowerCurrentThreshold 100
#define L_ON 0
#define L_OFF 1
#define locked_EE_address 0
//}

//{ Variables
int state = 0;
int current = 0;
long duration = 0;
boolean locked  = 0;
boolean last_locked = 0; 
boolean bridged = 0;
boolean debug = true;
boolean valve_btn_press = 0;
unsigned long start_time = 0;
//}




void setup() {
  Serial.begin(57600);
  pinMode(ValveOpenPin, OUTPUT);  
  pinMode(ValveClosePin, OUTPUT);  
  pinMode(FillPin, OUTPUT);  
  
  pinMode(JamLED, OUTPUT);   // 
  pinMode(NoValveLED, OUTPUT);  // 
  pinMode(NoFillLED, OUTPUT);  // 
  pinMode(BridgeLED, OUTPUT);  // 
  pinMode(LockedLED, OUTPUT);  // 
  
  pinMode(UpperSens, INPUT);  
  pinMode(LowerSens, INPUT); 
  pinMode(OpenSens, INPUT);
  pinMode(ClosedSens, INPUT);
  pinMode(ValveBTN, INPUT);  
  pinMode(ClearBTN, INPUT);  
	
  //////////////////////////////////////////////////////////////////////////////////////
  // Do a nice little light show to indicate that setup is running for
  // those who may not have a serial monitor connected. the delay() calls
  // give us a little time for the main power to come on and sensors to accurately 
  // read initial valve position. 
  /////////////////////////////////////////////////////////////////////////////////
  // TODO: Flash firmware version number with the LEDs on startup. having units deployed
  // at different develoment cycles will potentially become confusing. 
  for (int i=0; i<3; i++) {  
    digitalWrite(JamLED, L_ON);
    delay(100);
    digitalWrite(JamLED, L_OFF);

    digitalWrite(NoValveLED, L_ON);
    delay(100);
    digitalWrite(NoValveLED, L_OFF);
 		
    digitalWrite(NoFillLED, L_ON);
    delay(100);
    digitalWrite(NoFillLED, L_OFF);

    digitalWrite(BridgeLED, L_ON);
    delay(100);
    digitalWrite(BridgeLED, L_OFF);
	
    digitalWrite(LockedLED, L_ON);	
    delay(100);
    digitalWrite(LockedLED, L_OFF);		
  }  
  digitalWrite(NoValveLED, L_OFF);  // i'm not sure this line is necessary


  // Make sure nothing is moving
  digitalWrite(ValveOpenPin, LOW);
  digitalWrite(ValveClosePin, LOW);

  ////////////////////////////////////////////////////////////////////////////////	
  // Read the proximity sensors to determine the state of the valve
  // and the appropriate action to take at power-up. 
  ////////////////////////////////////////////////////////////////////////////
  // TODO: Noticing issues when the board is booted before the 12V valve main wile debug. Must press buttons a few times.
  if(debug) {Serial.println("Initial valve close check");}
   
  locked = EEPROM.read(locked_EE_address);    // read the stored locked state from last time we had power. 
  last_locked = locked;
  digitalWrite(LockedLED, (!locked));         // update LED state

  if (digitalRead(ClosedSens))    // if the closed sensor is lit, we're already closed, so wake in closed state.
    state = Closed_state;

  else if(locked)                 // if the valve is not closed, but we were locked before, stay put (open state). 
    state = Open_state;
	
  else                            // if the valve isn't closed, and we weren't locked before power cycle, try to close. 
    StartClosing();
  
  
  //////////////////////////////////////////////////////////////////////////////////
  // To initialize main automatic operation user must press reset, to clear the 
  // power up state. The reason behind this is that if the unit loses power and restarts
  // it may be nice to know that the unit lost power, or if someone is working on it
  // when it suddenly regains power having automatic mode start would be unfavourable.
  // This behaviour can be easily modified. 
  ///////////////////////////////////////////////////////////////////////////
  
}


void loop() {
  CheckState();
  CheckForBridging();
  CheckLocked();
  
  
  
  // send alarm for critical conditions.
  //digitalWrite(AlarmPin, (JamLED || NoValveLED || NoFillLED));	
  
  CheckButtons();
}

void CheckLocked() {                    // This function updates the LED and EEPROM mirrors of the locked bit. 
  if(locked == last_locked)             // only run if locked has changed. (to preserve EEPROM)
    return;
  last_locked = locked;                 // update last_locked to new value. 
  digitalWrite(LockedLED, (!locked));   // update LED state
  EEPROM.write(locked_EE_address, locked);     // store new value in EEPROM
}

void CheckState() {
static bool state_changed = 0; 
static int last_state = 0;
state_changed = (state != last_state);
last_state = state;

  switch (state) {
    case Closed_state:
    if(debug && state_changed) {Serial.println("State:Closed");}
    Closed();
    break;
    
    case Opening_state: 
    if(debug && state_changed) {Serial.println("State:Opening");}
    Opening();    
    break;
    
    case Open_state:
    if(debug && state_changed) {Serial.println("State:Open");}
    Open();
    break;
    
    case Closing_state: 
    if(debug && state_changed) {Serial.println("State:Closing");}
    Closing();
    break;
    
    case Filling_state: 
    if(debug && state_changed) {Serial.println("State:Filling");}
    Filling();
    break;
    
    case WaitingToClose_state: 
    if(debug && state_changed) {Serial.println("State:WaitingToClose");}
    WaitingToClose();
    break;

    /* case TryToClear_valve: 
    if(debug && state_changed) {Serial.println("State:TryToClear");}
    TryToClearValveJam();
    break; */
    
    default:
    state = 0;
  }
}

void CheckForBridging() {
  if (digitalRead(UpperSens) && !digitalRead(LowerSens)) {
    bridged = 1;
    digitalWrite(BridgeLED, L_ON);
  }
  else  {
    bridged = 0;
    digitalWrite(BridgeLED, L_OFF);
  }
}

void CheckButtons() {
  
  static boolean last_sample;
  static boolean this_sample;
  static boolean valve_btn_state;
  static unsigned long last_edge_time;
  
  if (digitalRead(ClearBTN)) {
    locked = false;
    digitalWrite(NoValveLED, L_OFF);
    digitalWrite(NoFillLED, L_OFF);
    digitalWrite(LockedLED, L_OFF);	
    digitalWrite(JamLED, L_OFF);
    digitalWrite(BridgeLED, L_OFF);
  }
  
  // check Valve button.  double-latch: 1: debounce 2: require release before reset
  last_sample = this_sample;
  this_sample = digitalRead(ValveBTN);
      
  if (this_sample != last_sample) {
    last_edge_time = millis();
    return;
  }
   
  if ((this_sample != valve_btn_state) && ((millis() - last_edge_time) > DebounceTime)) {
    // the physical button state has changed and it's been at the new state for longer
    // than the debounce delay, so take it as the actual current state:
    valve_btn_state = this_sample;

    // only set the "press" variable if the btn_state just changed to 1. 
    valve_btn_press |= valve_btn_state;
  }
}
    
    
/////////////////////////////////////////////////////////////////////////////////
// Ready to start movin and shakin. and hopefully closing too
///////////////////////////////////////////////////////////////////////////////
void Opening() {
  duration = millis() - start_time;
  current = analogRead(CurrentSens); 
  
  if (valve_btn_press) {              // button pressed 
    // move to open state
    valve_btn_press = 0;
		digitalWrite(ValveOpenPin, LOW);	// why was this removed?
    state = Open_state;
    locked = 1;
    return;
  }
  
  if (duration < OpenInrushTime) {        // inrush phase, keep going    
    return;
  }

  if (duration > MaxMotorTime) { // Valve time-out
    if(debug) {Serial.println("Duration timeout. Duration = "); }
    digitalWrite(NoValveLED, L_ON);
    digitalWrite(ValveOpenPin, LOW);
    locked = true;
    StartClosing();
    return;
  }
    
  if (current > CurrentThreshold) {
    if(debug) {Serial.println("Valve jammed in CurrentThreshold loop..."); }    // valve jammed 

    digitalWrite(ValveOpenPin, LOW);    // current trip, stop opening. 
    digitalWrite(JamLED, L_ON);         // light it up; we got a problem.
    locked = true;                      // stop automatic operation.
    StartClosing();                     // close it up if we can. 
  }
  else if (digitalRead(OpenSens) == LOW) { // reached fully open  
    digitalWrite(ValveOpenPin, LOW);  // stop the valve drive motor.
  
    if(debug) {Serial.println("Fully open state detected..."); }
  
    digitalWrite(JamLED, L_OFF);       // clear the error light, the valve is clear. 		
    if (locked) {                     // this process began from a button-press
      state = Open_state;
    }
    else {                            // normal operating conditions
      StartFilling();
    }
  }
}

void Open() {                    // this is a manual-open state.

  digitalWrite(ValveClosePin, LOW);     // just-in-cases.  There may be tired people working on this. 
  digitalWrite(ValveOpenPin, LOW);      
  
  if (valve_btn_press) {
    valve_btn_press = 0;
    locked = 1;
    StartClosing();
    return;
  }
  if (digitalRead(OpenSens) == LOW) { // reached fully open  
    digitalWrite(ValveOpenPin, LOW);  // stop the valve drive motor.
  }
  else if (current > CurrentThreshold) {
    digitalWrite(ValveOpenPin, LOW);
    digitalWrite(ValveClosePin, LOW);
  }
  digitalWrite(ValveOpenPin, LOW);  // stop the valve drive motor.
  

//  else if (digitalRead(locked)) {
//    digitalWrite(ValveOpenPin, LOW);  
//  }
//  state= Open_state;
 
//  if (digitalRead(OpenSens)) {
//    state = Opening_state;  
//  }
//  if (digitalRead(ClosedSens)) {
//    state = Closed_state;  
//  }
}

void Closing() {
  static byte close_attempts = 0;
  duration = millis() - start_time;
  current = analogRead(CurrentSens);
  
  if (valve_btn_press) {		// button press. 
    valve_btn_press = 0;
    digitalWrite(ValveClosePin, LOW);
    state = Closed_state;
    locked = 1;
    return;
  }

  if (duration < CloseInrushTime) {					// inrush phase, keep going
    return;
  }
	
  if (duration > MaxMotorTime) { 	// valve time-out
      digitalWrite(NoValveLED, L_ON);
      digitalWrite(ValveClosePin, LOW);
      locked = true;
      state = Open_state;
      return;
  }
  
	if (!digitalRead(ClosedSens)) {    				// valve not yet closed
		
		if (current > HighPowerCurrentThreshold) {			// valve jam 
			if(debug) {Serial.println("valve jammed closing"); }
			
			digitalWrite(ValveClosePin, LOW);   // current trip, stop opening.  
      digitalWrite(JamLED, L_ON);
			
			if (close_attempts < 5) {
        close_attempts++;
				StartOpening();
			}
			else {
        if(debug) Serial.println("no close attempt");
				digitalWrite(AlarmPin, 1);
        state = Open_state;
        
			}
		}
	}
  else { 															// Valve closed, wait for current trip.
		if(current > CurrentThreshold) {		// Current tripped: Valve fully closed
			digitalWrite(ValveClosePin, LOW);	// Turn off valve motor. 
			digitalWrite(JamLED, L_OFF);      // clear the error light, the valve is clear. 
			state = Closed_state;							// enter the closed state
			close_attempts = 0;								// reset close_attempts until next time we jam. 
			}
    return;
  }
}

void Closed() {

  digitalWrite(ValveClosePin, LOW);     // just-in-cases.  There may be tired people working on this. 
  digitalWrite(ValveOpenPin, LOW);
  
  if (!locked && !digitalRead(UpperSens) && !digitalRead(LowerSens)) {
    StartOpening();       // not filling. we have to open before we fill. 
  }
  if (valve_btn_press) {
    valve_btn_press = 0;
    locked = 1;
    StartOpening();
  }
 
}




/////////////////////////////////////////////////////////////////////////////
// All the things other than opening and closing 
//////////////////////////////////////////////////////////////////////////
void Filling() {
  duration = millis() - start_time;
  digitalWrite(ValveOpenPin, LOW);    // Just in case of bleary-eyed coders. 
    
  if (valve_btn_press) {
    // move to open state
    valve_btn_press = 0;
    state = Open_state;
    locked = 1;
    digitalWrite(FillPin, LOW);
    return;
  }

  if (digitalRead(UpperSens)) {     // we only require the upper sensor to be high because of the possibility of bridging. 
    digitalWrite(FillPin, LOW);     // Sensors read full, stop filling. 
    StartWaitingToClose();
  }
  
  else if (duration > MaxFillTime) {
    digitalWrite(FillPin, LOW);
    locked = true;
    digitalWrite(NoFillLED, L_ON);
    StartWaitingToClose();
  }
}

void WaitingToClose() {
  duration = millis() - start_time;
  
  if (valve_btn_press) {
    valve_btn_press = 0;
    locked = true;
    Open(); 
    return;
  }
  
  if (duration > WaitToCloseTime) {
    StartClosing();
  }
}

/*void TryToClearValveJam() {
  unsigned long pause_time = 0;
  unsigned long end_time = 4000UL; 
  int repetition = 1;
  int oldCurrentSample = 0;
  int newCurrentSample = 0;
  current = analogRead(CurrentSens);

  if (valve_btn_press) {
    valve_btn_press = 0;
    locked = true;
    StartWaitingToClose();    
    return;
  }
  if(debug){Serial.println("[***] Trying to clear valve jam! Current =");}
    
//////////////////////////////////////////////////////////////////////////////////////////
// First thing we need to do is to stop. This is for safety, delay() to 
// give some time to anyone needing to clear themselves from the valve, ignore sensors.
///////////////////////////////////////////////////////////////////////////////////////
  if(digitalRead(ValveOpenPin) || digitalRead(ValveClosePin)) {
    digitalWrite(ValveOpenPin, LOW);
    digitalWrite(ValveClosePin, LOW);
    digitalWrite(FillPin, LOW);
    
    delay(3000);
  }

//////////////////////////////////////////////////////////////////////////////////////
//  TODO: Add some intellegence arround how many times the valve attempts to clear
//  itself before throwing a more serious alarm and stopping in place. Some tuning in 
//  hardware and or firmware may be necessary to prevent false positives if the tracks
//  become gummed up. 
/////////////////////////////////////////////////////////////////////////////////////

  digitalWrite(ValveClosePin, HIGH);
  //this seems to set "current" to the greater of the last two samples.  FIXME
  for(int sampleSize; sampleSize< 20; sampleSize ++ ) {  
    oldCurrentSample = newCurrentSample;
    newCurrentSample = analogRead(CurrentSens);
    if(newCurrentSample<oldCurrentSample){
      current = oldCurrentSample;
    }
    else {
      current = newCurrentSample;
    }
  } 
  
  if (current < HighPowerCurrentThreshold) {
//  for(repetition=1; repetition <= 2; repetition++) {
    if(debug){Serial.println("in the jam loop");}
    GoForwards();
    pause_time = millis();
    while(pause_time < 20000) {    // listen for a button. Tune this to make more responsive. 
      if (valve_btn_press) {
        valve_btn_press = 0;
        locked = true;
        return;
      } 
//    GoBackwards();
    }
  }
  else {
    digitalWrite(ValveClosePin, HIGH);
    GoBackwards();
  }  
  return;  
} */


//  TODO: GoBkack/GoFowrward need a little refactoring to work as expected with the 
//  calling function above. Currently works but issues are obvious. 
/*void GoBackwards() {
  static unsigned long backwards_end_time = 80000;
  unsigned long backwards_start_time = 0;

  if(debug){Serial.println("GoBackwards .... Current:");}
      
  digitalWrite(ValveOpenPin, HIGH);       

  current = analogRead(CurrentSens);  
  if(debug){Serial.println(current);}
  backwards_start_time = millis();

  for (int j=0; j<2000; j++) {
    Serial.println(j);
//  while(backwards_start_time < backwards_end_time) {
    if (valve_btn_press) {
      valve_btn_press = 0;
      locked = true;
      return;
    }
  } 
  digitalWrite(ValveOpenPin, LOW);       
  while(backwards_start_time < 20000) {
    if (valve_btn_press) {
      valve_btn_press = 0;
      locked = true;
      return;
    } 
  }
  return;
}*/
   
/*void GoForwards() {
  static unsigned long forwards_end_time = 80000;
  unsigned long forward_start_time = 0;

  if(debug){Serial.println("GoForwards .... Current:");}
     
  digitalWrite(ValveClosePin, HIGH);       

  current = analogRead(CurrentSens);  
  if(debug){Serial.println(current);}
  forward_start_time = millis();
  if(debug){Serial.println(forward_start_time);}
  for (int i=0; i<2000; i++) {
//  while(forward_start_time < forwards_end_time) {
    Serial.println(i);

    if (valve_btn_press) {
      valve_btn_press = 0;
      locked = true;
      return;
    }
//  } 
  }
  digitalWrite(ValveClosePin, LOW);
  forward_start_time= millis();  
  while(forward_start_time < 20000) {
    if (valve_btn_press) {
      valve_btn_press = 0;
      locked = true;
      return;
    } 
  }
  GoBackwards();
//  return;
} */



//////////////////////////////////////////////////////////////////////////////////
// State machine action.
////////////////////////////////////////////////////////////////////////////
void StartOpening() {
  if(debug) {Serial.println("StartOpening() function"); }
  state = Opening_state;
  start_time = millis();  
  digitalWrite(ValveOpenPin, HIGH);
}

void StartClosing() {
  if(debug) {Serial.println("StartClosing() function"); }
  state = Closing_state;
  start_time = millis();  
  digitalWrite(ValveClosePin, HIGH);
  delay(500);
}

void StartFilling() {
  if(debug) {Serial.println("StartFilling() function"); }
  state = Filling_state;
  start_time = millis();
  digitalWrite(FillPin, HIGH);
}

void StartWaitingToClose() {
  if(debug) {Serial.println("StartWaitingToClose() function"); }  
  state = WaitingToClose_state;
  start_time = millis();
  
}

/* void StartTryingToClearValveJam() {
  if(debug) {Serial.println("TryToClearValve() function"); }
  state = TryToClear_valve;
  start_time = millis();  
//  digitalWrite(ValveOpenPin, HIGH);  
} */
