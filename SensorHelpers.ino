void setupFingerPrintSensor()
{
  while (!Serial);  delay(100);

  // finger.begin(57600); caused core panic, probably the wrong
  // serial ports where choosen. Calling serial begin manually
  // fixed it in my case
  mySerial.begin(57600, SERIAL_8N1, 17, 16);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (true);
  }

  Serial.print("Setting password... ");
  uint8_t p = finger.setPassword(SENSOR_PASSWORD);
  if (p == FINGERPRINT_OK) {
    Serial.println("Password set"); // Password is set
  } else {
    Serial.println("error setting password"); // Failed to set password
    while(true);
  }

  finger.getTemplateCount();
  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint Going to run 'enroll.");

    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 100, FINGERPRINT_LED_RED, 50);
    delay(5000);
    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 0);
    registerNewFingerprint();
  }
  else {
    Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  }
}

uint8_t getFingerImage()
{
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Capturing finger");
      break;
    case FINGERPRINT_NOFINGER:
      //Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
}


uint8_t getFeatureTemplate(int slot)
{

  uint8_t p = finger.image2Tz(slot);


  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Created feature template");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      finger.LEDcontrol(FINGERPRINT_LED_BREATHING, 100, FINGERPRINT_LED_PURPLE, 3);

      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
}


uint8_t searchFinger()
{
  uint8_t p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a match!");

  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_RED, 3);


    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }
}

uint8_t createModel()
{
  uint8_t s = finger.createModel();
  if (s == FINGERPRINT_OK) {
    Serial.println("Fingerprints matched!");
  } else if (s == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return s;
  } else if (s == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    Serial.println("Starting over");
    registerNewFingerprint() ;
    return s;
  } else {
    Serial.println("Unknown error");
    return s;
  }
}

uint8_t storeModel(int id)
{
  uint8_t s = finger.storeModel(id);
  if (s == FINGERPRINT_OK) {
    return s; ////
  } else if (s == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return s;
  } else if (s == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return s;
  } else if (s == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return s;
  } else {
    Serial.println("Unknown error");
    return s;
  }
}

void waitForFingerToBeRemovedAndPlacedAgain()
{
  Serial.println("Remove finger");
  finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_BLUE, 0);


  int s = 0;
  while (s != FINGERPRINT_NOFINGER) {
    s = getFingerImage();
  }
  Serial.print("ID "); Serial.println(id);

  s = -1;
  Serial.println("Place same finger again");
  finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_PURPLE, 0);
  while (s != FINGERPRINT_OK) {
    s = getFingerImage();
    if (s == FINGERPRINT_OK) {
      finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 25, FINGERPRINT_LED_PURPLE, 1);
    }
  }
}


void deleteAllFingerprints() {
  finger.emptyDatabase();
  Serial.println("All fingerprints deleted");
  finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 5, FINGERPRINT_LED_RED, 0);
  ESP.restart();
}
