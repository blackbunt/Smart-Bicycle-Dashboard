/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
//    __          _____    __   __     __      ___    __      __      __  __      __  __       __o   
//   /__`|\/| /\ |__)|    |__)|/  `\ //  `|   |__    |  \ /\ /__`|__||__)/  \ /\ |__)|  \    _ \<_
//   .__/|  |/~~\|  \|    |__)|\__, | \__,|___|___   |__//~~\.__/|  ||__)\__//~~\|  \|__/   (_)/(_) 
//
//  Smart Bicycle Dashboard
//
//  Informatik-Projekt WiSe 19/20
//  Bernhard Kauffmann (kabe1017)
//  kabe1017@hs-karlsruhe.de
//
//  https://github.com/blackbunt/Smart-Bicycle-Dashboard
//
//  Version: 1.3
//  Datum: 13.01.2020
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#define cPI 3.14159265358979323846264338327950288419716939937510582097494459230781640628620899

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SETUP
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//#define DEBUGMODE  // comment out to switch Debugmode on and to display additional information over the serial interface

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pin Setup & Constants
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const double radiusLaufrad = 0.337;                             // radius of wheel in m
#define DURCHMESSER_LAUFRAD (radiusLaufrad * 2.0)               // diameter of wheel in m

#define time2sleep_s 10                                         // Time until DeepSleep in seconds
#define CLR 25                                                  // GPIO Pin 2 Clear FlipFlop
#define D 39                                                    // GPIO Pin 2 Set "D" Pin on FlipFlop
#define switchReed 15                                           // GPIO Pin 4 Reed Switch Input
#define WakeUpInput GPIO_NUM_26                                 // GPIO Pin 4 WakeUp Input, wake ESP32 from Deep Sleep
#define switchDisplay 19                                        // GPIO Pin 2 Switch Display Power on/off
#define sensorLight 32                                          // GPIO Pin 2 Read analog Input of PhotoDiode
#define toggleLight 13                                          // GPIO Pin 2 toggle Lights
#define brightnessThreshold 60                                  // brightness threshold in %, 4 updateLight(), switchLight()
#define debouceTime 35                                          // debouce Time in ISR()
#define SEC_in_MS 1000                                          // delay in loop()

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Display Setup
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define SCREEN_WIDTH  128                                       // Display width in pixels
#define SCREEN_HEIGHT  64                                       // Display height in pixels
#define OLED_RESET      4 
#define I2C_ADDRESS_DISPLAY 0x3C                                // Display adress I2C Bus
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Variables that are touched by interrupt routines should be declared "volatile"
volatile int zaehlerSwitchReed = 0;                             // Number of rising edges are stored here
volatile unsigned long lastTime = 0;                            // Time of the last rising edge, necessary for debouncing, in ms

unsigned long timeSpeed = 0;                                    // Stores time in ms for calcSpeed()
const long TimeTillDeepSleep = time2sleep_s * 1000;             // Stores Time in ms until x is called
unsigned long time_now = 0;                                     // Stores current time for go2deepSleep()

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void isr();                                                    // Interrupt Service Routine
double calcSpeed();                                            // Calculates Speed in Km/h
int readSensorLight();                                         // Reads value of photo
void go2deepSleep(double speed, unsigned long TimeDS, unsigned long time);   // DeepSleep Funktion
void MainDisplay(double speed, int brightness, bool light);    // Display Ausgabe
void switchLight(int brightness);                              // Steuert Licht
bool updateLight(int brightness);                              // Light State (on/off) 4 MainDisplay()

#if defined DEBUGMODE
void debugSerial();                                            // Serial Debug Menu 
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  pinMode(CLR, OUTPUT);                                        // set PinMode
  pinMode(D, OUTPUT);
  pinMode(switchDisplay, OUTPUT);
  pinMode(sensorLight, OUTPUT);
  pinMode(toggleLight, OUTPUT);
  #if defined DEBUGMODE
    Serial.begin(115200);                                      // open serial Monitor
    Serial.println("booting...");
    delay(250);                                                //Take some time to open up the Serial Monitor
  #endif
  

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeepSleep Setup
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  esp_sleep_enable_ext0_wakeup(WakeUpInput , 1);              // 1 = High, 0 = Low
                                                              //Only RTC IO can be used as a source for external wake
                                                              //source. Pins: 0,2,4,12-15,25-27,32-39.

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Interrupt
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  pinMode(switchReed, INPUT_PULLUP);
  attachInterrupt(switchReed, isr, HIGH);                     // interrupt for pin switchReed, on each rising edge isr() is called

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Display Settings
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  digitalWrite(switchDisplay, HIGH);                          // power on Display
  display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS_DISPLAY);
  display.clearDisplay();                                     // clear Display
  display.display();

  #if defined DEBUGMODE
    while (!Serial); // Serial Debug Menu
    Serial.println("Reset FlipFlop == 1");
    Serial.println("DeepSleep == 5");
  #endif
}

void loop() {
  
  delay(SEC_in_MS);                                           // take a little time 2 measure
  double speed = calcSpeed();                                 // calc current speed
  int brightness = readSensorLight();                         // brightness value
  bool lightOnOff = updateLight(brightness);                  // check if light symbol should be shown on display 
  switchLight(brightness);                                    // external Lights on/off
  MainDisplay(speed, brightness, lightOnOff);                 // refresh Display
  go2deepSleep(speed, TimeTillDeepSleep, time_now);           // DeepSleep
  
  #if defined DEBUGMODE
    debugSerial();
  #endif
}

void isr() {                                                  // change counter if at least "debouceTime" have passed since the last edge
  unsigned long now = millis();
  if (now - lastTime > debouceTime){                          // debouce input
      zaehlerSwitchReed++;

      #if defined DEBUGMODE
        Serial.println("Interrupt triggered!");
      #endif
  }
  lastTime = now;                                             // save current time
}

double calcSpeed() {                                          // calculate velocity in Km/h
  double speed = 0.0;
  speed = ((cPI * DURCHMESSER_LAUFRAD * zaehlerSwitchReed * 1000.0) / ((millis() - timeSpeed) * 4) * 3.6);
  timeSpeed = millis();
  zaehlerSwitchReed = 0;                                      // change counter 2 "0" --> isr()

  return speed;
}

void go2deepSleep(double speed, unsigned long TimeDS, unsigned long time) { // sleep function
  
  #if defined DEBUGMODE
    Serial.print("\n\n Time until sleep: ");
    Serial.print(TimeDS - (millis() - time));
    Serial.print("\n");
  #endif

  if (speed > 0) {
    time_now = millis();
  }

  if (speed < 0.5 && millis() - time > TimeDS) {
    
    #if defined DEBUGMODE
          Serial.println("Preparing sleep...");
    #endif

    digitalWrite(CLR, HIGH);            // Prepare FliFlop 4 DeepSleep
    digitalWrite(CLR, LOW);
    digitalWrite(CLR, HIGH);
    digitalWrite(D, HIGH);
        
    #if defined DEBUGMODE
      Serial.println("Flip Flop Resettet, D pin HIGH!");
      Serial.println("I am ready 2 sleep!");
      Serial.println("Going to sleep now!");
    #endif
    
    display.clearDisplay();
    display.setTextSize(2);             // Normal 1:1 pixel scale
    display.setTextColor(WHITE);        // Draw white text
    display.setCursor(0,0);             // Display Deep Sleep Message
    display.print("Going");
    display.setCursor(0,32);      
    display.print("2 Sleep!");
    display.display();
    delay(750);                         // Wait a little bit
    digitalWrite(switchDisplay, LOW);   // Power of Display

    esp_deep_sleep_start();             // call DeepSleep Function

    #if defined DEBUGMODE
      Serial.println("This will never be printed");
    #endif
  }
} 

void MainDisplay(double speed, int brightness, bool light) {
  
  // Set value to 1 decimal place
  int iSpeed = speed * 10; 
  char buf[5];
  sprintf(buf, "%2d.%01d", iSpeed / 10 , abs(iSpeed % 10));

  display.clearDisplay();                                      // Clear Display
  display.setTextSize(5);                                      // Normal 1:1 pixel scale
  display.setTextColor(WHITE);                                 // Draw white text
  display.setCursor(0,0);                                      // Start at top-left corner
  display.print(buf);                                          // Velocity
  display.drawRect(0, 55, 90, 5, WHITE);                       // Rectangle 4 Brightnessbar
  display.fillRect(0, 55, 90*  brightness / 100 , 5, WHITE);   // Fill rectangle proportionally to brightness value
  display.setCursor(0, 44);
  display.setTextSize(1);
  display.print("Helligkeit: ");                               // Brightness Value
  display.print(brightness);

  // Draw filled Dot if Lights are on, if not just a circle
  if (light == true) {
    display.fillCircle(115, 55, 7, WHITE);
  }
  else {
    display.drawCircle(115, 55, 7, WHITE);
  }
  display.display();
    
}

int readSensorLight() {                                        // Read Val of photodiode, convert in %
  float analogVal = (float)analogRead(sensorLight);
	int result =  (int)(analogVal / 40.95);                      // Convert to %

  return result;
}

void switchLight(int brightness) {                             // controls lighting system 
  
  //If the brightness value is below the set value, the lighting system is switched on
  if (readSensorLight() < brightnessThreshold) {
    digitalWrite(toggleLight, LOW);
  }

  // When the brightness level is above the set value, the lighting system is switched off
  if(readSensorLight() >= brightnessThreshold) {
    digitalWrite(toggleLight, HIGH);
 }
}

bool updateLight(int brightness) {                             // return if Lights are on/off 
  if (readSensorLight() < brightnessThreshold) {
      
    return true;
  }
  else {
    
    return false;
  }
}

#if defined DEBUGMODE

void debugSerial() {                                          //small debug menu to interact with flipflop and trigger the DeepSleep manually                         
  if (Serial.available()) {
    
    
      int debug = Serial.parseInt();

      if (debug == 1) {

        digitalWrite(CLR, HIGH);
        delay(10);
        digitalWrite(CLR, LOW);
        delay(10);
        digitalWrite(CLR, HIGH);
        delay(10);
        digitalWrite(D, HIGH);
        delay(10);
        Serial.println("Flip Flop Reset");
      }

      if (debug == 5) {
        Serial.println("Preparing sleep...");
        //delay(100);
        digitalWrite(CLR, HIGH);
        //delay(10);
        digitalWrite(CLR, LOW);
        //delay(10);
        digitalWrite(CLR, HIGH);
        //delay(10);
        digitalWrite(D, HIGH);
        //delay(100);
        Serial.println("Flip Flop Resettet, D pin HIGH!");
        Serial.println("Going to sleep now!");
        esp_deep_sleep_start();
        Serial.println("This will never be printed");
      }
    }
}

#endif
