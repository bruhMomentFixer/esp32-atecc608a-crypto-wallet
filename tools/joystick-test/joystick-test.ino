#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// PROBAR CON D13
#define JOY_Y     19  // Cambia a este pin
#define JOY_BTN   15

void setup() {
    Serial.begin(115200);
    
    tft.initR(INITR_GREENTAB);
    tft.setRotation(2);
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    
    pinMode(JOY_BTN, INPUT_PULLUP);
    
    tft.setCursor(0, 0);
    tft.println("TEST CON GPIO 13");
    tft.println("VRy en D13");
    tft.println("==============");
    
    delay(1000);
}

void loop() {
    int yValue = analogRead(JOY_Y);
    bool btnState = digitalRead(JOY_BTN);
    
    tft.fillRect(0, 30, 128, 90, ST77XX_BLACK);
    tft.setCursor(0, 30);
    
    tft.print("GPIO 13: ");
    tft.setTextColor(ST77XX_CYAN);
    tft.println(yValue);
    
    tft.setTextColor(ST77XX_WHITE);
    tft.print("Estado: ");
    if (yValue < 1000) {
        tft.setTextColor(ST77XX_GREEN);
        tft.println("ARRIBA ↑");
    } else if (yValue > 3000) {
        tft.setTextColor(ST77XX_GREEN);
        tft.println("ABAJO ↓");
    } else {
        tft.setTextColor(ST77XX_YELLOW);
        tft.println("CENTRO");
    }
    
    Serial.printf("GPIO13: %4d\n", yValue);
    delay(100);
}
