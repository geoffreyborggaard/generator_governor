#include <ArduPID.h>


#include <SPI.h>
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for SSD1306 display connected using software SPI (default case):
#define OLED_MOSI    4
#define OLED_CLK     3
#define OLED_DC      2
#define OLED_CS      0
#define OLED_RESET   1
#define THROTTLE_PIN 9
#define CHOKE_PIN    10
#define SHUTDOWN_PIN 11


#define CHOKE_OFF      15
#define CHOKE_FULL     60
#define THROTTLE_OFF   148
#define THROTTLE_IDLE  145
#define THROTTLE_START 140
#define THROTTLE_FULL  120
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// Must be 0,1,2,3 or 7 on Leonardo
// See https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
#define RPM_PIN 7
#define BUFFER_SIZE 10

int currentRpm = 0;
Servo throttle;
Servo choke;
ArduPID myController;
double setpoint = 3600;
double input;
double output;
double p = 0.1;
double i = 0;
double d = 0;



volatile int nextRpmPosToWrite = 0;
volatile int nextRpmPosToRead = 0;
volatile unsigned long lastRpmRead = 0;
bool started = false;
bool shutdown = false;
bool shutdownByButton = false;
volatile unsigned long rpmTimes[BUFFER_SIZE];

char* message = "Press Start";


void setup() {
  Serial.begin(9600);
  initDisplay();
  initSensor();
  throttle.attach(THROTTLE_PIN);
  choke.attach(CHOKE_PIN);
  throttle.write(THROTTLE_START);
  choke.write(CHOKE_FULL);

  myController.begin(&input, &output, &setpoint, p, i, d);
  myController.reverse(); // Higher throttle is lower values;
  myController.setOutputLimits(THROTTLE_FULL, THROTTLE_IDLE);

  pinMode(SHUTDOWN_PIN, INPUT_PULLUP);
}

void loop() {
  calculateRpm();
  updateDisplay();
  //testServos();
  if (currentRpm > 1400 && !started){
    started = true;
    message = "Running";
    myController.start();
    choke.write(CHOKE_OFF);
  }

  // Read shutdown switch.
  //int buttonVal = digitalRead(SHUTDOWN_PIN);
  int buttonVal = HIGH;
  
//  if (buttonVal == LOW || (started && currentRpm < 100)) {
//    if (buttonVal == LOW) {
//      shutdownByButton = true;
//    }
//    shutdown = true;
//    message = "Shutting down";
//    throttle.write(THROTTLE_OFF);
//  } else if (buttonVal == HIGH && shutdownByButton == true) {
//    shutdown = false;
//    started = false;
//    choke.write(CHOKE_FULL);
//  }

  if (started && !shutdown) {
    input = (double) currentRpm;
    myController.compute();
    myController.debug(&Serial, "myController", PRINT_INPUT    | // Can include or comment out any of these terms to print
                                            PRINT_OUTPUT   | // in the Serial plotter
                                            PRINT_SETPOINT |
                                            PRINT_BIAS     |
                                            PRINT_P        |
                                            PRINT_I        |
                                            PRINT_D);

    throttle.write((int)output);
    delay(10);
  }
}

void initSensor() {

  for (int i = 0; i < BUFFER_SIZE; i++) {
    rpmTimes[i] = 0;
  }
  pinMode(RPM_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(RPM_PIN), rpmInterrupt, FALLING);
  
  lastRpmRead = 0;
}

void rpmInterrupt() {
  rpmTimes[nextRpmPosToWrite++] = millis();
  nextRpmPosToWrite %= BUFFER_SIZE;
}

void calculateRpm() {
  if (nextRpmPosToRead == nextRpmPosToWrite && millis() - lastRpmRead > 500) {
    currentRpm = 0;
  }
  
  while (nextRpmPosToRead != nextRpmPosToWrite) {
    unsigned long rpmTime = rpmTimes[nextRpmPosToRead];
    unsigned long delta = rpmTime - lastRpmRead;

    if (delta == 0 || delta > 60000) {
      currentRpm = 0;
    } else {
      currentRpm = ((float)60000) / ((float) delta);
    }
    lastRpmRead = rpmTime;
    nextRpmPosToRead++;
    nextRpmPosToRead %= BUFFER_SIZE;
//    Serial.print(" currentRPM ");
//    Serial.println(currentRpm);
  }
}

void updateDisplay() {
  display.clearDisplay();
  char buffer[128]; 
  
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  sprintf(buffer, "%d\n", currentRpm);
  display.write(buffer);
  display.setTextSize(1);      // Normal 1:1 pixel scale
  sprintf(buffer, "%s\n", message);
  display.write(buffer);
  sprintf(buffer, "%d\n", (int)output);
  display.write(buffer);
  display.display();
}

void initDisplay() {
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.display();
  delay(500); 
  display.display();
  display.clearDisplay();
}

void testServos(){
choke.write(CHOKE_OFF);
delay(1000);
choke.write(CHOKE_OFF);
delay(1000);
choke.write(CHOKE_FULL);
delay(1000);
}
