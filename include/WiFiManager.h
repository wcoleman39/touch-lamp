/****************************************************************************************************************************
  Based off:

  ESPAsync_WiFiManager_Lite.h
  For ESP8266 / ESP32 boards

  ESPAsync_WiFiManager_Lite (https://github.com/khoih-prog/ESPAsync_WiFiManager_Lite) is a library
  for the ESP32/ESP8266 boards to enable store Credentials in EEPROM/SPIFFS/LittleFS for easy
  configuration/reconfiguration and autoconnect/autoreconnect of WiFi and other services without Hardcoding.

  Built by Khoi Hoang https://github.com/khoih-prog/ESPAsync_WiFiManager_Lite
  Licensed under MIT license

  Version: 1.9.0
 *****************************************************************************************************************************/

#pragma once

#ifndef ESPAsync_WiFiManager_Lite_h
#define ESPAsync_WiFiManager_Lite_h

#if !(defined(ESP8266) || defined(ESP32))
#error This code is intended to run on the ESP8266 or ESP32 platform! Please check your Tools->Board setting.
#elif (ARDUINO_ESP32S2_DEV || ARDUINO_FEATHERS2 || ARDUINO_ESP32S2_THING_PLUS || ARDUINO_MICROS2 || \
       ARDUINO_METRO_ESP32S2 || ARDUINO_MAGTAG29_ESP32S2 || ARDUINO_FUNHOUSE_ESP32S2 ||             \
       ARDUINO_ADAFRUIT_FEATHER_ESP32S2_NOPSRAM || ARDUINO_ADAFRUIT_QTPY_ESP32S2)
// #warning Using ESP32_S2.
#define USING_ESP32_S2 true
#elif (ARDUINO_ESP32C3_DEV)
#if (defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 2))
#warning Using ESP32_C3 using core v2.0.0+. Either LittleFS, SPIFFS or EEPROM OK.
#else
#warning Using ESP32_C3 using core v1.0.6-. To follow library instructions to install esp32-c3 core. Only SPIFFS and EEPROM OK.
#endif

#warning You have to select Flash size 2MB and Minimal APP (1.3MB + 700KB) for some boards
#define USING_ESP32_C3 true
#elif (defined(ARDUINO_ESP32S3_DEV) || defined(ARDUINO_ESP32_S3_BOX) || defined(ARDUINO_TINYS3) || \
       defined(ARDUINO_PROS3) || defined(ARDUINO_FEATHERS3))
#warning Using ESP32_S3. To install esp32-s3-support branch if using core v2.0.2-.
#define USING_ESP32_S3 true
#endif

#ifndef ESP_ASYNC_WIFI_MANAGER_LITE_VERSION
#define ESP_ASYNC_WIFI_MANAGER_LITE_VERSION "ESPAsync_WiFiManager_Lite v1.9.0"

#define ESP_ASYNC_WIFI_MANAGER_LITE_VERSION_MAJOR 1
#define ESP_ASYNC_WIFI_MANAGER_LITE_VERSION_MINOR 9
#define ESP_ASYNC_WIFI_MANAGER_LITE_VERSION_PATCH 0

#define ESP_ASYNC_WIFI_MANAGER_LITE_VERSION_INT 1009000
#endif

#include "defines.h"
#include "webpage.h"
#include "WifiManagerDebug.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AdafruitIO_WiFi.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <Adafruit_NeoPixel.h>

#undef min
#undef max
// #include <algorithm>
// #include <esp_wifi.h>

#define Preferences_Namespace "WM_Prefs"
#define FS_Name Preferences_Namespace
#define PREFS_KEY_FORCED_CP "forcedCP"
#define PREFS_KEY_CP_ON_RESET "configOnReset"
#define PREFS_KEY_SSID "ssid"
#define PREFS_KEY_PWD "pwd"
#define PREFS_KEY_BOARD_NAME "boardName"
#define PREFS_KEY_COLOR "color"
#define PREFS_KEY_BRIGHTNESS "brightness"
#define PREFS_KEY_HEADER "header"
#define PREFS_KEY_CHECKSUM "checksum"
Preferences prefs;

#define HTTP_PORT 80
#define DNS_PORT 53

uint32_t getChipID();
uint32_t getChipOUI();

#if defined(ESP_getChipId)
#undef ESP_getChipId
#endif

#if defined(ESP_getChipOUI)
#undef ESP_getChipOUI
#endif

#define ESP_getChipId() getChipID()
#define ESP_getChipOUI() getChipOUI()

//////////////////////////////////////////////

// New from v1.3.0

#if !defined(MAX_SSID_IN_LIST)
#define MAX_SSID_IN_LIST 10
#elif (MAX_SSID_IN_LIST < 2)
#warning Parameter MAX_SSID_IN_LIST defined must be >= 2 - Reset to 10
#undef MAX_SSID_IN_LIST
#define MAX_SSID_IN_LIST 10
#elif (MAX_SSID_IN_LIST > 15)
#warning Parameter MAX_SSID_IN_LIST defined must be <= 15 - Reset to 10
#undef MAX_SSID_IN_LIST
#define MAX_SSID_IN_LIST 10
#endif

// NEW
#define MAX_ID_LEN 5
#define MAX_DISPLAY_NAME_LEN 16

typedef struct
{
    char id[MAX_ID_LEN + 1];
    char displayName[MAX_DISPLAY_NAME_LEN + 1];
    char *pdata;
    uint8_t maxlen;
} MenuItem;

#define SSID_MAX_LEN 32
// WPA2 passwords can be up to 63 characters long.
#define PASS_MAX_LEN 64

typedef struct
{
    char wifi_ssid[SSID_MAX_LEN];
    char wifi_pw[PASS_MAX_LEN];
} WiFi_Credentials;

#define NUM_WIFI_CREDENTIALS 1

// Configurable items besides fixed Header, just add board_name
#define NUM_CONFIGURABLE_ITEMS ((2 * NUM_WIFI_CREDENTIALS) + 3)

////////////////

#define HEADER_MAX_LEN 16
#define BOARD_NAME_MAX_LEN 24
#define COLOR_VALUE_MAX_LEN 10
#define BRIGHTNESS_MAX_LEN 8

#define ESP_WM_LITE_HEADER "TOUCH_LAMP"
#define WM_NO_CONFIG ""
#define WM_DEFAULT_BRIGHTNESS "20"

class WM_Configuration
{
public:
    String header;
    String wifi_ssid;
    String wifi_pw;
    String board_name;
    String assigned_color;
    String brightness;

    WM_Configuration()
    {
        this->header = ESP_WM_LITE_HEADER;
        this->wifi_ssid = WM_NO_CONFIG;
        this->wifi_pw = WM_NO_CONFIG;
        this->board_name = WM_NO_CONFIG;
        this->assigned_color = WM_NO_CONFIG;
        this->brightness = WM_DEFAULT_BRIGHTNESS;
    }
};

#if USE_CAPTIVE_PORTAL
class CaptiveRequestHandler : public AsyncWebHandler
{
public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request)
    {
        // request->addInterestingHeader("ANY");
        return true;
    }

    void handleRequest(AsyncWebServerRequest *request)
    {
        AsyncResponseStream *response = request->beginResponseStream("text/html");
        response->print("<!DOCTYPE html><html><head><title>Captive Portal</title></head><body>");
        response->print("<p>This is out captive portal front page.</p>");
        response->printf("<p>You were trying to reach: http://%s%s</p>", request->host().c_str(), request->url().c_str());
        response->printf("<p>Try opening <a href='http://%s'>this link</a> instead</p>", WiFi.softAPIP().toString().c_str());
        response->print("</body></html>");
        request->send(response);

        // const char *ip_addr = WiFi.softAPIP().toString().c_str();
        // AsyncResponseStream *response = request->beginResponseStream("text/html");
        // response->print("<!DOCTYPE html><html><head><title>Redirect</title>");
        // // response->printf("<meta http-equiv='refresh' content='5; url='http://%s'/>", ip_addr);
        // response->print("</head><body><p>Attempting to redirect to Touch Lamp Config Portal.</p>");
        // response->printf("<p>Try opening <a href='http://%s'>this link</a> to access the Touch Lamp Portal if not redirected automatically.</p>", ip_addr);
        // response->print("</body></html>");
        // request->send(response);
    }
};
#endif

/// New from v1.0.4
extern bool LOAD_DEFAULT_CONFIG_DATA;
extern WM_Configuration defaultConfig;

const char ESP_WM_LITE_OPTION_START[] /*PROGMEM*/ = "<option>";
const char ESP_WM_LITE_OPTION_END[] /*PROGMEM*/ = "</option>";

//////////////////////////////////////////

const char WM_HTTP_HEAD_TEXT_HTML[] PROGMEM = "text/html";
const char WM_HTTP_CACHE_CONTROL[] PROGMEM = "Cache-Control";
const char WM_HTTP_NO_STORE[] PROGMEM = "no-cache, no-store, must-revalidate";
const char WM_HTTP_PRAGMA[] PROGMEM = "Pragma";
const char WM_HTTP_NO_CACHE[] PROGMEM = "no-cache";
const char WM_HTTP_EXPIRES[] PROGMEM = "Expires";

//////////////////////////////////////////

uint32_t getChipID()
{
    uint64_t chipId64 = 0;

    for (int i = 0; i < 6; i++)
    {
        chipId64 |= (((uint64_t)ESP.getEfuseMac() >> (40 - (i * 8))) & 0xff) << (i * 8);
    }

    return (uint32_t)(chipId64 & 0xFFFFFF);
}

//////////////////////////////////////////

uint32_t getChipOUI()
{
    uint64_t chipId64 = 0;

    for (int i = 0; i < 6; i++)
    {
        chipId64 |= (((uint64_t)ESP.getEfuseMac() >> (40 - (i * 8))) & 0xff) << (i * 8);
    }

    return (uint32_t)(chipId64 >> 24);
}

//////////////////////////////////////////

String IPAddressToString(const IPAddress &_address)
{
    String str = String(_address[0]);
    str += ".";
    str += String(_address[1]);
    str += ".";
    str += String(_address[2]);
    str += ".";
    str += String(_address[3]);
    return str;
}

//////////////////////////////////////////

class ESPAsync_WiFiManager_Lite
{
public:
    ESPAsync_WiFiManager_Lite() {}

    //////////////////////////////////////////

    ~ESPAsync_WiFiManager_Lite()
    {
        if (server)
        {
            delete server;
        }
    }

    //////////////////////////////////////////

    void connectWiFi(const char *ssid, const char *pass)
    {
        ESP_WML_LOGINFO1(F("Con2:"), ssid);
        WiFi.mode(WIFI_STA);

        if (static_IP != IPAddress(0, 0, 0, 0))
        {
            ESP_WML_LOGINFO(F("UseStatIP"));
            WiFi.config(static_IP, static_GW, static_SN, static_DNS1, static_DNS2);
        }

        this->setHostname();

        if (this->IO_Wifi.status() < AIO_CONNECTED)
        {
            if (pass && strlen(pass))
            {
                this->IO_Wifi = AdafruitIO_WiFi(IO_USERNAME, IO_KEY, ssid, pass);
                this->IO_Wifi.connect();
            }
            else
            {
                // must have a password
                ESP_WML_LOGERROR(F("Error: Wifi Password not set."))
            }
        }

        while (this->IO_Wifi.status() < AIO_CONNECTED)
        {
            delay(500);
        }

        ESP_WML_LOGINFO(F("Conn2WiFi"));
        displayWiFiData();
    }

    //////////////////////////////////////////

    void start_status_led()
    {
        ESP_WML_LOGDEBUG(F("Status Neo pixel start."));

        // draw onboard status LED power pin high to use
        pinMode(NEOPIXEL_POWER, OUTPUT);
        digitalWrite(NEOPIXEL_POWER, HIGH);

        this->_status_led.begin(); // INITIALIZE onboard status LED
        this->_status_led.setBrightness(STATUS_LED_BRIGHTNESS);
    }

    Adafruit_NeoPixel *get_status_led()
    {
        return &_status_led;
    }

    //////////////////////////////////////////

    void begin(const char *ssid,
               const char *pass)
    {
        ESP_WML_LOGERROR(F("conW"));
        connectWiFi(ssid, pass);
    }

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // Pin D2 mapped to pin GPIO2/ADC12 of ESP32, control on-board LED
#endif

#define LED_OFF LOW
#define LED_ON HIGH

#define PASSWORD_MIN_LEN 8

    //////////////////////////////////////////

    void begin(const char *iHostname = CP_HOSTNAME_BASE)
    {
#define TIMEOUT_CONNECT_WIFI 30000

        // Turn OFF
        pinMode(LED_BUILTIN, OUTPUT);
        digitalWrite(LED_BUILTIN, LED_OFF);

        bool noConfigPortal = true;

        if (LOAD_DEFAULT_CONFIG_DATA)
        {
            ESP_WML_LOGDEBUG(F("======= Start Default Config Data ======="));
            displayConfigData(defaultConfig);
        }

        WiFi.mode(WIFI_STA);

        this->_hostname = iHostname;
        this->_hostname += String(ESP_getChipId(), HEX);
        this->_hostname.toUpperCase();
        this->_hostname.substring(0, CP_HOSTNAME_MAXLEN - 1);
        this->setHostname();

        ESP_WML_LOGINFO1(F("Hostname="), this->_hostname);

        hadConfigData = getConfigData();

        isForcedConfigPortal = isForcedCP();

        // start every other reset in config mode
        if (configOnResetFlag())
        {
            ESP_WML_LOGDEBUG("bg: configOnReset = true");
            setConfigOnResetFlag(false);
            startConfigurationMode();
            return;
        }
        else
        {
            setConfigOnResetFlag(true);
        }

        if (hadConfigData && noConfigPortal && (!isForcedConfigPortal))
        {
            hadConfigData = true;

            ESP_WML_LOGDEBUG(noConfigPortal ? F("bg: noConfigPortal = true") : F("bg: noConfigPortal = false"));

            for (uint16_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
            {
                if (WM_Config.wifi_pw.length() >= PASSWORD_MIN_LEN)
                {
                    this->IO_Wifi = AdafruitIO_WiFi(IO_USERNAME, IO_KEY, WM_Config.wifi_ssid.c_str(), WM_Config.wifi_pw.c_str());
                }
            }

            // if (connectIOWiFi() >= AIO_CONNECTED)
            if (this->IO_Wifi.run() >= AIO_CONNECTED)
            {
                ESP_WML_LOGINFO(F("bg: AIOWiFi OK."));
            }
            else
            {
                ESP_WML_LOGINFO(F("bg: Fail2connect AIOWiFi"));
                // failed to connect to WiFi, will start configuration mode
                startConfigurationMode();
            }
        }
        else
        {
            ESP_WML_LOGDEBUG(isForcedConfigPortal ? F("bg: isForcedConfigPortal = true") : F("bg: isForcedConfigPortal = false"));

            // If not persistent => clear the flag so that after reset. no more CP, even CP not entered and saved
            if (persForcedConfigPortal)
            {
                ESP_WML_LOGINFO1(F("bg:Stay forever in CP:"),
                                 isForcedConfigPortal ? F("Forced-Persistent") : (noConfigPortal ? F("No ConfigDat") : F("DRD/MRD")));
            }
            else
            {
                ESP_WML_LOGINFO1(F("bg:Stay forever in CP:"),
                                 isForcedConfigPortal ? F("Forced-non-Persistent") : (noConfigPortal ? F("No ConfigDat") : F("DRD/MRD")));
                clearForcedCP();
            }

            hadConfigData = isForcedConfigPortal ? true : (noConfigPortal ? false : true);

            // failed to connect to WiFi, will start configuration mode
            startConfigurationMode();
        }
    }

    //////////////////////////////////////////

#ifndef TIMEOUT_RECONNECT_WIFI
#define TIMEOUT_RECONNECT_WIFI 10000L
#else
    // Force range of user-defined TIMEOUT_RECONNECT_WIFI between 10-60s
#if (TIMEOUT_RECONNECT_WIFI < 10000L)
#warning TIMEOUT_RECONNECT_WIFI too low. Reseting to 10000
#undef TIMEOUT_RECONNECT_WIFI
#define TIMEOUT_RECONNECT_WIFI 10000L
#elif (TIMEOUT_RECONNECT_WIFI > 60000L)
#warning TIMEOUT_RECONNECT_WIFI too high. Reseting to 60000
#undef TIMEOUT_RECONNECT_WIFI
#define TIMEOUT_RECONNECT_WIFI 60000L
#endif
#endif

#ifndef RESET_IF_CONFIG_TIMEOUT
#define RESET_IF_CONFIG_TIMEOUT true
#endif

#ifndef CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET
#define CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET 10
#else
    // Force range of user-defined TIMES_BEFORE_RESET between 2-100
#if (CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET < 2)
#warning CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET too low. Reseting to 2
#undef CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET
#define CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET 2
#elif (CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET > 100)
#warning CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET too high. Reseting to 100
#undef CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET
#define CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET 100
#endif
#endif

    //////////////////////////////////////////

#if !defined(WIFI_RECON_INTERVAL)
#define WIFI_RECON_INTERVAL 0 // default 0s between reconnecting WiFi
#else
#if (WIFI_RECON_INTERVAL < 0)
#define WIFI_RECON_INTERVAL 0
#elif (WIFI_RECON_INTERVAL > 600000)
#define WIFI_RECON_INTERVAL 600000 // Max 10min
#endif
#endif

    void run()
    {
        static short retryTimes = 0;
        static bool wifiDisconnectedOnce = false;
        static unsigned long checkstatus_timeout = 0;
        const uint32_t curMillis = millis();

        if (!(this->configuration_mode) && (curMillis > checkstatus_timeout))
        {
            ESP_WML_LOGDEBUG(F("run status: regular mode"));

            if (this->IO_Wifi.status() < AIO_CONNECTED)
            {
                // Lost connection in running. Give chance to reconfig.
                // Check WiFi status every CHECK_STATUS_INTERVAL ms and update status
                // Check twice to be sure wifi disconnected is real
                if (wifiDisconnectedOnce)
                {
                    wifiDisconnectedOnce = false;

                    ESP_WML_LOGINFO(F("run: AIOWiFi lost. Reconnect AIOWiFi"));
                    if (connectIOWiFi() >= AIO_CONNECTED)
                    {
                        ESP_WML_LOGINFO(F("run: AIOWiFi reconnected"));
                    }
                }
                else
                {
                    ESP_WML_LOGDEBUG(F("run: AIOWiFi lost once, try again"));
                    wifiDisconnectedOnce = true;
                }
            }
            // connected to AIO & WiFi
            else
            {
                ESP_WML_LOGDEBUG(F("configuration_mode = false"));
                configuration_mode = false;
                ESP_WML_LOGINFO(F("run: got AIOWiFi back"));
            }

            checkstatus_timeout = curMillis + CHECK_STATUS_INTERVAL;
        }

        if (this->IO_Wifi.status() < AIO_CONNECTED)
        {
#if USE_CAPTIVE_PORTAL
            dnsServer.processNextRequest();
#endif

            if (this->configuration_mode && (configTimeout == 0 || (curMillis < configTimeout)))
            {
                // If configTimeout but user hasn't connected to configWeb => try to reconnect WiFi
                // But if user has connected to configWeb, stay there until done, then reset hardware
                Serial.print(".");
                retryTimes = 0;
                delay(1);
                return;
            }
            else
            {
                // If we're here but still in configuration_mode, permit running TIMES_BEFORE_RESET times before reset hardware
                // to permit user another chance to config.
                if ((this->configuration_mode) && (configTimeout != 0))
                {
                    ESP_WML_LOGDEBUG(F("run: Config Portal Timeout"));
                    if (++retryTimes <= CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET)
                    {
                        ESP_WML_LOGINFO1(F("run: AIOWiFi lost, configTimeout. Connect AIOWiFi. Retry#:"), retryTimes);
                    }
                    else
                    {
                        ESP_WML_LOGERROR(F("run: AIOWiFi lost, resetting."));
                        resetFunc();
                    }
                }

                // Not in config mode, try reconnecting before forcing to config mode
                if (this->IO_Wifi.status() < AIO_CONNECTED)
                {
                    ESP_WML_LOGINFO(F("run: AIOWiFi lost. Reconnect AIOWiFi"));
                    // if (this->connectIOWiFi() >= AIO_CONNECTED)
                    if (this->IO_Wifi.run(WIFI_MULTI_CONNECT_WAITING_MS) < AIO_CONNECTED)
                    {
                        static uint8_t numWiFiReconTries = 0;
                        numWiFiReconTries++;
                        ESP_WML_LOGWARN1(F("WiFi not connected, check #"), numWiFiReconTries);
#if RESET_IF_NO_WIFI
                        if (numWiFiReconTries >= MAX_NUM_WIFI_RECON_TRIES_PER_LOOP)
                        {
                            ESP_WML_LOGERROR2(F("WiFi not connected "), numWiFiReconTries, F(", resetting."));
                            resetFunc();
                        }
#endif
                    }
                }
            }
        }

        else if (this->configuration_mode)
        {
            ESP_WML_LOGDEBUG(F("configuration_mode = false"));
            this->configuration_mode = false;
            ESP_WML_LOGINFO(F("run: got AIOWiFi back"));
        }

        // display status
        static unsigned long status_check_interval = 0;
        static unsigned short checks = 0;
        static aio_status_t io_status;

        // check only every CHECK_STATUS_INTERVAL ms
        if (curMillis > status_check_interval)
        {
            ESP_WML_LOGDEBUG5(F("run => curMillis: "), curMillis, F("; checkstatus_timeout: "), checkstatus_timeout, F("; configTimeout: "), configTimeout);
            ESP_WML_LOGDEBUG1(F("WiFi mode: "), WiFi.getMode());
            ESP_WML_LOGDEBUG3(F("CP:SSID="), WiFi.softAPSSID(), F(",PW="), portal_pass);
            ESP_WML_LOGDEBUG3(F("IP="), WiFi.softAPIP().toString(), ",ch=", WiFi.channel());

            checks++;
            io_status = this->IO_Wifi.status();

            if (this->configuration_mode)
            {
                Serial.print(F("C")); // H means connected to AIO (& WiFi)
                this->_status_led.fill(STATUS_COLOR_CONFIG);
                this->_status_led.show();
            }
            else if (io_status >= AIO_CONNECTED)
            {
                Serial.print(F("H")); // H means connected to AIO (& WiFi)
                this->_status_led.fill(STATUS_COLOR_OK);
                this->_status_led.show();
            }
            else if (io_status == AIO_NET_CONNECTED)
            {
                Serial.print(F("W")); // W means connected to WiFi but not AIO
                this->_status_led.fill(STATUS_COLOR_WARNING);
                this->_status_led.show();
            }
            else
            {
                Serial.print(F("F")); // F means not connected to WiFi
                this->_status_led.fill(STATUS_COLOR_ERROR);
                this->_status_led.show();
            }
            if (checks == 80)
            {
                Serial.println();
                checks = 0;
            }
            else if (checks++ % 10 == 0)
            {
                Serial.print(F(" "));
            }

            status_check_interval = curMillis + CHECK_STATUS_INTERVAL;
        }
    }

    //////////////////////////////////////////////

    void setHostname()
    {
        WiFi.setHostname(this->_hostname.c_str());
    }

    //////////////////////////////////////////////

    void setConfigPortalIP(const IPAddress &portalIP = IPAddress(192, 168, 4, 1))
    {
        portal_apIP = portalIP;
    }

    //////////////////////////////////////////////

    void setConfigPortal(const String &ssid = "", const String &pass = "")
    {
        portal_ssid = ssid;
        portal_pass = pass;
    }

    //////////////////////////////////////////////

#define MIN_WIFI_CHANNEL 1
#define MAX_WIFI_CHANNEL 11 // Channel 13 is flaky, because of bad number 13 ;-)

    int setConfigPortalChannel(const int &channel = 1)
    {
        // If channel < MIN_WIFI_CHANNEL - 1 or channel > MAX_WIFI_CHANNEL => channel = 1
        // If channel == 0 => will use random channel from MIN_WIFI_CHANNEL to MAX_WIFI_CHANNEL
        // If (MIN_WIFI_CHANNEL <= channel <= MAX_WIFI_CHANNEL) => use it
        if ((channel < MIN_WIFI_CHANNEL - 1) || (channel > MAX_WIFI_CHANNEL))
            WiFiAPChannel = 1;
        else if ((channel >= MIN_WIFI_CHANNEL - 1) && (channel <= MAX_WIFI_CHANNEL))
            WiFiAPChannel = channel;

        return WiFiAPChannel;
    }

    //////////////////////////////////////////////

    String getWiFiSSID()
    {
        if (!hadConfigData)
            getConfigData();

        // return (String(ESP_WM_LITE_config.WiFi_Creds[index].wifi_ssid));
        return (WM_Config.wifi_ssid);
    }

    //////////////////////////////////////////////

    String getWiFiPW()
    {
        if (!hadConfigData)
            getConfigData();

        // return (String(ESP_WM_LITE_config.WiFi_Creds[index].wifi_pw));
        return WM_Config.wifi_pw;
    }

    //////////////////////////////////////////////

    String getBoardName()
    {
        if (!hadConfigData)
            getConfigData();

        // return (String(ESP_WM_LITE_config.board_name));
        return WM_Config.board_name;
    }

    //////////////////////////////////////////////

    String getAssignedColor()
    {
        if (!hadConfigData)
            getConfigData();

        // return (String(ESP_WM_LITE_config.color));
        return WM_Config.assigned_color;
    }

    //////////////////////////////////////////////

    String getConfiguredBrightness()
    {
        if (!hadConfigData)
            getConfigData();

        // return (String(ESP_WM_LITE_config.brightness));
        return WM_Config.brightness;
    }

    //////////////////////////////////////////////

    String localIP()
    {
        ipAddress = IPAddressToString(WiFi.localIP());
        return ipAddress;
    }

    //////////////////////////////////////////////

    void clearConfigData()
    {
        // memset(&ESP_WM_LITE_config, 0, sizeof(ESP_WM_LITE_config));
        // memset(&WM_Config, 0, sizeof(WM_Config));
        WM_Config = WM_Configuration();
        saveConfigData();
    }

    //////////////////////////////////////////////

    bool isConfigDataValid()
    {
        return hadConfigData;
    }

    //////////////////////////////////////////////

    bool isConfigMode()
    {
        return configuration_mode;
    }

    //////////////////////////////////////////////

    void resetAndEnterConfigPortal()
    {
        persForcedConfigPortal = false;
        setForcedCP(persForcedConfigPortal);

        // Delay then reset the ESP8266 after save data
        delay(1000);
        resetFunc();
    }

    //////////////////////////////////////////////

    // This will keep CP forever, until you successfully enter CP, and Save data to clear the flag.
    void resetAndEnterConfigPortalPersistent()
    {
        persForcedConfigPortal = true;
        setForcedCP(persForcedConfigPortal);

        // Delay then reset the ESP8266 after save data
        delay(1000);
        resetFunc();
    }

    //////////////////////////////////////////////

    void resetFunc()
    {
        ESP_WML_LOGINFO(F("Resetting..."));
        delay(1000);
        ESP.restart();
    }

    //////////////////////////////////////

    AdafruitIO_WiFi IO_Wifi = AdafruitIO_WiFi(IO_USERNAME, IO_KEY, "", "");

private:
    Adafruit_NeoPixel _status_led = Adafruit_NeoPixel(STATUS_LED_NUM, STATUS_LED_PIN, NEO_GRB + NEO_KHZ800);

    String ipAddress = "0.0.0.0";
    AsyncWebServer *server = NULL;
    
#if USE_CAPTIVE_PORTAL
    DNSServer dnsServer;
    CaptiveRequestHandler *_captiveRequestHandler = new CaptiveRequestHandler();
#endif

    bool configuration_mode = false;

    unsigned long configTimeout;
    bool hadConfigData = false;

    bool isForcedConfigPortal = false;
    bool persForcedConfigPortal = isForcedConfigPortal;

    WM_Configuration WM_Config;

    uint16_t totalDataSize = 0;

    String macAddress = "";

    IPAddress portal_apIP = IPAddress(192, 168, 4, 1);
    int WiFiAPChannel = 10;

    String portal_ssid = "";
    String portal_pass = "";

    IPAddress static_IP = IPAddress(0, 0, 0, 0);
    IPAddress static_GW = IPAddress(0, 0, 0, 0);
    IPAddress static_SN = IPAddress(255, 255, 255, 0);
    IPAddress static_DNS1 = IPAddress(0, 0, 0, 0);
    IPAddress static_DNS2 = IPAddress(0, 0, 0, 0);

    //////////////////////////////////////

    int WiFiNetworksFound = 0; // Number of SSIDs found by WiFi scan, including low quality and duplicates
    int *indices;              // WiFi network data, filled by scan (SSID, BSSID)
    String ListOfSSIDs = "";   // List of SSIDs found by scan, in HTML <option> format
    String _hostname;

    //////////////////////////////////////

    void displayConfigData(const WM_Configuration &configData)
    {
        ESP_WML_LOGERROR5(F("Hdr="), configData.header, F(",SSID="), configData.wifi_ssid,
                          F(",PW="), "********");
        ESP_WML_LOGERROR1(F("BName="), configData.board_name);
        ESP_WML_LOGERROR1(F("Color="), configData.assigned_color);
        ESP_WML_LOGERROR1(F("Brightness="), configData.brightness);
    }

    //////////////////////////////////////

    void displayWiFiData()
    {
        ESP_WML_LOGERROR3(F("SSID="), WiFi.SSID(), F(",RSSI="), WiFi.RSSI());
        ESP_WML_LOGERROR1(F("IP="), localIP());
        ESP_WML_LOGERROR1(F("AIO="), this->IO_Wifi.statusText());
    }

    //////////////////////////////////////

    bool isWiFiConfigValid()
    {
        bool ret = false;

        if (WM_Config.wifi_ssid == WM_NO_CONFIG || WM_Config.wifi_ssid.length() == 0)
        {
            ret = false;
        }
        else if (WM_Config.wifi_pw == WM_NO_CONFIG || WM_Config.wifi_pw.length() == 0)
        {
            ret = false;
        }
        else if (WM_Config.board_name == WM_NO_CONFIG)
        {
            ret = false;
        }
        else if (WM_Config.header == WM_NO_CONFIG)
        {
            ret = false;
        }
        else if (WM_Config.assigned_color == WM_NO_CONFIG)
        {
            ret = false;
        }
        else if (WM_Config.brightness == WM_NO_CONFIG)
        {
            ret = false;
        }
        else
        {
            ret = true;
        }

        if (!ret)
        {
            ESP_WML_LOGERROR(F("Invalid Stored WiFi Config Data"));
            hadConfigData = false;
            WM_Config = WM_Configuration();
        }
        return ret;
    }

    //////////////////////////////////////////////

    void setForcedCP(const bool isPersistent)
    {
        ESP_WML_LOGINFO(F("SaveForcedCP "));
        prefs.begin(Preferences_Namespace, false);
        prefs.putBool(PREFS_KEY_FORCED_CP, isPersistent);
        prefs.end();
        ESP_WML_LOGINFO(F("OK"));
    }

    //////////////////////////////////////////////

    void clearForcedCP()
    {
        setForcedCP(false);
    }

    //////////////////////////////////////////////

    bool isForcedCP()
    {
        prefs.begin(Preferences_Namespace, true);
        return prefs.getBool(PREFS_KEY_FORCED_CP, false);
        prefs.end();
    }

    //////////////////////////////////////////////

    void setConfigOnResetFlag(const bool flag)
    {
        ESP_WML_LOGDEBUG3(F("set "), PREFS_KEY_CP_ON_RESET, F("="), flag ? "true" : "false");
        prefs.begin(Preferences_Namespace, false);
        prefs.putBool(PREFS_KEY_CP_ON_RESET, flag);
        prefs.end();
        ESP_WML_LOGINFO(F("OK"));
    }

    //////////////////////////////////////////////

    bool configOnResetFlag()
    {
        prefs.begin(Preferences_Namespace, true);
        bool ret = prefs.getBool(PREFS_KEY_CP_ON_RESET, false);
        prefs.end();
        ESP_WML_LOGDEBUG2(PREFS_KEY_CP_ON_RESET, F("="), ret ? "true" : "false");
        return ret;
    }

    //////////////////////////////////////////////

    // save all data
    void saveConfigData()
    {
        ESP_WML_LOGINFO(F("SavePrefs "));
        size_t totalBytes = 0;
        prefs.begin(Preferences_Namespace, false);
        totalBytes += prefs.putString(PREFS_KEY_BOARD_NAME, WM_Config.board_name);
        totalBytes += prefs.putString(PREFS_KEY_SSID, WM_Config.wifi_ssid);
        totalBytes += prefs.putString(PREFS_KEY_PWD, WM_Config.wifi_pw);
        totalBytes += prefs.putString(PREFS_KEY_COLOR, WM_Config.assigned_color);
        totalBytes += prefs.putString(PREFS_KEY_BRIGHTNESS, WM_Config.brightness);
        totalBytes += prefs.putString(PREFS_KEY_HEADER, WM_Config.header);
        prefs.end();
        ESP_WML_LOGINFO(F(totalBytes + " bytes saved"));
        ESP_WML_LOGINFO(F("OK"));
    }

    //////////////////////////////////////////////

    // Return false if init new EEPROM or SPIFFS. No more need trying to connect. Go directly to config mode
    bool getConfigData()
    {
        hadConfigData = false;

        if (LOAD_DEFAULT_CONFIG_DATA)
        {
            WM_Config = defaultConfig;
            WM_Config.header = ESP_WM_LITE_HEADER;

            // Including config and dynamic data, and assume valid
            saveConfigData();

            ESP_WML_LOGINFO(F("======= Start Loaded Config Data ======="));
            displayConfigData(WM_Config);

            // Don't need Config Portal anymore
            return true;
        }

        else
        {
            // Load data from Preferences
            ESP_WML_LOGINFO(F("loading prefs..."))
            prefs.begin(Preferences_Namespace, true);
            WM_Config.board_name = prefs.getString(PREFS_KEY_BOARD_NAME, WM_NO_CONFIG);
            WM_Config.wifi_ssid = prefs.getString(PREFS_KEY_SSID, WM_NO_CONFIG);
            WM_Config.wifi_pw = prefs.getString(PREFS_KEY_PWD, WM_NO_CONFIG);
            WM_Config.header = prefs.getString(PREFS_KEY_HEADER, ESP_WM_LITE_HEADER);
            WM_Config.assigned_color = prefs.getString(PREFS_KEY_COLOR, WM_NO_CONFIG);
            WM_Config.brightness = prefs.getString(PREFS_KEY_BRIGHTNESS, WM_NO_CONFIG);
            prefs.end();
            ESP_WML_LOGINFO(F("OK"));
            displayConfigData(WM_Config);

            // Get config data. If "blank" or NULL, set false flag and exit
            if (!isWiFiConfigValid())
            {
                return false;
            }

            ESP_WML_LOGINFO(F("======= Start Stored Config Data ======="));
            displayConfigData(WM_Config);
        }

        if (!isWiFiConfigValid())
        {
            // If SSID, PW ="blank" or NULL, stay in config mode forever until having config Data.
            ESP_WML_LOGDEBUG0(F("!isWiFiConfigValid()"));
            return false;
        }
        else
        {
            displayConfigData(WM_Config);
        }

        return true;
    }

    //////////////////////////////////////////////

    // Max times to try WiFi per loop() iteration. To avoid blocking issue in loop()
    // Default 1 and minimum 1.
#if !defined(MAX_NUM_WIFI_RECON_TRIES_PER_LOOP)
#define MAX_NUM_WIFI_RECON_TRIES_PER_LOOP 1
#else
#if (MAX_NUM_WIFI_RECON_TRIES_PER_LOOP < 1)
#define MAX_NUM_WIFI_RECON_TRIES_PER_LOOP 1
#endif
#endif

    aio_status_t connectIOWiFi()
    {
#if (USING_ESP32_S2 || USING_ESP32_C3)
#define WIFI_MULTI_1ST_CONNECT_WAITING_MS 500L
#else
        // For ESP32 core v1.0.6, must be >= 500
#define WIFI_MULTI_1ST_CONNECT_WAITING_MS 800L
#endif

#define WIFI_MULTI_CONNECT_WAITING_MS 500L

        aio_status_t io_status;

        ESP_WML_LOGINFO(F("Connecting AIOWiFi..."));
        WiFi.mode(WIFI_STA);
        this->setHostname();

        int i = 0;
        ESP_WML_LOGINFO(F("AIOWiFi run..."));
        io_status = this->IO_Wifi.run();
        ESP_WML_LOGDEBUG(F("done"));
        delay(WIFI_MULTI_1ST_CONNECT_WAITING_MS);

        uint8_t numWiFiReconTries = 0;

        while ((io_status < AIO_CONNECTED) && (numWiFiReconTries++ < MAX_NUM_WIFI_RECON_TRIES_PER_LOOP))
        {
            ESP_WML_LOGDEBUG(F("Not connected, first check."));
            // wait and then check again
            delay(WIFI_MULTI_CONNECT_WAITING_MS);
            io_status = this->IO_Wifi.status();

            if (io_status >= AIO_CONNECTED)
            {
                ESP_WML_LOGWARN1(F("WiFi connected after time: "), i);
                ESP_WML_LOGWARN3(F("SSID="), WiFi.SSID(), F(",RSSI="), WiFi.RSSI());
                ESP_WML_LOGWARN3(F("Channel="), WiFi.channel(), F(",IP="), WiFi.localIP());
                break;
            }
            else
            {
                ESP_WML_LOGERROR(F("WiFi not connected, second check."));
#if RESET_IF_NO_WIFI
                resetFunc();
#endif
            }
        }

        return io_status;
    }

    //////////////////////////////////////////////

    void createHTML(String &root_html_template, boolean hadConfigData)
    {
        String pitem;
        ListOfSSIDs = "";

        root_html_template = String(FPSTR(CP_HTML));

        ESP_WML_LOGDEBUG1(WiFiNetworksFound, F(" SSIDs found, generating HTML now"));

        for (int i = 0, list_items = 0; (i < WiFiNetworksFound) && (list_items < MAX_SSID_IN_LIST); i++)
        {
            if (indices[i] == -1)
            {
                continue; // skip duplicates and those that are below the required quality
            }

            String ssid = WiFi.SSID(indices[i]);

            ListOfSSIDs += ESP_WM_LITE_OPTION_START;
            ListOfSSIDs += ssid;
            ListOfSSIDs += ESP_WM_LITE_OPTION_END;

            list_items++; // Count number of suitable, distinct SSIDs to be included in list
        }

        ESP_WML_LOGDEBUG(ListOfSSIDs);

        if (ListOfSSIDs == "") // No SSID found or none was good enough
            ListOfSSIDs = "<option disabled style='color:gray'>No suitable WiFi networks available</option>";

        root_html_template.replace("[[id_list]]", ListOfSSIDs);
        root_html_template.replace("[[nm_max]]", LAMP_NAME_MAX_LEN);

        if (hadConfigData)
        {
            root_html_template.replace("[[nm]]", WM_Config.board_name);
            root_html_template.replace("[[id]]", WM_Config.wifi_ssid);
            root_html_template.replace("[[pw]]", "");
            root_html_template.replace("[[col]]", WM_Config.assigned_color);
            root_html_template.replace("[[brt]]", WM_Config.brightness);

            root_html_template.replace("[[ph_nm]]", WM_Config.board_name + " (unchanged)");
            root_html_template.replace("[[ph_id]]", WM_Config.wifi_ssid + " (unchanged)");
            root_html_template.replace("[[ph_pw]]", "******** (unchanged)");
        }
        else
        {
            root_html_template.replace("[[nm]]", "");
            root_html_template.replace("[[id]]", "");
            root_html_template.replace("[[pw]]", "");
            root_html_template.replace("[[id1]]", "");
            root_html_template.replace("[[pw1]]", "");
            root_html_template.replace("[[col]]", "");
            root_html_template.replace("[[brt]]", "");

            root_html_template.replace("[[ph_nm]]", "Lamp Name");
            root_html_template.replace("[[ph_id]]", "WiFi Name/SSID");
            root_html_template.replace("[[ph_pw]]", "Password (min 8 chars)");
        }

        return;
    }

    //////////////////////////////////////////////

    void printWSRequest(AsyncWebServerRequest *request)
    {
        ESP_WML_LOGDEBUG1(F("method: "), request->methodToString());
        ESP_WML_LOGDEBUG1(F("content type: "), request->contentType());
        ESP_WML_LOGDEBUG1(F("content len: "), request->contentLength());
        ESP_WML_LOGDEBUG1(F("url: "), request->urlDecode(request->url()));

        for (size_t i = 0; i < request->headers(); i++)
        {
            ESP_WML_LOGDEBUG1(F("header: "), request->getHeader(i)->toString());
        }
        for (size_t i = 0; i < request->args(); i++)
        {
        }
        for (size_t i = 0; i < request->params(); i++)
        {
            ESP_WML_LOGDEBUG3(F("params: "), request->getParam(i)->name(), F(": "), request->getParam(i)->value());
        }
    }

    void handleRequest(AsyncWebServerRequest *request)
    {
        if (request)
        {
            printWSRequest(request);

            String result;
            createHTML(result, hadConfigData);

            ESP_WML_LOGDEBUG1(F("h:Repl:"), result);

            // Reset configTimeout to stay here until finished.
            configTimeout = 0;

            ESP_WML_LOGDEBUG1(F("h:HTML page size:"), result.length());
            ESP_WML_LOGDEBUG1(F("h:HTML="), result);

            AsyncWebServerResponse *response = request->beginResponse(200, FPSTR(WM_HTTP_HEAD_TEXT_HTML), result);
            response->addHeader(FPSTR(WM_HTTP_CACHE_CONTROL), FPSTR(WM_HTTP_NO_STORE));

            response->addHeader(FPSTR(WM_HTTP_PRAGMA), FPSTR(WM_HTTP_NO_CACHE));
            response->addHeader(FPSTR(WM_HTTP_EXPIRES), "-1");

            request->send(response);

            return;
        }
    }
    void handleConfigRequest(AsyncWebServerRequest *request)
    {
        if (request)
        {
            // printWSRequest(request);

            static int number_items_Updated = 0;

            if (request->hasParam("nm", true))
            {
                ESP_WML_LOGDEBUG(F("h:repl nm"));
                number_items_Updated++;

                WM_Config.board_name = request->getParam("nm", true)->value();
            }

            if (request->hasParam("id", true))
            {
                ESP_WML_LOGDEBUG(F("h:repl id"));
                number_items_Updated++;

                WM_Config.wifi_ssid = request->getParam("id", true)->value();
            }

            if (request->hasParam("pw", true))
            {
                ESP_WML_LOGDEBUG(F("h:repl pw"));
                number_items_Updated++;

                WM_Config.wifi_pw = request->getParam("pw", true)->value();
            }

            if (request->hasParam("col", true))
            {
                ESP_WML_LOGDEBUG(F("h:repl col"));
                number_items_Updated++;

                WM_Config.assigned_color = request->getParam("col", true)->value();
            }

            if (request->hasParam("brt", true))
            {
                ESP_WML_LOGDEBUG(F("h:repl brt"));
                number_items_Updated++;

                WM_Config.brightness = request->getParam("brt", true)->value();
            }

            ESP_WML_LOGDEBUG1(F("h:items updated ="), number_items_Updated);
            request->send(200, WM_HTTP_HEAD_TEXT_HTML, "OK");

            if (number_items_Updated > 0)

            {
                ESP_WML_LOGERROR1(F("h:Updating Preferences:"), Preferences_Namespace);
                saveConfigData();

                // Done with CP, Clear CP Flag here if forced
                if (isForcedConfigPortal)
                {
                    clearForcedCP();
                }

                ESP_WML_LOGERROR(F("h:Rst"));

                // Delay then reset the board after save data
                delay(1000);
                resetFunc(); // call reset
            }
        } // if (server)
    }

    //////////////////////////////////////////////

#ifndef CONFIG_TIMEOUT
#warning Default CONFIG_TIMEOUT = 60s
#define CONFIG_TIMEOUT 60000L
#endif

    void startConfigurationMode()
    {
        ESP_WML_LOGINFO(F("Starting Config Portal."));
        configTimeout = 0; // To allow user input in CP
        WiFiNetworksFound = scanWifiNetworks(&indices);

        if ((portal_ssid == "") || portal_pass == "")
        {
            String chipID = String(ESP_getChipId(), HEX);
            chipID.toUpperCase();

            // TOUCH_LAMP_0786F1
            portal_ssid = ESP_WM_LITE_HEADER;
            portal_ssid += "_";
            portal_ssid += chipID;

            // !My_TOUCH_LAMP_0786F1
            portal_pass = "!My_";
            portal_pass += portal_ssid;
        }

        ESP_WML_LOGDEBUG(F("WiFi mode: AP."));
        WiFi.mode(WIFI_AP);

        // New
        delay(100);

        static int channel;

        // Use random channel if WiFiAPChannel == 0
        if (WiFiAPChannel == 0)
        {
            // channel = random(MAX_WIFI_CHANNEL) + 1;
            channel = (millis() % MAX_WIFI_CHANNEL) + 1;
        }
        else
            channel = WiFiAPChannel;

        // softAPConfig() must be put before softAP() for ESP8266 core v3.0.0+ to work.
        // ESP32 or ESP8266is core v3.0.0- is OK either way
        ESP_WML_LOGDEBUG(F("Start WiFi AP."));
        WiFi.softAP(portal_ssid.c_str(), portal_pass.c_str(), channel);
        delay(100); // ref: https://github.com/espressif/arduino-esp32/issues/985#issuecomment-359157428
        WiFi.softAPConfig(portal_apIP, portal_apIP, IPAddress(255, 255, 255, 0));

#if USE_CAPTIVE_PORTAL
        ESP_WML_LOGDEBUG(F("DNS Server start."));
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
        ESP_WML_LOGDEBUG(F("DNS Server addHandler."));
        server->addHandler(_captiveRequestHandler).setFilter(ON_AP_FILTER); // only when requested from AP
        ESP_WML_LOGDEBUG(F("done adding handler."));
#endif

        ESP_WML_LOGERROR3(F("CP:SSID="), portal_ssid, F(",PW="), portal_pass);
        ESP_WML_LOGERROR3(F("IP="), portal_apIP.toString(), ",ch=", channel);


        if (!server)
        {
            server = new AsyncWebServer(HTTP_PORT);
        }

        // See https://stackoverflow.com/questions/39803135/c-unresolved-overloaded-function-type?rq=1
        if (server)
        {
            server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
                       { handleRequest(request); });
            server->on("/config", HTTP_POST, [this](AsyncWebServerRequest *request)
                       { handleConfigRequest(request); });
            ESP_WML_LOGINFO(F("Begin AP Server."));
            server->begin();
        }

        // If there is no saved config Data, stay in config mode forever until having config Data.
        // or SSID, PW, Server,Token ="nothing"
        if (hadConfigData)
        {
            configTimeout = millis() + CONFIG_TIMEOUT;

            ESP_WML_LOGDEBUG3(F("s:millis() = "), millis(), F(", configTimeout = "), configTimeout);
        }
        else
        {
            configTimeout = 0;
            ESP_WML_LOGDEBUG(F("s:configTimeout = 0"));
        }

        ESP_WML_LOGDEBUG(F("configuration_mode = true"));
        configuration_mode = true;
    }

    // Source code adapted from https://github.com/khoih-prog/ESP_WiFiManager/blob/master/src/ESP_WiFiManager-Impl.h

    int _paramsCount = 0;
    int _minimumQuality = -1;
    bool _removeDuplicateAPs = true;

    //////////////////////////////////////////

    void swap(int *thisOne, int *thatOne)
    {
        int tempo;

        tempo = *thatOne;
        *thatOne = *thisOne;
        *thisOne = tempo;
    }

    //////////////////////////////////////////

    void setMinimumSignalQuality(const int &quality)
    {
        _minimumQuality = quality;
    }

    //////////////////////////////////////////

    // if this is true, remove duplicate Access Points - default true
    void setRemoveDuplicateAPs(const bool &removeDuplicates)
    {
        _removeDuplicateAPs = removeDuplicates;
    }

    //////////////////////////////////////////

    // Scan for WiFiNetworks in range and sort by signal strength
    // space for indices array allocated on the heap and should be freed when no longer required
    int scanWifiNetworks(int **indicesptr)
    {
        ESP_WML_LOGDEBUG(F("Scanning Network"));

        int n = WiFi.scanNetworks();

        ESP_WML_LOGDEBUG1(F("scanWifiNetworks: Done, Scanned Networks n = "), n);

        // KH, Terrible bug here. WiFi.scanNetworks() returns n < 0 => malloc( negative == very big ) => crash!!!
        // In .../esp32/libraries/WiFi/src/WiFiType.h
        // #define WIFI_SCAN_RUNNING   (-1)
        // #define WIFI_SCAN_FAILED    (-2)
        // if (n == 0)
        if (n <= 0)
        {
            ESP_WML_LOGDEBUG(F("No network found"));
            return (0);
        }
        else
        {
            // Allocate space off the heap for indices array.
            // This space should be freed when no longer required.
            int *indices = (int *)malloc(n * sizeof(int));

            if (indices == NULL)
            {
                ESP_WML_LOGDEBUG(F("ERROR: Out of memory"));
                *indicesptr = NULL;
                return (0);
            }

            *indicesptr = indices;

            // sort networks
            for (int i = 0; i < n; i++)
            {
                indices[i] = i;
            }

            ESP_WML_LOGDEBUG(F("Sorting"));

            // RSSI SORT
            // old sort
            for (int i = 0; i < n; i++)
            {
                for (int j = i + 1; j < n; j++)
                {
                    if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i]))
                    {
                        // std::swap(indices[i], indices[j]);
                        //  Using locally defined swap()
                        swap(&indices[i], &indices[j]);
                    }
                }
            }

            ESP_WML_LOGDEBUG(F("Removing Dup"));

            // remove duplicates ( must be RSSI sorted )
            if (_removeDuplicateAPs)
            {
                String cssid;

                for (int i = 0; i < n; i++)
                {
                    if (indices[i] == -1)
                        continue;

                    cssid = WiFi.SSID(indices[i]);

                    for (int j = i + 1; j < n; j++)
                    {
                        if (cssid == WiFi.SSID(indices[j]))
                        {
                            ESP_WML_LOGDEBUG1("DUP AP:", WiFi.SSID(indices[j]));
                            indices[j] = -1; // set dup aps to index -1
                        }
                    }
                }
            }

            for (int i = 0; i < n; i++)
            {
                if (indices[i] == -1)
                    continue; // skip dups

                int quality = getRSSIasQuality(WiFi.RSSI(indices[i]));

                if (!(_minimumQuality == -1 || _minimumQuality < quality))
                {
                    indices[i] = -1;
                    ESP_WML_LOGDEBUG(F("Skipping low quality"));
                }
            }

            ESP_WML_LOGWARN(F("WiFi networks found:"));

            for (int i = 0; i < n; i++)
            {
                if (indices[i] == -1)
                    continue; // skip dups
                else
                    ESP_WML_LOGWARN5(i + 1, ": ", WiFi.SSID(indices[i]), ", ", WiFi.RSSI(i), "dB");
            }

            return (n);
        }
    }

    //////////////////////////////////////////

    int getRSSIasQuality(const int &RSSI)
    {
        int quality = 0;

        if (RSSI <= -100)
        {
            quality = 0;
        }
        else if (RSSI >= -50)
        {
            quality = 100;
        }
        else
        {
            quality = 2 * (RSSI + 100);
        }

        return quality;
    }

    //////////////////////////////////////////
};

#endif // ESPAsync_WiFiManager_Lite_h
