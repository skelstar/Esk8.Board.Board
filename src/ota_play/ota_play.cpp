#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <elapsedMillis.h>
#include <Button2.h>
#include <WiFiManager.h>

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define OTA_PIN 32

Button2 otaButton(OTA_PIN);

void setup()
{
  Serial.begin(115200);

  // WiFi.mode(WIFI_STA);
  // wm.resetSettings();

  //************************** OTA BEGINS

  DEBUG("Ready");
}

const unsigned long TIME_WAITING_FOR_OTA = 30 * 1000;

void loop()
{
  otaButton.loop();

  if (otaButton.isPressed())
  {
    DEBUG("OTA mode started");

    WiFiManager wm;
    WiFi.mode(WIFI_STA);
    wm.setTimeout(120); // 2 minutes
    wm.autoConnect("esk8.Board.AP");

    ArduinoOTA
        .onStart([]() {
          String type;
          if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
          else // U_SPIFFS
            type = "filesystem";

          // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
          Serial.println("Start updating " + type);
        })
        .onEnd([]() {
          Serial.println("\nEnd");
        })
        .onProgress([](unsigned int progress, unsigned int total) {
          Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
          Serial.printf("Error[%u]: ", error);
          if (error == OTA_AUTH_ERROR)
            Serial.println("Auth Failed");
          else if (error == OTA_BEGIN_ERROR)
            Serial.println("Begin Failed");
          else if (error == OTA_CONNECT_ERROR)
            Serial.println("Connect Failed");
          else if (error == OTA_RECEIVE_ERROR)
            Serial.println("Receive Failed");
          else if (error == OTA_END_ERROR)
            Serial.println("End Failed");
        });

    ArduinoOTA.begin(); //******************** OTA ENDS

    elapsedMillis since_started_ota_listening;

    DEBUG("---------------------------------------");
    DEBUG("      START OTA UPLOAD! (30 secs)      ");
    DEBUG("---------------------------------------");

    while (since_started_ota_listening < TIME_WAITING_FOR_OTA)
    {
      ArduinoOTA.handle();

      delay(10);
    }

    DEBUG("OTA mode finished");
  } // end if otaButton()

  vTaskDelay(10);
}