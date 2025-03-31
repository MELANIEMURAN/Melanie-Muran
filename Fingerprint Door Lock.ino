#include <U8g2lib.h>
//U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/U8X8_PIN_NONE);

#include <Keypad.h>

const byte ROWS = 4;  //four rows
const byte COLS = 4;  //four columns
//define the cymbols on the buttons of the keypads

char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

byte colPins[COLS] = { 7, 6, 5, 4 };    //connect to the column pinouts of the keypad
byte rowPins[ROWS] = { 11, 10, 9, 8 };  //connect to the row pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

#include <Adafruit_Fingerprint.h>
// pin #2(RX) is IN from sensor (BLACK wire)
// pin #3(TX) is OUT from arduino (YELLOW wire)
SoftwareSerial mySerial(2, 3);  //RX ,TX

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

const int LED_GN = A0;
const int LED_RD = A1;
const int RELAY = 12;
const int BUZZER = 13;

bool passwdOk = false;
bool passwdOn = false;
bool enrollOn = false;
bool on = false;
char key;

void setup() {

  randomSeed(analogRead(0));

  pinMode(RELAY, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_GN, OUTPUT);
  pinMode(LED_RD, OUTPUT);

  digitalWrite(RELAY, HIGH);

  Serial.begin(9600);
  finger.begin(57600);

  u8x8.begin();
  u8x8.setContrast(50);
  u8x8.setFlipMode(2);
  u8x8.setFont(u8x8_font_torussansbold8_r);
  while (!finger.verifyPassword()) {
    Serial.println(F("Did not find fingerprint sensor"));
    u8x8.setCursor(0, 0);
    u8x8.print(F("sensor not found"));
    delay(2000);
  }
  readingParamters();
}

void loop() {

  key = keypad.getKey();

  passwd();
  enroll();
  search();
}

void passwd() {

  if (passwdOk) return;

  static unsigned long elapse = millis();
  if (!passwdOn | key)elapse = millis();
  if (millis() - elapse >= 10000)key = '#';
  else if (passwdOn) {
    u8x8.setCursor(7, 6);
    u8x8.print(u8x8_u16toa(abs((10000 - millis() + elapse) / 1000), 2));
  }

  const char password[] = "03142025";

  static char passw[17] = "";
  static byte index = 0;
  byte size = sizeof(passw);

  if (key) {
    if (passwdOn) {
      switch (key) {

        default:
          if (index < size - 1) {
            passw[index] = key;
            index++;
          }
          break;

        case '*':
          if (index > 0) {
            index--;
            passw[index] = 0;
          }
          break;

        case '#':
          if (!strcmp(passw, password)) passwdOk = true;
          u8x8.setCursor(0, 6);
          if (passwdOk) u8x8.print(F("password correct"));
          else u8x8.print(F(" password wrong "));
          index = 0;
          memset(passw, 0, size);
          delay(1000);
          u8x8.clearDisplay();
          passwdOn = false;
          return;
      }
    }
    passwdOn = true;

    u8x8.setCursor(0, 0);
    u8x8.print(F("      login     "));
    u8x8.setCursor(0, 1);
    u8x8.print(F("----------------"));
    u8x8.setCursor(0, 2);
    u8x8.print(F(" enter password "));
    u8x8.clearLine(3);
    u8x8.setCursor(0, 4);
    u8x8.print(F("****************"));
    u8x8.setCursor(0, 4);
    u8x8.print(passw);
    u8x8.clearLine(5);
    u8x8.clearLine(6);
    u8x8.setCursor(0, 7);
    u8x8.print(F("----------------"));
  }
}

void enroll() {

  if (!passwdOk) return;

  static uint8_t id = 0;
  static unsigned long code = 0;
  static unsigned long rand = random(10000000, 99999999);

  int num;

  if (key) {
    if (enrollOn) {
      switch (key) {

        case '0' ... '9':

          num = key - 48;

          if (!code) {
            if (code + num <= 99999999) code = code + num;
          } else {
            if (10 * code + num <= 99999999) code = 10 * code + num;
          }
          break;

        case '*':
          code = code / 10;
          break;

        case 'A':
          u8x8.clearDisplay();
          if (code >= 0 & code <= 255) {
            id = code;
            getFingerprintEnroll(id);
          } else {
            u8x8.setCursor(0, 0);
            u8x8.print(F("only 0 - 255"));
          }
          delay(3000);
          break;

        case 'B':
          u8x8.clearDisplay();
          if (code >= 0 & code <= 255) {
            id = code;
            deleteFingerprint(id);
          } else {
            u8x8.setCursor(0, 0);
            u8x8.print(F("only 0 - 255"));
          }
          delay(3000);
          break;

        case 'C':
          u8x8.clearDisplay();
          u8x8.setCursor(0, 0);
          if (code == rand) {
            finger.emptyDatabase();
            rand = random(10000000, 99999999);
            u8x8.print(F("-database empty-"));
          } else {
            u8x8.print(F("only "));
            u8x8.print(rand);
          }
          delay(3000);
          break;

        case 'D':
          on = !on;
          break;

        case '#':
          u8x8.clearDisplay();
          passwdOk = false;
          enrollOn = false;
          on = false;
          return;
      }
    }
    enrollOn = true;

    u8x8.clearLine(0);
    u8x8.setCursor(0, 0);
    u8x8.print(code);
    u8x8.setCursor(0, 1);
    u8x8.print(F("----------------"));
    u8x8.setCursor(0, 2);
    u8x8.print(F("A: enroll finger"));
    u8x8.setCursor(0, 3);
    u8x8.print(F("B: delete finger"));
    u8x8.setCursor(0, 4);
    u8x8.print(F("C: ClearDatabase"));
    u8x8.setCursor(0, 5);
    u8x8.print(F("D: <open><close>"));
    u8x8.setCursor(0, 6);
    u8x8.print(F("#:    logout    "));
    u8x8.setCursor(0, 7);
    u8x8.print(F("----------------"));
  }
}

void search() {

  const unsigned long ONTIME = 5000;
  const unsigned long PERIOD = 9000;
  const unsigned long REPEAT = 2000;

  unsigned long timer = millis();
  static unsigned long period = timer - PERIOD;
  static unsigned long repeat = timer - REPEAT;
  int p;

  if (!enrollOn) {
    if (timer - period > PERIOD) {
      if (timer - repeat > REPEAT) {

        if (passwdOn) p = getFingerprintIDez();
        else p = getFingerprintID();

        timer = millis();
        repeat = timer;
        switch (p) {
          default:
            break;
          case 0 ... 255:
            period = timer;
            digitalWrite(RELAY, LOW);
            digitalWrite(BUZZER, HIGH);
            digitalWrite(LED_GN, HIGH);
            digitalWrite(LED_RD, LOW);
            if (!passwdOn) {
              u8x8.setCursor(0, 6);
              u8x8.print(F("Access Granted  "));
              u8x8.setCursor(0, 7);
              u8x8.print(F("Door Unlocked   "));
            }
            break;
        }
      }
    }
    if (timer - period > 1000) {
      digitalWrite(BUZZER, LOW);
      if (timer - period > ONTIME) {
        digitalWrite(RELAY, HIGH);
        digitalWrite(LED_RD, HIGH);
        if (timer - period < PERIOD) {
          digitalWrite(LED_GN, LOW);
          if (!passwdOn) {
            u8x8.setCursor(0, 6);
            u8x8.print(F("Door Locked     "));
            u8x8.setCursor(0, 7);
            u8x8.print(F("System Secure   "));
          }
        } else {
          digitalWrite(LED_GN, bitRead(timer, 10));
        }
      }
    }
    return;
  }

  period = timer - PERIOD;

  static unsigned long elapse = timer;

  if (timer - elapse > ONTIME) on = false;

  if (on) {
    digitalWrite(RELAY, LOW);
    digitalWrite(BUZZER, HIGH);
    digitalWrite(LED_RD, LOW);
    digitalWrite(LED_GN, HIGH);
  } else {
    digitalWrite(RELAY, HIGH);
    digitalWrite(BUZZER, LOW);
    digitalWrite(LED_RD, HIGH);
    digitalWrite(LED_GN, LOW);
    elapse = timer;
  }
}

void readingParamters() {

  u8x8.clearDisplay();
  u8x8.setCursor(0, 0);
  u8x8.print(F("ReadingParameter"));
  finger.getParameters();
  u8x8.setCursor(0, 1);
  u8x8.print(F("Status: 0x"));
  u8x8.print(finger.status_reg, HEX);
  u8x8.setCursor(0, 2);
  u8x8.print(F("Sys ID: 0x"));
  u8x8.print(finger.system_id, HEX);
  u8x8.setCursor(0, 3);
  u8x8.print(F("Capacity: "));
  u8x8.print(finger.capacity);
  u8x8.setCursor(0, 4);
  u8x8.print(F("Securitylevel: "));
  u8x8.print(finger.security_level);
  u8x8.setCursor(0, 5);
  u8x8.print(F("address:"));
  u8x8.print(finger.device_addr, HEX);
  u8x8.setCursor(0, 6);
  u8x8.print(F("Packet len: "));
  u8x8.print(finger.packet_len);
  u8x8.setCursor(0, 7);
  u8x8.print(F("Baudrate: "));
  u8x8.print(finger.baud_rate);
  delay(3000);
  u8x8.clearDisplay();
}

uint8_t getFingerprintEnroll(uint8_t id) {

  int i;
  int p;
  int countdown;
  static unsigned long timeout;

  u8x8.clearDisplay();

  u8x8.setCursor(0, 0);
  u8x8.print(F("Enrolling ID "));
  u8x8.print(id);

  for (i = 1; i <= 2; i++) {
    u8x8.setCursor(0, 2);
    switch (i) {
      case 1:
        u8x8.print(F("Waiting f.finger"));
        break;
      case 2:
        u8x8.print(F("Put same finger"));
        break;
    }

    timeout = millis() + 60000;
    do {
      p = finger.getImage();
      u8x8.setCursor(0, 3);
      switch (p) {
        case FINGERPRINT_OK:
          u8x8.print(F("Image taken     "));
          break;
        case FINGERPRINT_NOFINGER:
          u8x8.print(F("...             "));
          break;
        case FINGERPRINT_PACKETRECIEVEERR:
          u8x8.print(F("Communicat.error"));
          break;
        case FINGERPRINT_IMAGEFAIL:
          u8x8.print(F("Imaging error   "));
          break;
        default:
          u8x8.print(F("Unknown error   "));
          break;
      }
      countdown = (timeout - millis()) / 1000;
      if (countdown < 0) return p;
      u8x8.setCursor(7, 7);
      u8x8.print(u8x8_u16toa(countdown, 2));
    } while (p != FINGERPRINT_OK);

    u8x8.clearLine(7);

    // OK success!

    p = finger.image2Tz(i);
    u8x8.setCursor(0, 4);
    switch (p) {
      case FINGERPRINT_OK:
        u8x8.print(F("Image converted"));
        break;
      case FINGERPRINT_IMAGEMESS:
        u8x8.print(F("Image too messy"));
        return p;
      case FINGERPRINT_PACKETRECIEVEERR:
        u8x8.print(F("Communicat.error"));
        return p;
      case FINGERPRINT_FEATUREFAIL:
        u8x8.print(F("Features fail"));
        return p;
      case FINGERPRINT_INVALIDIMAGE:
        u8x8.print(F("Invalid Image"));
        return p;
      default:
        u8x8.print(F("Unknown error"));
        return p;
    }

    // OK converted!
    u8x8.setCursor(0, 5);
    u8x8.print(F("Remove finger"));
    delay(2000);

    timeout = millis() + 60000;
    do {
      p = finger.getImage();
      if (p == FINGERPRINT_NOFINGER) break;
      countdown = (timeout - millis()) / 1000;
      if (countdown < 0) return p;
      u8x8.setCursor(7, 7);
      u8x8.print(u8x8_u16toa(countdown, 2));
    } while (1);

    u8x8.clearLine(2);
    u8x8.clearLine(3);
    u8x8.clearLine(4);
    u8x8.clearLine(5);
    u8x8.clearLine(6);
    u8x8.clearLine(7);
  }

  u8x8.setCursor(0, 2);
  u8x8.print(F("Creating model!"));

  p = finger.createModel();
  u8x8.setCursor(0, 3);
  if (p == FINGERPRINT_OK) {
    u8x8.print(F("Prints matched!"));
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    u8x8.print(F("Communicat.error"));
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    u8x8.print(F("Prints not match"));
    return p;
  } else {
    u8x8.print(F("Unknown error"));
    return p;
  }

  p = finger.storeModel(id);
  u8x8.setCursor(0, 4);
  if (p == FINGERPRINT_OK) {
    u8x8.print(F("Stored!"));
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    u8x8.print(F("Communicat.error"));
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    u8x8.print(F("Could not store"));
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    u8x8.print(F("FlashWrite error"));
    return p;
  } else {
    u8x8.print(F("Unknown error"));
    return p;
  }
  return true;
}

uint8_t deleteFingerprint(uint8_t id) {
  uint8_t p = -1;

  u8x8.clearDisplay();

  p = finger.deleteModel(id);
  u8x8.setCursor(0, 0);
  if (p == FINGERPRINT_OK) {
    u8x8.print(F("Deleted ID "));
    u8x8.print(id);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    u8x8.print(F("Communicat.error"));
  } else if (p == FINGERPRINT_BADLOCATION) {
    u8x8.print(F("Could not delete"));
  } else if (p == FINGERPRINT_FLASHERR) {
    u8x8.print(F("FlashWrite error"));
  } else {
    u8x8.print(F("Unknown error"));
    u8x8.setCursor(0, 1);
    u8x8.print(F("Errorcode: 0x"));
    u8x8.print(p, HEX);
  }
  return p;
}

int getFingerprintID() {

  uint8_t p;

  u8x8.clearDisplay();

  p = finger.getImage();
  u8x8.setCursor(0, 0);
  switch (p) {
    case FINGERPRINT_OK:
      u8x8.print(F("Image taken"));
      break;
    case FINGERPRINT_NOFINGER:
      u8x8.print(F("No finger detect"));
      return -1;
    case FINGERPRINT_PACKETRECIEVEERR:
      u8x8.print(F("Communicat.error"));
      return -1;
    case FINGERPRINT_IMAGEFAIL:
      u8x8.print(F("Imaging error"));
      return -1;
    default:
      u8x8.print(F("Unknown error"));
      return -1;
  }

  // OK success!

  p = finger.image2Tz();
  u8x8.setCursor(0, 1);
  switch (p) {
    case FINGERPRINT_OK:
      u8x8.print(F("Image converted"));
      break;
    case FINGERPRINT_IMAGEMESS:
      u8x8.print(F("Image too messy"));
      return -1;
    case FINGERPRINT_PACKETRECIEVEERR:
      u8x8.print(F("Communicat.error"));
      return -1;
    case FINGERPRINT_FEATUREFAIL:
      u8x8.print(F("Features fail"));
      return -1;
    case FINGERPRINT_INVALIDIMAGE:
      u8x8.print(F("Invalid Image"));
      return -1;
    default:
      u8x8.print(F("Unknown error"));
      return -1;
  }

  // OK converted!

  p = finger.fingerSearch();
  u8x8.setCursor(0, 2);
  if (p == FINGERPRINT_OK) {
    u8x8.print(F("Found a match!"));
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    u8x8.print(F("Communicat.error"));
    return -1;
  } else if (p == FINGERPRINT_NOTFOUND) {
    u8x8.print(F("Did not match"));
    return -1;
  } else {
    u8x8.print(F("Unknown error"));
    return -1;
  }

  // found a match!

  u8x8.setCursor(0, 3);
  u8x8.print(F("Found ID #"));
  u8x8.print(finger.fingerID);
  u8x8.setCursor(0, 4);
  u8x8.print(F("Confidence: "));
  u8x8.print(finger.confidence);

  return finger.fingerID;
}

int getFingerprintIDez() {

  uint8_t p;

  p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  // OK success!

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  // OK converted!

  p = finger.fingerSearch();
  if (p != FINGERPRINT_OK) return -1;

  // found a match!

  return finger.fingerID;
}
