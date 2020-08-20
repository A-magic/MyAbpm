#include "heltec.h"
#include "images.h"

/********SSD1306定义**********/
#define DEMO_DURATION 3000
typedef void (*Demo)(void);

int demoMode = 0;
int counter = 1;

void drawFontFaceDemo(int hp, int lp, int hr) {
  // Font Demo1
  // create more fonts at http://oleddisplay.squix.ch/
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->drawString(0, 0, "HP:  " + String(hp));

  Heltec.display->drawString(0, 16, "LP:   " + String(lp));

  Heltec.display->drawString(0, 32, "HR:  " + String(hr));
  Heltec.display->display();
}


void printBuffer(void) {
  // Initialize the log buffer
  // allocate memory to store 8 lines of text and 30 chars per line.
  Heltec.display->setLogBuffer(3, 10);

  // Some test data
  const char* test[] = {
    "Hello",
    "World" ,
    "----",
    "Show off",
    "how",
    "the log buffer",
    "is",
    "working.",
    "Even",
    "scrolling is",
    "working"
  };

  for (uint8_t i = 0; i < 11; i++) {
    Heltec.display->clear();
    // Print to the screen
    Heltec.display->println(test[i]);
    // Draw it to the internal screen buffer
    Heltec.display->drawLogBuffer(0, 0);
    // Display it on the screen
    Heltec.display->display();
    delay(500);
  }
}

Demo demos[] = {printBuffer};
int demoLength = (sizeof(demos) / sizeof(Demo));
long timeSinceLastModeSwitch = 0;