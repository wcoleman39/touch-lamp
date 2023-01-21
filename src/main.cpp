// #include "defines.h"
// #include "webpage.h"
// #include "WifiManagerDebug.h"
// #include <WiFi.h>
// #include <ESPAsyncWebServer.h>
// #include <AdafruitIO_WiFi.h>
// #include <Preferences.h>
// #include <DNSServer.h>
// #include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include "WifiManager.h"
#include "Credentials.h"
#include <ArduinoJson.h>
#include <time.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

ESPAsync_WiFiManager_Lite *ESPAsync_WiFiManager;

Adafruit_NeoPixel *lamp_led = new Adafruit_NeoPixel(LAMP_LED_NUM, LAMP_LED_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel *status_led = new Adafruit_NeoPixel();

bool touch_detected = false;

bool ignore_own_io_touch_events = true; // ignore self touch events by default

void externalEventHandler(AdafruitIO_Data *data);
void internalTouchHandler(aio_status_t io_status);

AdafruitIO_Feed *io_feed;
bool gotten = false;
bool sent = false;

aio_status_t io_status;
aio_status_t io_status_prev;

unsigned long last_io_touch_event_ms = 0;
unsigned long touch_hold_start_ms = 0;
unsigned long last_touch_ms = 0;

void gotTouch()
{
    touch_detected = true;
}

void signal_status(Adafruit_NeoPixel *led, uint32_t status_color)
{
    const uint8_t blinks = 5;
    uint32_t current_color = led->getPixelColor(0);
    static unsigned long checkstatus_timeout = 0;

    for (uint8_t i = 0; i < blinks; i++)
    {
        if (led->getPixelColor(0) == led->Color(0, 0, 0))
        {
            led->fill(status_color);
        }
        else
        {
            led->fill();
        }
        led->show();
        delay(100);
    }
    led->fill(current_color);
    led->show();
}

void rainbow(int loops, int duration_ms)
{
    const uint32_t max_hue = 65536;
    int wait_ms = (256 * duration_ms) / (loops * max_hue);
    for (long firstPixelHue = 0; firstPixelHue < (loops * max_hue); firstPixelHue += 256)
    {
        status_led->rainbow(firstPixelHue);
        lamp_led->rainbow(firstPixelHue);
        status_led->show(); // Update status LED with new contents
        lamp_led->show();   // Update lamp LED with new contents
        delay(wait_ms);     // Pause for a moment
    }

    // turn off
    status_led->fill();
    lamp_led->fill();
    status_led->show();
    lamp_led->show();
}

void escapeJsonStringForIO(String *json)
{
    json->replace('"', '\'');
}

void descapeJsonStringFromIO(String *json)
{
    json->replace('\'', '"');
}

void externalEventHandler(AdafruitIO_Data *data)
{
    // react to new events over Adafruit IO
    String jsondata = String(data->toString());
    descapeJsonStringFromIO(&jsondata);
    ESP_WML_LOGINFO(F("AIO event received."));
    ESP_WML_LOGDEBUG1(F("AIO event: "), jsondata);

    StaticJsonDocument<JSON_DOC_BYTES> jsondoc;
    DeserializationError error = deserializeJson(jsondoc, jsondata.c_str());

    if (error)
    {
        ESP_WML_LOGERROR1(F("deserializeJson() failed: "), error.f_str());
        delete &jsondoc;
        return;
    }

    String lamp_name = jsondoc[JSON_KEY_LAMP_NAME];
    String event_type = jsondoc[JSON_KEY_EVENT_TYPE];

    // do not react to own events
    if (ignore_own_io_touch_events && (lamp_name == ESPAsync_WiFiManager->getBoardName()))
    {
        ESP_WML_LOGDEBUG(F("Disregarding self-triggered event."))
        return;
    }

    // touch events
    if (event_type == JSON_VAL_EVENT_TYPE_TOUCH)
    {
        last_io_touch_event_ms = millis();

        String color_str = jsondoc[JSON_KEY_COLOR];
        if (color_str == NULL)
        {
            ESP_WML_LOGERROR(F("Received color touch event with invalid color value."))
            return;
        }

        color_str.replace("#", "0x");
        uint32_t color_hex = strtoul(color_str.c_str(), NULL, 0);

        ESP_WML_LOGINFO3(F("Changing color to: "), color_str, F("(from: "), lamp_name);
        lamp_led->fill(color_hex);
        lamp_led->show();
    }

    // delete &jsondoc;
    return;
}

void internalTouchHandler(aio_status_t io_status)
{
    ESP_WML_LOGINFO("Local touch event detected.");

    // connected to WiFi/AIO
    if (io_status >= AIO_CONNECTED)
    {
        last_io_touch_event_ms = millis();

        String color_str = ESPAsync_WiFiManager->getAssignedColor();
        color_str.replace("#", "0x");
        uint32_t color_hex = strtoul(color_str.c_str(), NULL, 0);

        ESP_WML_LOGINFO1(F("Changing color to: "), color_str);
        lamp_led->fill(color_hex);
        lamp_led->show();

        StaticJsonDocument<JSON_DOC_BYTES> jsondoc;

        time_t now;
        bool ret = time(&now);

        jsondoc[JSON_KEY_EVENT_TYPE] = JSON_VAL_EVENT_TYPE_TOUCH;
        jsondoc[JSON_KEY_LAMP_NAME] = ESPAsync_WiFiManager->getBoardName();
        jsondoc[JSON_KEY_COLOR] = ESPAsync_WiFiManager->getAssignedColor();

        String jsondata;
        serializeJson(jsondoc, jsondata);

        Serial.println(jsondata);
        escapeJsonStringForIO(&jsondata);
        if (io_feed->save(jsondata))
        {
            ESP_WML_LOGDEBUG(F("IO save() success."));
            sent = true;
        }
        else
        {
            ESP_WML_LOGERROR(F("IO save() failure."));
        }
    }

    // delete &jsondoc;
    return;
}

void setup()
{
    // Debug console
    Serial.begin(115200);
    delay(1000);

#ifndef WAIT_FOR_SERIAL
#define WAIT_FOR_SERIAL false
#endif
#if WAIT_FOR_SERIAL
    while (!Serial)
        delay(500);
    delay(500);
#endif

    ESP_WML_LOGDEBUG("Starting...");
    Serial.println("Starting...");

    ESP_WML_LOGDEBUG("Disabling brownout...");
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    pinMode(LAMP_LED_PIN, OUTPUT);
    pinMode(LAMP_TOUCH_PIN, INPUT);

    ESP_WML_LOGDEBUG(F("ESP Touch Interrupt attached."));
    touchAttachInterrupt(LAMP_TOUCH_PIN, gotTouch, TOUCH_THRESHOLD);

    ESP_WML_LOGDEBUG(F("Lamp Neo pixel start."));
    lamp_led->begin(); // INITIALIZE lamp LED strip

    ESP_WML_LOGDEBUG1(F("Starting Touch Lamp with "), ARDUINO_BOARD);
    ESPAsync_WiFiManager = new ESPAsync_WiFiManager_Lite();

    // Optional to change default AP IP(192.168.4.1) and channel(10)
    // ESPAsync_WiFiManager->setConfigPortalIP(IPAddress(192, 168, 120, 1));
    ESPAsync_WiFiManager->setConfigPortalChannel(0);
    status_led = ESPAsync_WiFiManager->get_status_led();

    // setup AIO feed and start WiFi
    ESP_WML_LOGDEBUG(F("Connecting to AIO Feed."));
    io_feed = ESPAsync_WiFiManager->IO_Wifi.feed(IO_FEED_NAME);
    io_feed->onMessage(externalEventHandler);

    ESP_WML_LOGDEBUG(F("Starting Wifi Manager"));
    ESPAsync_WiFiManager->begin();

    String brightness_str = ESPAsync_WiFiManager->getConfiguredBrightness();
    long brightness = strtol(brightness_str.c_str(), NULL, 10);
    if (brightness <= 0)
    {
        brightness = DEFAULT_LED_BRIGHTNESS;
    }
    ESP_WML_LOGDEBUG1(F("Setting lamp brightness: "), brightness);
    lamp_led->setBrightness(brightness);

    // led color
    if (ESPAsync_WiFiManager->isConfigMode())
    {
        rainbow(5, 2000);
    }

    ESP_WML_LOGDEBUG1(F("Configuring Time with: "), NTP_SERVER);
    configTime(0, 0, NTP_SERVER);
}

void loop()
{
    ESPAsync_WiFiManager->run();

    if (io_status >= AIO_CONNECTED)
    {
        if (io_status != io_status_prev)
        {
            Serial.println("Getting last touch event.");
            ignore_own_io_touch_events = false;
            io_feed->get();
            ignore_own_io_touch_events = true;
        }

        // check if lamp should turn off
        if ((lamp_led->getPixelColor(0)) != lamp_led->Color(0, 0, 0, 0) &&
            (last_io_touch_event_ms != 0 && (LAMP_LED_TIMEOUT < (millis() - last_io_touch_event_ms))))
        {
            ESP_WML_LOGINFO2(F("No touch event in the last "), round(double(LAMP_LED_TIMEOUT / 1000)), F("s, shutting off lamp."));

            // timeout since last AIO event, turn the lamp off
            lamp_led->fill();
            lamp_led->show();

            // blink status LED
            signal_status(status_led, STATUS_COLOR_WARNING);
            status_led->fill();
            status_led->show();
        }
    }
    io_status_prev = io_status;

    if (touch_detected)
    {
        touch_detected = false;
        if (touchInterruptGetLastStatus(LAMP_TOUCH_PIN))
        {
            ESP_WML_LOGINFO(F(" --- Local touch event: pressed."));
            internalTouchHandler(io_status);
            touch_hold_start_ms = millis();
        }
        else
        {
            ESP_WML_LOGINFO(F(" --- Local touch event: released."));
            unsigned long touch_duration_ms = millis() - touch_hold_start_ms;
            Serial.print("Touch duration: ");
            Serial.print(round(double(touch_duration_ms / 1000)), DEC);
            Serial.print("s\n");

            // if held for >20s, clear config and restart
            if (touch_hold_start_ms != 0 && (TOUCH_HOLD_CLEAR_CONFIG_MS < touch_duration_ms))
            {
                Serial.println("Clearing configuration and entering Config Portal.");
                ESPAsync_WiFiManager->clearConfigData();
                ESPAsync_WiFiManager->resetAndEnterConfigPortal();
            }

            // if held for >5 secs, reset and enter config mode
            else if (!ESPAsync_WiFiManager->isConfigMode())
            {
                if (touch_hold_start_ms != 0 && (TOUCH_HOLD_TO_RESET_MS < touch_duration_ms))
                {
                    Serial.println("Resetting and entering Persistent Config Portal.");
                    ESPAsync_WiFiManager->resetAndEnterConfigPortalPersistent();
                }
            }

            // reset last touch marker
            touch_hold_start_ms = 0;
        }
    }

    // main loop delay
    delay(80);
}