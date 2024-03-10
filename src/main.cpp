#include <Arduino.h>
#include "Adafruit_NeoPixel.h"

#include "InlandEPD.h"
#include "epdpaint.h"

#include "WiFi.h"
#include "NTP.h"
#include "time.h"
#include "esp_sntp.h"
#include "timeformat.hpp"

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
int render(bool fullUpdate);
void printLine(std::string line, int x = 0);

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
    // int next_refresh = render(false);
    int next_refresh = render(wakeup_reason != ESP_SLEEP_WAKEUP_TIMER);

    Serial.flush();
    Serial.end();

    fetchRtcTime();
    int secondsremaining = 60 - human_time.tm_sec;
    secondsremaining += (next_refresh - human_time.tm_min) * 60;

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

bool inRange(int query, int lower_inclusive, int upper_exclusive) {
    return (query >= lower_inclusive) && (query < upper_exclusive);
}

std::vector<std::string> chopToFit(std::string sentence, int max_chars = 18) {
    std::vector<std::string> lines;

    Serial.println(("chopping sentence: \"" + sentence + "\"").c_str());

    if (sentence.length() < max_chars) {
        lines.push_back(sentence);
    } else {
        int start = 0;

        while (start < sentence.length()) {
            Serial.println(sentence.substr(start).c_str( ));

            // advance past leading spaces
            while (sentence[start] == ' ') {
                if (start >= sentence.length()) {
                    break;
                }

                start++;
            }

            int end;
            if (start + max_chars > sentence.length()) {
                end = sentence.length();
            } else {
                end = start + max_chars;

                // look backwards from the end for a word boundary
                while (end > start && sentence[end] != ' ') {
                    end--;
                }

                if (end == start) {
                    break;
                }

            }

            // exclude the trailing space
            lines.push_back(sentence.substr(start, end - start));

            // advance past trailing space
            start = end;
        }

    }

    Serial.println("Chopped sentence: ");
    for (int i = 0; i < lines.size(); i++) {
        Serial.print("\t");
        Serial.println(lines[i].c_str());
    }

    return lines;
}


int render(bool fullUpdate) {
    if (!fullUpdate) epd.update_partial(true);

    std::string sentence_time = "it's ";
    sentence_time.reserve(255);

    int minute = human_time.tm_min;
    if (inRange(minute, 4, 57)) {
        sentence_time += "about ";
    }

    int next_refresh;
    if (inRange(minute, 0, 4)) {
        sentence_time += "just after ";
        next_refresh = 4;
    } else if (inRange(minute, 4, 7)) {
        sentence_time += "five ";
        next_refresh = 7;
    } else if (inRange(minute, 7, 14)) {
        sentence_time += "ten ";
        next_refresh = 14;
    } else if (inRange(minute, 14, 17)) {
        sentence_time += "quarter ";
        next_refresh = 17;
    } else if (inRange(minute, 17, 24)) {
        sentence_time += "twenty ";
        next_refresh = 24;
    } else if (inRange(minute, 24, 27)) {
        sentence_time += "twenty-five ";
        next_refresh = 27;
    } else if (inRange(minute, 27, 34)) {
        sentence_time += "half ";
        next_refresh = 34;
    } else if (inRange(minute, 34, 37)) {
        sentence_time += "thirty-five ";
        next_refresh = 37;
    } else if (inRange(minute, 37, 44)) {
        sentence_time += "twenty ";
        next_refresh = 44;
    } else if (inRange(minute, 44, 47)) {
        sentence_time += "quarter ";
        next_refresh = 47;
    } else if (inRange(minute, 47, 54)) {
        sentence_time += "ten ";
        next_refresh = 54;
    } else if (inRange(minute, 54, 57)) {
        sentence_time += "five ";
        next_refresh = 57;
    } else {
        sentence_time += "almost ";
        next_refresh = 60;
    }

    if (inRange(minute, 4, 37)) {
        sentence_time += "past ";
    }

    if (inRange(minute, 37, 57)) {
        sentence_time += "to ";
    }

    if (inRange(minute, 0, 37)) {
        sentence_time += hours_to_strings[human_time.tm_hour];
    } else {
        sentence_time += hours_to_strings[(human_time.tm_hour + 1) % 24];
    }

    if (inRange(human_time.tm_hour, 0, 1)) {
        // pass - midnight
    } else if (inRange(human_time.tm_hour, 1, 12)) {
        sentence_time += "in the morning ";
    } else if (inRange(human_time.tm_hour, 12, 13)) {
        // pass - noon
    } else if (inRange(human_time.tm_hour, 13, 17)) {
        sentence_time += "in the afternoon ";
    } else if (inRange(human_time.tm_hour, 17, 19)) {
        sentence_time += "in the evening ";
    } else if (inRange(human_time.tm_hour, 19, 23)) {
        sentence_time += "at night ";
    } else {
        // pass - midnight
    }

    sentence_time += "on ";
    sentence_time += days_to_strings[human_time.tm_wday];
    sentence_time += months_to_strings[human_time.tm_mon];
    sentence_time += day_to_ordinal[human_time.tm_mday - 1];
    sentence_time += '.';

    auto lines = chopToFit(sentence_time);

    for (int i = 0; i < lines.size(); i++) {
        printLine(lines[i]);
    }

    if (fullUpdate) {
        epd.update_full();
    } else {
        epd.update_partial(false);
    }

    return next_refresh;
}

int text_row = 1;
static const int FONT_HEIGHT_PX = 16;
void printLine(std::string line, int x) {
    // if (text_row >= (EPD_WIDTH / FONT_HEIGHT_PX)) {
    //     text_row = 1;
    //     paint.Clear(UNCOLORED);
    // }
    int y = text_row * FONT_HEIGHT_PX - FONT_HEIGHT_PX/2;
    text_row++;

    Serial.println(line.c_str());
    paint.DrawStringAt(x, y, line.c_str(), &Font20, COLORED);
}
