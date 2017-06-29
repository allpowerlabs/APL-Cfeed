APL-Cfeed
=========

Continuous feed controller for the Power Pallet by All-Power-Labs

Currently the APL Continuous Feed controller is a stand alone device solely responsible for opening and closing the fuel gate in both manual and automatic operation. 
The codebase can be compiled with any standard distribution of the Arduino IDE or comparable Atmel toolchain at the time of this writing. As this codebase evolves we will track changes below. 

Changelog
=========
#---20161031 --- Minor changes to the code which is currently shipping. View 'git diff' for the commit details. 

#---20170120 --- Conditional logic change which reflects changes to the wiring harness for the level sensors(nc>no)
>colordiff ./cfeed.ino ../../CFeedArduinoCode_2017120.ino >> ./README.md 
>define CurrentThreshold 120           // was 80 which should be ~5A, changed to 120
>define CurrentThreshold 180           // was 80 which should be ~5A, changed to 120. Changed to 180 by SS (1/20/2017)
>if(debug) {Serial.println("valve jammed closing"); }
>if(debug) {Serial.println("valve jammed closing");
>digitalWrite(ValveClosePin, LOW);               // Turn off valve motor.
>digitalWrite(ValveClosePin, LOW);         // Turn off valve motor.
>if (!locked && digitalRead(UpperSens) && digitalRead(LowerSens)) {          //(!locked && !digitalRead(UpperSens) && !digitalRead(LowerSens))
>if (!locked && !digitalRead(UpperSens) && !digitalRead(LowerSens)) {          //(!locked && !digitalRead(UpperSens) && !digitalRead(LowerSens))
>if (!digitalRead(UpperSens)) {     // we only require the upper sensor to see fuel because of the possibility of bridging.
>if (digitalRead(UpperSens)) {     // we only require the upper sensor to see fuel because of the possibility of bridging.
>start_time = millis();
>start_time = millis();
>Serial.println("Relay");
>Serial.println(FillPin);

#---20170127 --- Added some comments to break up functions more clearly and generated .hex file which will be added to Arena

#---20170627 --- Version 1.2 Release
	The primary change between the v1.1 and v1.2 is compatibility with the new CFeed inductive sensors. The old inductive sensors had a Normally Open wiring configuration. 
	The new sensors have a Normally Closed wiring configuration. Because of this change any CFeed with the v1.2 software is no longer compatible with previous versions.
>if (digitalRead(ClosedSens))  // if the closed sensor lit, we're already closed, so wake in closed state. 
Changed to:
>if (!digitalRead(ClosedSens))  // if the closed sensor light turns off, we're already closed, so wake in closed state. 
>else if (digitalRead(OpenSens) == LOW) {        // reached fully open  
Changed to:
>else if (digitalRead(OpenSens) == HIGH) {        // reached fully open 
>if (digitalRead(OpenSens) == LOW) {             // reached fully open  
Changed to:
>if (digitalRead(OpenSens) == HIGH) {             // reached fully open  
>if (!digitalRead(ClosedSens)) {                // valve not yet closed 
Changed to:
>if (digitalRead(ClosedSens)) {                // valve not yet closed 

Current Threshold (current used to determine whether valve has closed) was moved back to 120 from 180. Additional current was deemed unnecessary for sealing.  

#---20170629 --- Added logic in Open function that forces CFeed to close when auto/reset button is pressed. Previously, if CFeed was in manual mode and open state and the auto/reset button was pressed, the valve would remain in the open state indefinitely. 
>if (!locked){
>    StartClosing();
>  }

