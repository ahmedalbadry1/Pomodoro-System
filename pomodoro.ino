// ============================================
//  Pomodoro Timer System - Arduino UNO
//  RoboTech Summer Training '25
// ============================================
//
//  Components:
//    - Arduino UNO
//    - LCD 16x2 with I2C module (address 0x27)
//    - Push Button (to start/reset)
//    - Buzzer (active buzzer)
//    - Green LED (Work phase)
//    - Red LED (Break phase)
//
//  Wiring:
//    LCD I2C:
//      SDA -> A4
//      SCL -> A5
//      VCC -> 5V
//      GND -> GND
//
//    Push Button:
//      One leg  -> Pin 2
//      Other leg -> GND
//      (using INPUT_PULLUP, no external resistor needed)
//
//    Buzzer:
//      (+) -> Pin 8
//      (-) -> GND
//
//    Green LED (Work):
//      Anode (+)   -> Pin 5 (through 220 ohm resistor)
//      Cathode (-) -> GND
//
//    Red LED (Break):
//      Anode (+)   -> Pin 6 (through 220 ohm resistor)
//      Cathode (-) -> GND
// ============================================

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---- Pin Definitions ----
#define BUTTON_PIN   2    // Push button pin
#define BUZZER_PIN   8    // Buzzer pin
#define GREEN_LED    5    // Green LED = Work phase
#define RED_LED      6    // Red LED   = Break phase

// ---- Timer Settings (in seconds) ----
#define WORK_TIME    25   // 25 seconds for Work session
#define BREAK_TIME   5    // 5 seconds for Break session
#define NUM_SESSIONS 4    // Number of work/break cycles

// ---- LCD Setup (I2C address 0x27, 16 columns, 2 rows) ----
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---- State Variables ----
bool timerRunning = false;    // Is the timer currently running?
bool isWorkPhase  = true;     // true = Work, false = Break
int  timeLeft     = WORK_TIME;// Countdown in seconds
int  sessionCount = 0;        // Current session number
unsigned long lastMillis = 0; // For tracking 1-second intervals

// ---- Button Debounce ----
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
#define DEBOUNCE_DELAY 50

// =====================
//  SETUP
// =====================
void setup() {
  // Initialize pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Button with internal pull-up
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(GREEN_LED,  OUTPUT);
  pinMode(RED_LED,    OUTPUT);

  // Turn off buzzer and LEDs initially
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(GREEN_LED,  LOW);
  digitalWrite(RED_LED,    LOW);

  // Initialize LCD
  lcd.init();
  lcd.backlight();

  // Show welcome message
  lcd.setCursor(0, 0);
  lcd.print("  Pomodoro Timer");
  lcd.setCursor(0, 1);
  lcd.print(" Press to Start!");

  // Initialize Serial for debugging
  Serial.begin(9600);
  Serial.println("Pomodoro System Ready!");
}

// =====================
//  LOOP
// =====================
void loop() {
  handleButton();

  if (timerRunning) {
    runTimer();
  }
}

// =====================
//  Handle Button Press
// =====================
void handleButton() {
  bool reading = digitalRead(BUTTON_PIN);

  // Debounce logic
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    // Button pressed (LOW because INPUT_PULLUP)
    if (reading == LOW && lastButtonState == HIGH) {
      if (!timerRunning) {
        // Start the timer
        startTimer();
      } else {
        // Reset the timer
        resetTimer();
      }
    }
  }

  lastButtonState = reading;
}

// =====================
//  Start Timer
// =====================
void startTimer() {
  timerRunning  = true;
  isWorkPhase   = true;
  timeLeft      = WORK_TIME;
  sessionCount  = 1;
  lastMillis    = millis();

  // Turn on Green LED for Work phase
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED,   LOW);

  // Update LCD
  updateDisplay();

  Serial.println("Timer Started - Work Phase!");
}

// =====================
//  Reset Timer
// =====================
void resetTimer() {
  timerRunning = false;
  isWorkPhase  = true;
  timeLeft     = WORK_TIME;
  sessionCount = 0;

  // Turn off LEDs and buzzer
  digitalWrite(GREEN_LED,  LOW);
  digitalWrite(RED_LED,    LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Show welcome screen again
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Pomodoro Timer");
  lcd.setCursor(0, 1);
  lcd.print(" Press to Start!");

  Serial.println("Timer Reset!");
}

// =====================
//  Run Timer (called every loop)
// =====================
void runTimer() {
  unsigned long currentMillis = millis();

  // Check if 1 second has passed
  if (currentMillis - lastMillis >= 1000) {
    lastMillis = currentMillis;
    timeLeft--;

    // Update the display every second
    updateDisplay();

    // Print to Serial for debugging
    Serial.print(isWorkPhase ? "WORK" : "BREAK");
    Serial.print(" | Session: ");
    Serial.print(sessionCount);
    Serial.print("/");
    Serial.print(NUM_SESSIONS);
    Serial.print(" | Time: ");
    Serial.println(timeLeft);

    // Time is up!
    if (timeLeft <= 0) {
      soundAlarm();

      if (isWorkPhase) {
        // Work phase ended -> switch to Break
        isWorkPhase = false;
        timeLeft    = BREAK_TIME;

        // Switch LEDs
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(RED_LED,   HIGH);

        Serial.println(">> Break Time!");
      } else {
        // Break phase ended -> check if more sessions
        sessionCount++;

        if (sessionCount > NUM_SESSIONS) {
          // All sessions completed!
          allDone();
          return;
        }

        // Start next Work phase
        isWorkPhase = true;
        timeLeft    = WORK_TIME;

        // Switch LEDs
        digitalWrite(GREEN_LED, HIGH);
        digitalWrite(RED_LED,   LOW);

        Serial.println(">> Work Time!");
      }

      updateDisplay();
    }
  }
}

// =====================
//  Update LCD Display
// =====================
void updateDisplay() {
  lcd.clear();

  // Row 0: Phase and session count
  lcd.setCursor(0, 0);
  if (isWorkPhase) {
    lcd.print("WORK  Session:");
  } else {
    lcd.print("BREAK Session:");
  }
  lcd.print(sessionCount);
  lcd.print("/");
  lcd.print(NUM_SESSIONS);

  // Row 1: Time remaining (MM:SS format)
  lcd.setCursor(0, 1);
  lcd.print("Time Left: ");

  int minutes = timeLeft / 60;
  int seconds = timeLeft % 60;

  if (minutes < 10) lcd.print("0");
  lcd.print(minutes);
  lcd.print(":");
  if (seconds < 10) lcd.print("0");
  lcd.print(seconds);
}

// =====================
//  Sound the Alarm (Buzzer)
// =====================
void soundAlarm() {
  // Beep 3 times to alert the user
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

// =====================
//  All Sessions Done
// =====================
void allDone() {
  timerRunning = false;

  // Turn off LEDs
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED,   LOW);

  // Celebration alarm - longer beeps
  for (int i = 0; i < 5; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(300);
    digitalWrite(BUZZER_PIN, LOW);
    delay(150);
  }

  // Display completion message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  All Sessions");
  lcd.setCursor(0, 1);
  lcd.print("  Completed! :)");

  Serial.println("=== ALL SESSIONS COMPLETED! ===");
}
