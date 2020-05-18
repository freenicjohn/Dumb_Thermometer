////////////////////// Setup ///////////////////////
#include <Event.h>
#include <Timer.h>
#include <Stepper.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 2  
#define DHTTYPE DHT11
#define HALFSTEP 8

// Motor pin definitions
#define motorPin1  6     // IN1 on the ULN2003 driver 1
#define motorPin2  7     // IN2 on the ULN2003 driver 1
#define motorPin3  8     // IN3 on the ULN2003 driver 1
#define motorPin4  9     // IN4 on the ULN2003 driver 1

// Global variables
bool relay_state = false;
bool last_relay_state;
bool display_F = true;
bool button_up;
bool last_button_up;
bool button_pressed;
bool last_button_pressed;
bool button_released;
bool button_down;
bool last_button_down;
int myPosition;
int jog_distance = 4;
int jog_speed = 8;
int abs_move_speed = 8;
const int stepsPerRevolution = 2048;
const int totalTempRange = 187;
const int stepsPerDegree = 11;
float temperature;
unsigned long time;
unsigned long relay_time;
unsigned long button_held_time;
unsigned long last_time;
unsigned long up_time;
unsigned long down_time;
unsigned long in_time;

Timer t;

DHT dht(DHTPIN, DHTTYPE);

Stepper stepper1(stepsPerRevolution, motorPin1, motorPin3, motorPin2, motorPin4);

// Setup hardware
void setup() {
  pinMode(LED_BUILTIN, OUTPUT); // LED
  pinMode(3, OUTPUT); // Relay control
  pinMode(10, INPUT_PULLUP); // Selector up
  pinMode(11, INPUT_PULLUP); // Selector in
  pinMode(12, INPUT_PULLUP); // Selector down
  Serial.begin(9600);
  dht.begin();
  stepper1.setSpeed(jog_speed);
}

////////////////////// Main ///////////////////////
// Continuous loop
void loop() {

  // Update time variable
  time = millis();

  // Call selector control - used to calibrate gauge position
  selectorControl();

  // Get temperature and update position
  if (time - last_time >= 3000) {
    last_time = time;
    temperature = getTempData();
    if (!button_pressed) {
      updatePosition((int)temperature*stepsPerDegree, false);
    }
    printStatus(); // Make sure Serial.begin is called in setup() if using this
  }

  // Turn off relay after it's been on for 2 seconds
  checkRelay();
} 

///////////////////// Update Position ///////////////////////
void updatePosition(int setpoint, bool now) {
  if (((!relay_state) || now) && (abs(myPosition - setpoint) >= stepsPerDegree + 1)) {
    absoluteMove(setpoint);
  }
}

///////////////////// Selector Control ///////////////////////
void selectorControl() {
// Control LED based on selector myPosition

  // Update inputs
  button_up = debounceUp(100);
  button_pressed = debouncePressed(100);
  button_down = debounceDown(100);

  // Button pressed - set new home or toggle Fahrenheit/Celsius
  if (button_pressed) {
    if (!button_released) {
      button_held_time = time;
    }
    buttonHeld();
  }

  // Button Up - jog clockwise
  else if (button_up) {
    jogForward();
  }

  // Button Down - jog counter-clockwise
  else if (button_down) {
    jogReverse();
  }

  if ((button_released == true) && (button_pressed == false)) {
    display_F = !display_F;
    //temperature = getTempData();
    //updatePosition((int)temperature*stepsPerDegree, true);
  }

  // Light LED if any button is pressed
  if (button_up || button_down || button_pressed) {
    digitalWrite(LED_BUILTIN, HIGH);
  }
  else {
    digitalWrite(LED_BUILTIN, LOW);
  }

  // Set last button states
  last_button_up = !digitalRead(12);
  last_button_pressed = !digitalRead(11);
  last_button_down = !digitalRead(10);
  button_released = button_pressed;
}

void buttonHeld() {
  if (time - button_held_time >= 2000) {
    myPosition = 0;
    display_F = !display_F;
  }
}

////////////////////// Motion Control ///////////////////////
void incrementalMove(int distance) {
  if (relay_state) {
    stepper1.step(distance);
    myPosition = myPosition + distance;
    relay_time = time;
  }
  else {
    digitalWrite(3, HIGH); // Energize relay for servo
    relay_state = true;
    delay(250);
    stepper1.step(distance);
    myPosition = myPosition + distance;
    relay_time = time;
  }
}

void jogForward() {
  stepper1.setSpeed(jog_speed);
  incrementalMove(jog_distance);
}

void jogReverse() {
  stepper1.setSpeed(jog_speed);
  incrementalMove(-jog_distance);
}

void absoluteMove(int setpoint) {
  stepper1.setSpeed(abs_move_speed);
  incrementalMove(setpoint - myPosition);
}

void checkRelay() {
  if (relay_state) {
    if (time - relay_time >= 2000) {
      digitalWrite(3, LOW);
      relay_state = false;
    }
  }
  last_relay_state = relay_state;
}

/////////////////// Temperature Reading ///////////////////////
float getTempData() {
// Get humidity and temperature in fahrenheit and celcius
  //float humidity = dht.readHumidity();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float temp = dht.readTemperature(display_F);
  return temp;
}

////////////////////// Print Status ///////////////////////
void printStatus() {
  Serial.write("Position: ");
  Serial.println(myPosition);
  Serial.write("Temp: ");
  Serial.println(temperature);
}

////////////////////// LED Control ///////////////////////
void flashQuick() {
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(100);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(100);  
}

void flashSlow() {
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000); 
}

///////////////// Debounce Button Inputs  ///////////////////////
bool debounceUp(unsigned long debounce_time) {
  if (!digitalRead(12)) {
    if (!last_button_up) {
      up_time = time;
    }
    if (time - up_time >= debounce_time) {
      return true;
    }
  }
  else {
    return false;
  }
}

bool debounceDown(unsigned long debounce_time) {
  if (!digitalRead(10)) {
    if (!last_button_down) {
      down_time = time;
    }
    if (time - down_time >= debounce_time) {
      return true;
    }
  }
  else {
    return false;
  }
}
bool debouncePressed(unsigned long debounce_time) {
  if (!digitalRead(11)) {
    if (!last_button_pressed) {
      in_time = time;
    }
    if (time - in_time >= debounce_time) {
      return true;
    }
  }
  else {
    return false;
  }
}
