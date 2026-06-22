#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// Pines LCD
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  Serial.begin(115200);
  
  // Inicializar LCD
  Serial.println("Iniciando LCD...");
  tft.initR(INITR_BLACKTAB);
  
  // Pruebas visuales
  tft.fillScreen(ST77XX_BLACK);
  delay(500);
  tft.fillScreen(ST77XX_RED);
  delay(500);
  tft.fillScreen(ST77XX_GREEN);
  delay(500);
  tft.fillScreen(ST77XX_BLUE);
  delay(500);
  tft.fillScreen(ST77XX_BLACK);
  
  // Texto de prueba
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("LCD FUNCIONA!");
  tft.setTextSize(1);
  tft.setCursor(10, 40);
  tft.println("Si ves esto, OK");
  
  Serial.println("LCD prueba completada");
}

void loop() {
  // Nada aquí
}
