#include <Arduino.h>
#include "Adafruit_NeoPixel.h"

#include "InlandEPD.h"
#include "epdpaint.h"

#include "WiFi.h"
#include "NTP.h"
#include "time.h"
#include "esp_sntp.h"

#if !(defined(WIFI_SSID) && defined(WIFI_PASSWORD))
    #error "Please define WIFI_SSID and WIFI_PASSWORD in platformio.ini"
#endif

// Neopixel
#define NEOPIXELS   1
Adafruit_NeoPixel pixels(NEOPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// Inland epaper display
#define BUSY_PIN    18
#define RST_PIN     17
#define DC_PIN      9
#define CS_PIN      8
#define UNCOLORED   0xFF
#define COLORED     0x00
InlandEPD epd(CS_PIN, DC_PIN, RST_PIN, BUSY_PIN);
Paint paint(epd.buffer.data(), EPD_WIDTH, EPD_HEIGHT);

// Line by line printing
int text_row = 1;
void printline(const char* line, int x = 0);

// Network
WiFiUDP wifiUdp;
NTP ntp(wifiUdp);

void setup() {
    Serial.begin(115200);
    // Toggle this to wait for host connection
    while (!Serial) {
        ; // wait for connection
    }

    pinMode(NEOPIXEL_POWER, OUTPUT);
    digitalWrite(NEOPIXEL_POWER, HIGH);

    pixels.begin();
    pixels.setBrightness(20);
    pixels.fill(0x000000);
    pixels.show();

    paint.SetRotate(ROTATE_90);
    paint.Clear(UNCOLORED);
    epd.init();
    epd.update_full();


    esp_sleep_source_t wakeup_reason = esp_sleep_get_wakeup_cause();
    Serial.print("wakeup reason: ");
    Serial.println(wakeup_reason);

    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();

    if (wakeup_reason != ESP_SLEEP_WAKEUP_TIMER) {
        Serial.println("Not awaking from deep sleep, will sync time");

        WiFi.persistent(false);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        while (WiFi.status() != WL_CONNECTED) {
            Serial.print(".");
            delay(250);
        }
        Serial.println("Connected.");

        Serial.println("Start NTP...");
        ntp.begin("pool.ntp.org");
        ntp.stop();
        Serial.println(ntp.formattedTime("Got UTC time: %c"));

        timeval tv;
        tv.tv_sec = ntp.utc();
        settimeofday(&tv, NULL);

        WiFi.disconnect(true, true);
        Serial.println("Disconnected and powered off radio");
    } else {
        Serial.println("Awake from deep sleep");
    }

    timeval tv;
    gettimeofday(&tv, NULL);

    tm timeinfo;
    localtime_r(&tv.tv_sec, &timeinfo);

    char timeString[15];
    strftime(timeString, sizeof(timeString), "%I:%M:%S", &timeinfo);
    printline(timeString);

    int secondsremaining = 60 - timeinfo.tm_sec;

    Serial.end();
    Serial.flush();

    esp_deep_sleep(secondsremaining * 1000 * 1000);
}

void loop() {
}

static const int FONT_HEIGHT_PX = 18;
void printline(const char* line, int x) {
    if (text_row >= (EPD_WIDTH / FONT_HEIGHT_PX)) {
        text_row = 1;
        epd.wipe();
    }
    int y = text_row * FONT_HEIGHT_PX - FONT_HEIGHT_PX/2;
    text_row++;

    Serial.println(line);
    paint.DrawStringAt(x, y, line, &Font24, COLORED);
    epd.update_partial();
}
