#include <Adafruit_GFX.h>    
#include<Adafruit_ST7735.h>
#include <SPI.h>

// | Value | Orientation                   |
// | ----- | ----------------------------- |
// | 0     | Portrait (default)            |
// | 1     | Landscape (clockwise)         |
// | 2     | Portrait (upside down)        |
// | 3     | Landscape (counter-clockwise) |
// tft.initR(INITR_BLACKTAB);
// tft.setRotation(1); // rotate display



// Pin definitions for ESP32
#define TFT_CS     5
#define TFT_RST   15
#define TFT_DC    32
#define TFT_MOSI  23
#define TFT_CLK   18

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST);

void setup() {
  Serial.begin(115200);
  Serial.println("DHTxx test!");
  tft.initR(INITR_BLACKTAB); // For 1.8" TFT with black tab
  tft.fillScreen(ST77XX_BLACK);
  //dht.begin();
  tft.setRotation(1);
  tft.setCursor(45, 60);             // Set position on screen
  tft.setTextColor(ST77XX_BLUE);    // Text color
  tft.setTextSize(2);                // Scale text size
  tft.println("Hello");
  delay(2000);

  tft.fillScreen(ST77XX_BLACK);
  tft.drawTriangle(30, 30, 60, 90, 90, 30, ST77XX_GREEN);
  delay(2000);

  // Print text
       
}

void loop() {
  
  tft.fillScreen(ST77XX_YELLOW);
  tft.setRotation(0);
  tft.setCursor(0, 0);             // Set position on screen
  tft.setTextColor(ST77XX_ORANGE);    // Text color
  tft.setTextSize(0);                // Scale text size
  tft.println("Hello0");
  delay(2000);

  tft.fillScreen(ST77XX_GREEN);
  tft.setRotation(1);
  tft.setCursor(10, 10);             // Set position on screen
  tft.setTextColor(ST77XX_RED);    // Text color
  tft.setTextSize(1);                // Scale text size
  tft.println("Hello1");
  delay(2000);

  tft.fillScreen(ST77XX_CYAN);
  tft.setRotation(2);
  tft.setCursor(10, 10);             // Set position on screen
  tft.setTextColor(ST77XX_RED);    // Text color
  tft.setTextSize(2);                // Scale text size
  tft.println("Hello2");
  delay(2000);

  tft.fillScreen(ST77XX_MAGENTA);
  tft.setRotation(3);
  tft.setCursor(10, 10);             // Set position on screen
  tft.setTextColor(ST77XX_BLUE);    // Text color
  tft.setTextSize(3);                // Scale text size
  tft.println("Hello3");
  delay(2000);

  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(4);
  tft.setCursor(10, 10);             // Set position on screen
  tft.setTextColor(ST77XX_WHITE);    // Text color
  tft.setTextSize(4);                // Scale text size
  tft.println("Hello4");
  delay(2000);

/*
  for (int x = 0; x < 128; x += 2) { // Move from left to right
    tft.fillScreen(ST77XX_BLACK);   // Clear screen

    // Draw triangle at new position
    tft.fillTriangle(x, 50, x + 15, 80, x + 50, 50, ST77XX_RED);

    delay(50);
  }
*/
}