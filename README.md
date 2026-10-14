# ensc-351-project-STASH


Milestone 1- Basic System (Core Logic and GPIO) 

Flash Linux onto the BeagleY-AI and configure the C/C++ build environment 

Write drivers/functions to read the rotary encoder and push buttons via GPIO 

Configure GPIO outputs to control the status LEDs and buzzer 

Implement the C/C++ timer logic to handle transitions between study, break and paused states based on hardware inputs 

Deliverable: A functional study timer that can take physical inputs and provide LED/Audio feedback without a screen. 

Project Task Breakdown: 

Ashman— GPIO and hardware interface 

Hardware 

Files: gpio_hal.hpp, gpio_hal.cpp 

Map the physical header pins and configure GPIO inputs/outputs. 

Read the rotary encoder and push buttons. 

Control the LEDs  

Test every input/output independently on the BeagleY-AI. 

Deliverable: A tested hardware interface that other modules can call without needing to know the GPIO details. 

Shannah — Timer and state machine 

Core logic 

Files: timer.hpp, timer.cpp 

Implement study, break, paused, and idle states. 

Manage countdowns and transitions when time expires. 

Implement start, pause, resume, and reset logic. 

Use a monotonic clock so timing is not affected by system clock changes. 

Deliverable: A timer module that works with simulated inputs before connecting to hardware. 

Tajleen — User controls and input handling 

Interaction 

Files: input_handler.hpp, input_handler.cpp 

Decode rotary encoder direction and button presses, joystick  

Translate physical actions into commands such as start, pause, and reset. 

Add button debouncing and prevent repeated actions from one press. 

Let the user adjust study and break durations using the encoder. 

Deliverable: Reliable user input that can be passed to the timer module. Coordinate with Person 1 so GPIO access is not implemented twice. 

Samantha — Status feedback and alerts  

Outputs 

Files: feedback.hpp, feedback.cpp, LED.cpp, LED.hpp 

Define the LED behavior for each timer state. 

Activate the buzzer when study or break sessions finish. 

Audio for timer milestones  

Provide clear feedback when the timer is paused or waiting to start. 

Deliverable: A feedback module controlled by timer state changes. Coordinate GPIO output calls with Person 1. 

Harkirit — Integration, build system, and testing 

Integration 

Files: main.cpp, CMakeLists.txt, README.md, tests/ 

Create the main program loop that connects inputs, timer logic, and feedback. 

Configure CMake to compile everyone's modules. 

Write integration tests and test cases for state transitions. 

Document the build/run instructions and prepare the final demonstration. 

Deliverable: A complete application that builds and runs on the BeagleY-AI. 

Person 5 should coordinate integration from the start rather than waiting for everyone to finish. 

Ordering Parts? 

‘ 

 

Timeline:  

 

 

 

 

 

 

 

 

 

 

GPIO Pinout:  

Component, BeagleY-AI physical pin, GPIO signal 

Encoder channel A (CLK), 7, GPIO4 

Encoder channel B (DT), 11, GPIO17 

Encoder push switch (SW), 13, GPIO27 

Pause/resume button, 15, GPIO22 

Reset button, 16, GPIO23 

Study LED, 18, GPIO24 

Break LED, 22, GPIO25 

Buzzer control, 33, GPIO13 

Common ground, 9, GND 

 

KY-040 Rotary Encoder Module 360 Degree Switch Encoders with Knob Cap – AusPower AustraliaHow to wire each component 

A. Rotary encoder 

CLK → physical pin 7 

DT → physical pin 11 

SW → physical pin 13 

GND → physical pin 9 

VCC → 3.3 V, if supported by your encoder module 

Turning the knob adjusts the selected study/break duration; pressing it starts or selects. 

6x6mm Tactile Push Button Switch Cap PCB BreadBoard Arduino Raspberry PI ESP32  | eBay Australia 

B. Two push buttons 

Pause button: physical pin 15 → button → GND 

Reset button: physical pin 16 → button → GND 

Configure the GPIO inputs with pull-ups if supported. The input reads LOW when pressed. 

C. Two status LEDs 

For each LED, connect its GPIO output through a current-limiting resistor (for example, 1 kΩ–2.2 kΩ) to the LED anode, with the cathode connected to GND. 

Study LED anode circuit → physical pin 18 

Break LED anode circuit → physical pin 22 

Green for study and amber for break is a reasonable convention. 

D. Buzzer 

GPIO control → physical pin 33 

Use a suitable transistor driver if required by the buzzer's current 

Connect the buzzer's supply and ground according to its specifications 

Don't connect an unknown buzzer directly to the GPIO output. 

 
