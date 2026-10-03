// VIRAL SCIENCE
// POWER-LOSS SAFE - DS3231 + EEPROM + RELAY
//
// Relay changes every 2 hours.
// DS3231 = real clock
// EEPROM = persistent schedule
//
// Compatible with the currently installed DS3231 library.

#include <Wire.h>
#include <DS3231.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// ==================================================
// LCD
// ==================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ==================================================
// DS3231
// ==================================================

DS3231 rtc(Wire);

// ==================================================
// RELAY
// ==================================================

const int Relay = 4;

// ==================================================
// EEPROM
// ==================================================

const int EEPROM_MAGIC_ADDRESS = 0;
const byte EEPROM_MAGIC = 0x5A;

const int EEPROM_START_HOUR = 1;
const int EEPROM_START_MINUTE = 2;
const int EEPROM_INITIAL_STATE = 3;

// ==================================================
// SCHEDULE
// ==================================================

const int INTERVAL_MINUTES = 120;

int startHour;
int startMinute;

bool initialRelayState;
bool isOn;


// ==================================================
// Convert HH:MM to minutes
// ==================================================

int timeToMinutes(int hour, int minute) {

  return (hour * 60) + minute;
}


// ==================================================
// Normalize minutes
// ==================================================

int normalizeMinutes(int minutes) {

  minutes %= 1440;

  if (minutes < 0) {
    minutes += 1440;
  }

  return minutes;
}


// ==================================================
// Get hour
// ==================================================

int getHourFromMinutes(int totalMinutes) {

  totalMinutes = normalizeMinutes(totalMinutes);

  return totalMinutes / 60;
}


// ==================================================
// Get minute
// ==================================================

int getMinuteFromMinutes(int totalMinutes) {

  totalMinutes = normalizeMinutes(totalMinutes);

  return totalMinutes % 60;
}


// ==================================================
// Save schedule
// ==================================================

void saveSchedule() {

  EEPROM.update(
    EEPROM_MAGIC_ADDRESS,
    EEPROM_MAGIC
  );

  EEPROM.update(
    EEPROM_START_HOUR,
    startHour
  );

  EEPROM.update(
    EEPROM_START_MINUTE,
    startMinute
  );

  EEPROM.update(
    EEPROM_INITIAL_STATE,
    initialRelayState ? 1 : 0
  );

  Serial.println("Schedule saved to EEPROM");
}


// ==================================================
// Load schedule
// ==================================================

bool loadSchedule() {

  if (EEPROM.read(EEPROM_MAGIC_ADDRESS)
      != EEPROM_MAGIC) {

    return false;
  }

  startHour =
      EEPROM.read(EEPROM_START_HOUR);

  startMinute =
      EEPROM.read(EEPROM_START_MINUTE);

  initialRelayState =
      EEPROM.read(EEPROM_INITIAL_STATE) == 1;

  if (startHour > 23) {
    return false;
  }

  if (startMinute > 59) {
    return false;
  }

  return true;
}


// ==================================================
// Get RTC time
// ==================================================

void getCurrentTime(
  int &hour,
  int &minute,
  int &second
) {

  bool h12;
  bool PM;

  hour = rtc.getHour(h12, PM);
  minute = rtc.getMinute();
  second = rtc.getSecond();
}


// ==================================================
// Calculate elapsed minutes from schedule start
// ==================================================

int getElapsedMinutes(
  int currentHour,
  int currentMinute
) {

  int startTotal =
      timeToMinutes(
        startHour,
        startMinute
      );

  int currentTotal =
      timeToMinutes(
        currentHour,
        currentMinute
      );

  int elapsed =
      currentTotal - startTotal;

  if (elapsed < 0) {
    elapsed += 1440;
  }

  return elapsed;
}


// ==================================================
// Calculate current relay state
// ==================================================

bool calculateRelayState(
  int currentHour,
  int currentMinute
) {

  int elapsed =
      getElapsedMinutes(
        currentHour,
        currentMinute
      );

  int intervals =
      elapsed / INTERVAL_MINUTES;

  if (intervals % 2 == 0) {
    return initialRelayState;
  }

  return !initialRelayState;
}


// ==================================================
// Calculate NEXT switching time
// ==================================================

void getNextSwitchTime(
  int currentHour,
  int currentMinute,
  int currentSecond,
  int &nextHour,
  int &nextMinute
) {

  int startTotal =
      timeToMinutes(
        startHour,
        startMinute
      );

  int currentTotal =
      timeToMinutes(
        currentHour,
        currentMinute
      );

  int elapsed =
      currentTotal - startTotal;

  if (elapsed < 0) {
    elapsed += 1440;
  }

  // Find the next 2-hour boundary
  int intervals =
      (elapsed / INTERVAL_MINUTES) + 1;

  int nextTotal =
      startTotal +
      (intervals * INTERVAL_MINUTES);

  nextTotal =
      normalizeMinutes(nextTotal);

  nextHour =
      getHourFromMinutes(nextTotal);

  nextMinute =
      getMinuteFromMinutes(nextTotal);
}


// ==================================================
// Set relay
// ==================================================

void setRelay(bool state) {

  isOn = state;

  if (isOn) {
    digitalWrite(Relay, HIGH);
  }
  else {
    digitalWrite(Relay, LOW);
  }
}


// ==================================================
// SETUP
// ==================================================

void setup() {

  Wire.begin();

  Serial.begin(115200);

  // ------------------------------------------------
  // LCD
  // ------------------------------------------------

  lcd.init();
  lcd.backlight();

  // ------------------------------------------------
  // Relay
  // ------------------------------------------------

  pinMode(Relay, OUTPUT);

  // ------------------------------------------------
  // Read RTC
  // ------------------------------------------------

  int currentHour;
  int currentMinute;
  int currentSecond;

  getCurrentTime(
    currentHour,
    currentMinute,
    currentSecond
  );

  Serial.println();
  Serial.println("==============================");
  Serial.println("SYSTEM START");
  Serial.println("==============================");

  Serial.print("RTC: ");

  if (currentHour < 10) {
    Serial.print("0");
  }

  Serial.print(currentHour);
  Serial.print(":");

  if (currentMinute < 10) {
    Serial.print("0");
  }

  Serial.print(currentMinute);
  Serial.print(":");

  if (currentSecond < 10) {
    Serial.print("0");
  }

  Serial.println(currentSecond);


  // ------------------------------------------------
  // Load previous schedule
  // ------------------------------------------------

  bool scheduleExists =
      loadSchedule();


  // ------------------------------------------------
  // First ever startup
  // ------------------------------------------------

  if (!scheduleExists) {

    Serial.println(
      "No previous schedule found."
    );

    Serial.println(
      "Creating new schedule."
    );

    startHour = currentHour;
    startMinute = currentMinute;

    initialRelayState = true;

    saveSchedule();
  }

  else {

    Serial.println(
      "Previous schedule restored."
    );
  }


  // ------------------------------------------------
  // Calculate correct relay state
  // ------------------------------------------------

  bool requiredState =
      calculateRelayState(
        currentHour,
        currentMinute
      );

  setRelay(requiredState);


  Serial.print("Schedule start: ");

  if (startHour < 10) {
    Serial.print("0");
  }

  Serial.print(startHour);
  Serial.print(":");

  if (startMinute < 10) {
    Serial.print("0");
  }

  Serial.println(startMinute);


  Serial.print("Relay: ");

  if (isOn) {
    Serial.println("ON");
  }
  else {
    Serial.println("OFF");
  }

  delay(1000);
}


// ==================================================
// LOOP
// ==================================================

void loop() {

  int currentHour;
  int currentMinute;
  int currentSecond;

  // ------------------------------------------------
  // Read RTC
  // ------------------------------------------------

  getCurrentTime(
    currentHour,
    currentMinute,
    currentSecond
  );


  // ------------------------------------------------
  // Calculate correct relay state
  // ------------------------------------------------

  bool requiredState =
      calculateRelayState(
        currentHour,
        currentMinute
      );


  // ------------------------------------------------
  // Update relay
  // ------------------------------------------------

  if (requiredState != isOn) {

    setRelay(requiredState);

    Serial.println();

    if (isOn) {
      Serial.println("RELAY -> ON");
    }
    else {
      Serial.println("RELAY -> OFF");
    }
  }


  // ------------------------------------------------
  // Calculate next switching time
  // ------------------------------------------------

  int nextHour;
  int nextMinute;

  getNextSwitchTime(
    currentHour,
    currentMinute,
    currentSecond,
    nextHour,
    nextMinute
  );


  // ------------------------------------------------
  // 12-hour conversion
  // ------------------------------------------------

  int displayHour = nextHour;

  if (displayHour == 0) {

    // Midnight
    displayHour = 12;

  }
  else if (displayHour > 12) {

    displayHour -= 12;
  }


  // ------------------------------------------------
  // Format hour
  // ------------------------------------------------

  String displayHourString;

  if (displayHour < 10) {
    displayHourString =
        "0" + String(displayHour);
  }
  else {
    displayHourString =
        String(displayHour);
  }


  // ------------------------------------------------
  // Format minute
  // ------------------------------------------------

  String displayMinuteString;

  if (nextMinute < 10) {
    displayMinuteString =
        "0" + String(nextMinute);
  }
  else {
    displayMinuteString =
        String(nextMinute);
  }


  // ------------------------------------------------
  // AM / PM
  // ------------------------------------------------

  String manipulation;

  if (nextHour >= 12) {
    manipulation = "PM";
  }
  else {
    manipulation = "AM";
  }


  // ------------------------------------------------
  // Serial
  // ------------------------------------------------

  Serial.print("Current: ");

  if (currentHour < 10) {
    Serial.print("0");
  }

  Serial.print(currentHour);
  Serial.print(":");

  if (currentMinute < 10) {
    Serial.print("0");
  }

  Serial.print(currentMinute);
  Serial.print(":");

  if (currentSecond < 10) {
    Serial.print("0");
  }

  Serial.println(currentSecond);


  Serial.print("Next change: ");
  Serial.print(displayHourString);
  Serial.print(":");
  Serial.print(displayMinuteString);
  Serial.print(" ");
  Serial.println(manipulation);


  // ------------------------------------------------
  // LCD
  // ------------------------------------------------

  lcd.clear();

  if (isOn) {

    lcd.setCursor(0, 0);
    lcd.print("Light will be");

    lcd.setCursor(0, 1);
    lcd.print("off at ");
    lcd.print(displayHourString);
    lcd.print(":");
    lcd.print(displayMinuteString);
    lcd.print(" ");
    lcd.print(manipulation);

  }
  else {

    lcd.setCursor(0, 0);
    lcd.print("Light on at");

    lcd.setCursor(0, 1);
    lcd.print(displayHourString);
    lcd.print(":");
    lcd.print(displayMinuteString);
    lcd.print(" ");
    lcd.print(manipulation);
  }


  delay(1000);
}