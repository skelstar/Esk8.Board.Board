#ifdef FEATURE_OPTIONAL_OTA
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <wifi_secrets.h>

#endif
//------------------------------------------------------

void otaInit()
{
  if (board_packet.moving)
  {
    // cannot allow board to go into OTA if board moving
    return;
  }
  DEBUG("FEATURE_OPTIONAL_OTA");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.printf("\nConnected to %s\n", ssid);
  Serial.printf("IP address: \n");
  WiFi.setHostname("Esk8BoardOTA");
  Serial.printf("Host: 'Esk8BoardOTA'\n");
  Serial.println(WiFi.localIP());

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

  ArduinoOTA.begin();

  sendToFootLightEventQueue(QUEUE_EV_OTA_MODE);
}
//------------------------------------------------------

elapsedMillis sinceFlashedBuiltinLed;
int builtInLedState = HIGH;

void otaLoop()
{
  if (sinceFlashedBuiltinLed > 500)
  {
    sinceFlashedBuiltinLed = 0;

    digitalWrite(22, builtInLedState); // turn the LED on (HIGH is the voltage level)
    builtInLedState = builtInLedState > 0 ? LOW : HIGH;
  }

#ifdef FEATURE_OPTIONAL_OTA
  ArduinoOTA.handle();
#endif
}
//------------------------------------------------------
