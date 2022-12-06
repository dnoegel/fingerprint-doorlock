
/*
 * WIFI
 */
void ensureWifiConnected()
{
  if ((WiFi.status() != WL_CONNECTED) ) {
    WiFi.disconnect();
    delay(100);
    WiFi.setHostname(HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to "); Serial.println(WIFI_SSID);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    ip = itoa(WiFi.localIP(), charBuf, 15);
  }


}

/*
 * MQTT
 */
void setupMqtt()
{
  client.setServer(MqttBroker, MqttPort);
  //client.setCallback(callback);

  if (!client.connected()) {
    mqttReconnect();
  }


}


void mqttReconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    ensureWifiConnected();

    Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    ip.toCharArray(charBuf, 15);
    if (client.connect(charBuf, "", "", "mqtt.0.fingerprint.status", 1, true, "offline")) {
      Serial.println("connected");
      // Subscribe
      client.publish("mqtt.0.fingerprint.status", "online");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loopMqtt() {
  // ensure MQTT connection
  if (!client.connected()) {
    mqttReconnect();
  }
  client.loop();
}

/*
 * Web Updater
 */
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


void setupWebUpdater(void) {

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPasswordHash(OTA_HASH);

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
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
}


void loopWebUpdater() {
  ArduinoOTA.handle();
}
