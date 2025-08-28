#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <Servo.h>

// Pentru functia de testare
// #define MODE_TEST

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;
Servo servo;

const int servoPin = 3;
const int resetButtonPin = 6;
const int redLedPin = 9;
const int greenLedPin = 8;
const int buzzerPin = 10;

DateTime lastDoseTime;
bool doseDispensed = false;
bool buzzerDone = false;

#ifdef MODE_TEST
const long testIntervalSec = 60; 
#endif

int doseHours[] = {8, 14, 20};

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  if (!rtc.begin()) {
    lcd.print("RTC ERROR");
    while (1);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  servo.attach(servoPin);
  servo.write(0); 

  pinMode(resetButtonPin, INPUT_PULLUP);
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  digitalWrite(greenLedPin, HIGH);
  digitalWrite(redLedPin, LOW);
  digitalWrite(buzzerPin, HIGH);

#ifdef MODE_TEST
  lastDoseTime = rtc.now(); 
#endif
}

void loop() {
  DateTime now = rtc.now();

  lcd.setCursor(0, 0);
  lcd.print("Ora: ");
  printHourMinute(now.hour(), now.minute());
  lcd.print("     ");

  bool timeToDispense = false;

#ifdef MODE_TEST
  long elapsed = now.unixtime() - lastDoseTime.unixtime();
  long remaining = testIntervalSec - elapsed;

  lcd.setCursor(0, 1);
  if (remaining > 0) {
    int h = remaining / 3600;
    int m = (remaining % 3600) / 60;
    int s = remaining % 60;
    lcd.print("Urm: ");
    printTime(h, m, s);
    lcd.print(" ");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("Luati medicatia! ");
    timeToDispense = true;
  }

#else
  DateTime nextDose = getNextDoseTime(now);
  long remaining = nextDose.unixtime() - now.unixtime();

  if (remaining <= 0 && !doseDispensed) {
    lcd.setCursor(0, 1);
    lcd.print("Luati medicatia! ");
    timeToDispense = true;
  } else {
    int h = remaining / 3600;
    int m = (remaining % 3600) / 60;

    lcd.setCursor(0, 1);
    lcd.print("Urm: ");
    if (h < 10) lcd.print("0");
    lcd.print(h);
    lcd.print(":");
    if (m < 10) lcd.print("0");
    lcd.print(m);
    lcd.print("      ");
  }
#endif

  if (timeToDispense && !doseDispensed) {
    dispensePills();
    doseDispensed = true;
    buzzerDone = false;
  }

  if (digitalRead(resetButtonPin) == LOW) {
    delay(100);
    if (digitalRead(resetButtonPin) == LOW) {
#ifdef MODE_TEST
      lastDoseTime = now;
      buzzerDone = false; 
#endif
      digitalWrite(greenLedPin, HIGH);
      digitalWrite(redLedPin, LOW);
      digitalWrite(buzzerPin, HIGH);
      doseDispensed = false;
      lcd.clear();
    }
  }

  delay(200);
}

void dispensePills() {
  for (int pos = 0; pos <= 180; pos++) {
    servo.write(pos);
    delay(10);
  }
  delay(1000);
  for (int pos = 180; pos >= 0; pos--) {
    servo.write(pos);
    delay(10);
  }

  digitalWrite(redLedPin, HIGH);
  digitalWrite(greenLedPin, LOW);

  if (!buzzerDone) {
    for (int i = 0; i < 3; i++) {
      digitalWrite(buzzerPin, LOW);  
      delay(1000);
      digitalWrite(buzzerPin, HIGH); 
      delay(1000);
    }
    buzzerDone = true;
  }
}

DateTime getNextDoseTime(DateTime now) {
  for (int i = 0; i < 3; i++) {
    if (now.hour() < doseHours[i] || (now.hour() == doseHours[i] && now.minute() == 0)) {
      return DateTime(now.year(), now.month(), now.day(), doseHours[i], 0, 0);
    }
  }
  DateTime nextDay = now + TimeSpan(1, 0, 0, 0);
  return DateTime(nextDay.year(), nextDay.month(), nextDay.day(), doseHours[0], 0, 0);
}

void printHourMinute(int h, int m) {
  if (h < 10) lcd.print("0");
  lcd.print(h);
  lcd.print(":");
  if (m < 10) lcd.print("0");
  lcd.print(m);
}

void printTime(int h, int m, int s) {
  if (h < 10) lcd.print("0");
  lcd.print(h);
  lcd.print(":");
  if (m < 10) lcd.print("0");
  lcd.print(m);
  lcd.print(":");
  if (s < 10) lcd.print("0");
  lcd.print(s);
}
