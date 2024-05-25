
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for SSD1306 display connected using software SPI (default case):
#define OLED_MOSI  4
#define OLED_CLK   3
#define OLED_DC    2
#define OLED_CS    0
#define OLED_RESET 1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// Must be 0,1,2,3 or 7 on Leonardo
// See https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
#define RPM_PIN 7
#define BUFFER_SIZE 10

int currentRpm = 0;

volatile int nextRpmPosToWrite = 0;
volatile int nextRpmPosToRead = 0;
volatile unsigned long lastRpmRead = 0;

volatile unsigned long rpmTimes[BUFFER_SIZE];


void setup() {
  Serial.begin(9600);
  initDisplay();
  initSensor();
}

void loop() {
  calculateRpm();
  updateDisplay();
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
//    Serial.print("rpmTime ");
//    Serial.print(rpmTime);
//    Serial.print(" lastRpmRead ");
//    Serial.println(lastRpmRead);
//    Serial.print("delta");
//    Serial.println(delta);

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
  sprintf(buffer, "%d", currentRpm);
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
