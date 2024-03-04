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

#ifndef WAIT_FOR_HOST
    #define WAIT_FOR_HOST false
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

// Network
WiFiUDP wifiUdp;
NTP ntp(wifiUdp);

// time and rendering
tm human_time;
timeval posix_timeval;

void init(bool wait_for_host = true);
bool attemptNtpUpdate();
void fetchRtcTime();
void render(bool fullUpdate);
void printLine(const char* line, int x = 0);

void setup() {
    init(WAIT_FOR_HOST);

    esp_sleep_source_t wakeup_reason = esp_sleep_get_wakeup_cause();
    Serial.print("wakeup reason: ");
    Serial.println(wakeup_reason);

    if (wakeup_reason != ESP_SLEEP_WAKEUP_TIMER) {
        Serial.println("Not awaking from deep sleep, will sync time");
        attemptNtpUpdate();
    } else {
        Serial.println("Awake from deep sleep");
    }

    fetchRtcTime();
    render(false);
    // render(wakeup_reason != ESP_SLEEP_WAKEUP_TIMER);

    fetchRtcTime();
    int secondsremaining = 60 - human_time.tm_sec;

    Serial.end();
    Serial.flush();

    esp_deep_sleep(secondsremaining * 1000 * 1000);
}

void loop() {}

void init(bool wait_for_host) {
    Serial.begin(115200);
    if (wait_for_host) {
        while (!Serial) {
            ; // wait for connection
        }
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

    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();
}

bool attemptNtpUpdate() {
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

    WiFi.disconnect(true, true);
    Serial.println("Disconnected and powered off radio");

    posix_timeval.tv_sec = ntp.utc();
    settimeofday(&posix_timeval, NULL);

    return true;
}

void fetchRtcTime() {
    gettimeofday(&posix_timeval, NULL);
    localtime_r(&posix_timeval.tv_sec, &human_time);
}

void render(bool fullUpdate) {
    if (!fullUpdate) epd.update_partial(true);

    char timeString[15];
    strftime(timeString, sizeof(timeString), "%I:%M:%S", &human_time);
    printLine(timeString);

    if (fullUpdate) {
        epd.update_full();
    } else {
        epd.update_partial(false);
    }
}

int text_row = 1;
static const int FONT_HEIGHT_PX = 18;
void printLine(const char* line, int x) {
    if (text_row >= (EPD_WIDTH / FONT_HEIGHT_PX)) {
        text_row = 1;
        paint.Clear(UNCOLORED);
    }
    int y = text_row * FONT_HEIGHT_PX - FONT_HEIGHT_PX/2;
    text_row++;

    Serial.println(line);
    paint.DrawStringAt(x, y, line, &Font24, COLORED);
}
