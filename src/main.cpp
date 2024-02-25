#include <Arduino.h>
#include "Adafruit_NeoPixel.h"
#include <SPIFFS.h>
#include "InlandEPD.h"
#include "epdpaint.h"

#define NEOPIXELS   1

#define BUSY_PIN    18
#define RST_PIN     17
#define DC_PIN      9
#define CS_PIN      8

#define UNCOLORED   0xFF
#define COLORED     0x00

Adafruit_NeoPixel pixels(NEOPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
InlandEPD epd(CS_PIN, DC_PIN, RST_PIN, BUSY_PIN);
Paint paint(epd.buffer.data(), EPD_WIDTH, EPD_HEIGHT);

int text_row = 1;

void printline(const char* line, int x = 0);


void setup() {
    Serial.begin(115200);
    SPIFFS.begin();

    pinMode(NEOPIXEL_POWER, OUTPUT);
    digitalWrite(NEOPIXEL_POWER, HIGH);

    pixels.begin();
    pixels.setBrightness(20);
    pixels.fill(0x000000);
    pixels.show();

    paint.SetRotate(ROTATE_90);

    epd.init();
}

void loop() {
    printline("Hello!");
    // if (bw) {
    //     // std::fill(epd.buffer.begin(), epd.buffer.end(), 0x00);
    //     paint.Clear(0x00);
    // } else {
    //     // std::fill(epd.buffer.begin(), epd.buffer.end(), 0xFF);
    //     paint.Clear(0xFF);
    // }
    // bw = !bw;
    // epd.update_full();

    // pixels.fill(0xFF0000);
    // pixels.show();
    // delay(500);

    // pixels.fill(0x00FF00);
    // pixels.show();
    // delay(500);

    // pixels.fill(0x0000FF);
    // pixels.show();

    // delay(2000);
}

void printline(const char* line, int x) {
    if (text_row >= (EPD_WIDTH / 24)) {
        text_row = 1;
        epd.wipe();
    }
    int y = text_row * 24;
    text_row++;

    Serial.println(line);
    paint.DrawStringAt(x, y, line, &Font24, COLORED);
    epd.update_partial();
    // epd.update_full();
}
