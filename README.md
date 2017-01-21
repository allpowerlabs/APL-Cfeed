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



