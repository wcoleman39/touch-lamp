/****************************************************************************************************************************
  defines.h
  For ESP8266 / ESP32 boards

  ESPAsync_WiFiManager_Lite (https://github.com/khoih-prog/ESPAsync_WiFiManager_Lite) is a library
  for the ESP32/ESP8266 boards to enable store Credentials in EEPROM/SPIFFS/LittleFS for easy
  configuration/reconfiguration and autoconnect/autoreconnect of WiFi and other services without Hardcoding.

  Built by Khoi Hoang https://github.com/khoih-prog/ESPAsync_WiFiManager_Lite
  Licensed under MIT license
 *****************************************************************************************************************************/

#pragma once

#ifndef defines_h
#define defines_h

#if !(ESP8266 || ESP32)
#error This code is intended to run only on the ESP8266/ESP32 boards ! Please check your Tools->Board setting.
#endif

#define IO_USERNAME "williamjcoleman"
#define IO_KEY "aio_kRsG77Lc2YCsQROBaaO43ZiNBSBc"
#define IO_FEED_NAME "touch_lamp"

#define LAMP_NAME_MAX_LEN "16"

#define LAMP_LED_PIN 5
#define LAMP_LED_NUM 12
#define STATUS_LED_PIN PIN_NEOPIXEL
#define STATUS_LED_NUM NEOPIXEL_NUM
#define LAMP_TOUCH_PIN T6

#define LAMP_LED_TIMEOUT    900000L    // 15 min

#define STATUS_LED_BRIGHTNESS   50         // 0-255
#define STATUS_COLOR_CONFIG     0x0000FF   // blue
#define STATUS_COLOR_WARNING    0xDFFF00   // yellow
#define STATUS_COLOR_ERROR      0xFF0000   // red
#define STATUS_COLOR_OK         0x00FF00   // green

#define NTP_SERVER "pool.ntp.org"

#define JSON_DOC_BYTES 128
#define JSON_KEY_LAMP_NAME "n"
#define JSON_KEY_COLOR "c"
#define JSON_KEY_TIMESTAMP "t"
#define JSON_KEY_EVENT_TYPE "e"

#define JSON_VAL_EVENT_TYPE_TOUCH "T"
#define JSON_VAL_EVENT_TYPE_START "S"
#define JSON_VAL_EVENT_TYPE_ERROR "E"

/* Comment this out to disable prints and save space */
#define ESP_WM_LITE_DEBUG_OUTPUT Serial
#define WAIT_FOR_SERIAL true

#define _ESP_WM_LITE_LOGLEVEL_ 5

/////////////////////////////////////////////

// Force some params
#define TIMEOUT_RECONNECT_WIFI 60000L

// Permit running CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET times before reset hardware
// to permit user another chance to config. Only if Config Data is valid.
// If Config Data is invalid, this has no effect as Config Portal will persist
#define RESET_IF_CONFIG_TIMEOUT true

// Permitted range of user-defined CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET between 2-100
#define CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET 5

// Config Timeout 10s. Applicable only if Config Data is Valid
#define CONFIG_TIMEOUT 10000L

/////////////////////////////////////////////

#define USE_CAPTIVE_PORTAL  false

/////////////////////////////////////////////

#define TOUCH_THRESHOLD 150   // ESP32s2
#define TOUCH_HOLD_TO_RESET_MS 5000L
#define TOUCH_HOLD_CLEAR_CONFIG_MS 15000L

/////////////////////////////////////////////

// Max times to try WiFi per loop() iteration. To avoid blocking issue in loop()
// Default 1 if not defined, and minimum 1.
#define MAX_NUM_WIFI_RECON_TRIES_PER_LOOP 2

// Default no interval between recon WiFi if lost
// Max permitted interval will be 10mins
// Uncomment to use. Be careful, WiFi reconnect will be delayed if using this method
// Only use whenever urgent tasks in loop() can't be delayed. But if so, it's better you have to rewrite your code, e.g. using higher priority tasks.
// #define WIFI_RECON_INTERVAL                   30000

/////////////////////////////////////////////

// Permit reset hardware if no WiFi to permit user another chance to access Config Portal.
#define RESET_IF_NO_WIFI true
#define WIFI_MULTI_CONNECT_WAITING_MS 500L

/////////////////////////////////////////////

// From 2-15
#define MAX_SSID_IN_LIST 10

/////////////////////////////////////////////

// MS to check status of lamp
#define CHECK_STATUS_INTERVAL 5000L

/////////////////////////////////////////////

#define CP_HOSTNAME_BASE "Touch_Lamp"
#define CP_HOSTNAME_MAXLEN 24

#ifdef LED_BUILTIN
#define LED_PIN LED_BUILTIN
#else
#define LED_PIN 13
#endif

#endif // defines_h

/////////////////////////////////////////////

#ifndef DEFAULT_LED_BRIGHTNESS
#define DEFAULT_LED_BRIGHTNESS 20
#endif

#define NEO_W_VALUE 100
