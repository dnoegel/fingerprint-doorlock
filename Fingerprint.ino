/*
   Based on the Adafruit Fingrerprint example s
   - https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library/blob/master/examples/fingerprint/fingerprint.ino
   - https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library/blob/master/examples/emptyDatabase/emptyDatabase.ino
   - https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library/blob/master/examples/enroll/enroll.ino

   Shoutout to youtube.com/ModItBetter who made good efforts to combine those examples into one sketch that can be controlled soley via the sensor
*/

#include "Setup.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_Fingerprint.h>

WiFiClient espClient;
PubSubClient client(espClient);
HardwareSerial mySerial(2);
HTTPClient http;

#if(FIRST_BOOT)
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
#else
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial, SENSOR_PASSWORD);
#endif

uint8_t id;
int noFingerSeconds = 0;
int adminFingerSeconds = 0;
String ip;
char charBuf[40];

void setup()
{
  Serial.begin(SERIAL_SPEED);

  if (false) {
    Serial.println("Boot");
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 1);
    Serial.println("Going to sleep now");
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
  }

  ensureWifiConnected();
  setupWebUpdater();
  setupFingerPrintSensor();
  setupMqtt();
}

void loop()
{
  ensureWifiConnected();
  loopMqtt();
  loopWebUpdater();

  if (noFingerSeconds == 3)  {
    if (adminFingerSeconds >= REGISTER_NEW_FINGER_SECONDS && adminFingerSeconds <= (DELETE_ALL_FINGER_SECONDS - 2)) {
      registerNewFingerprint();
    }
    if (adminFingerSeconds >= DELETE_ALL_FINGER_SECONDS ) {
      deleteAllFingerprints();
    }

    noFingerSeconds = 0;
    adminFingerSeconds = 1;
  }

  int p = readFingerPrints();

  // door opening only for non-admin fingers
  if (p > 1) {
    client.publish("mqtt.0.fingerprint.last_seen_finger", itoa(p, charBuf, 10));
    client.publish("mqtt.0.fingerprint.last_seen_confidence", itoa(finger.confidence, charBuf, 10));
    if (finger.confidence > 50) {
      client.publish("mqtt.0.fingerprint.last_open_finger", itoa(p, charBuf, 10));
      openDoor(p);
    }

  }

  delay(1000);
  noFingerSeconds += 1;
  Serial.print("No Finger Seconds: "); Serial.println(noFingerSeconds);
}

void openDoor(int p)
{
  Serial.println("== OPENING DOOR ==");
  Serial.print("Finger ID: "); Serial.println(p);

  http.begin(OPEN_DOOR_API_CALL);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);
  } else {
    Serial.println("Error on HTTP request");
  }

  http.end();

}

uint8_t readFingerPrints() {
  uint8_t p;

  if (p = getFingerImage() != FINGERPRINT_OK) return 0;
  if (p = getFeatureTemplate(1) != FINGERPRINT_OK) return 0;
  if (p = searchFinger() != FINGERPRINT_OK) return 0;

  finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_BLUE, 1);

  handleAdminFinger();



  return finger.fingerID;
}

void handleAdminFinger()
{

  if (finger.fingerID == 1) {
    adminFingerSeconds += 1;
    noFingerSeconds = 0;

    if (adminFingerSeconds == REGISTER_NEW_FINGER_SECONDS) {
      Serial.print("Release finger to trigger new user creation");
      finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED);
      delay (1000);
    }

    if (adminFingerSeconds >= 20) {
      Serial.println("Release finger to trigger deletion of all records");
      finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_PURPLE);
      delay (1000);
    }

    Serial.print("Admin finger seconds: ");
    Serial.println(adminFingerSeconds);

  }
}

void registerNewFingerprint()
{
  finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_PURPLE, 0);
  Serial.println("Registering a new fingerprint");

  finger.getTemplateCount();
  Serial.print("Sensor contains "); Serial.println(finger.templateCount);

  id = (finger.templateCount + 1);

  while (!  doRegisterNewFingerprint() );
}

uint8_t doRegisterNewFingerprint() {

  int s = -1;
  Serial.print("Waiting for finger to be registers as ID #"); Serial.println(id);

  finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_PURPLE, 0);
  while (s != FINGERPRINT_OK) {
    s = getFingerImage();
    if (s == FINGERPRINT_OK) {
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_PURPLE, 1);
      delay(2000);
    }

  }

  s = getFeatureTemplate(1);

  waitForFingerToBeRemovedAndPlacedAgain();

  if (s = getFeatureTemplate(2) != FINGERPRINT_OK) return s;
  if (s = createModel() != FINGERPRINT_OK) return s;
  if (s = storeModel(id) != FINGERPRINT_OK) return s;

  if (s == FINGERPRINT_OK) {
    Serial.println("Stored!");

    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_BLUE, 20);
    finger.LEDcontrol(FINGERPRINT_LED_GRADUAL_ON, 200, FINGERPRINT_LED_BLUE);
    delay(2000);
    finger.LEDcontrol(FINGERPRINT_LED_GRADUAL_OFF, 200, FINGERPRINT_LED_BLUE);
    delay(2000);
    finger.LEDcontrol(FINGERPRINT_LED_GRADUAL_ON, 200, FINGERPRINT_LED_BLUE);
    delay(2000);
    finger.LEDcontrol(FINGERPRINT_LED_GRADUAL_OFF, 200, FINGERPRINT_LED_BLUE);
    delay(2000);
  }


  return true;
}
