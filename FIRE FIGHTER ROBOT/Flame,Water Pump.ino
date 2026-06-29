#include <Servo.h>

// ===== FLAME SENSOR PINS =====
#define FLAME_LEFT   2
#define FLAME_CENTER 3
#define FLAME_RIGHT  4

// ===== LED + BUZZER =====
#define RED_LED 9
#define BUZZER  8

// ===== RELAY (WATER PUMP - Active HIGH) =====
#define RELAY_PIN 5

// ===== SERVO MOTOR =====
#define SERVO_PIN 6
Servo waterServo;

// ===== VARIABLES =====
int currentAngle = 90;
int targetAngle = 90;
unsigned long pumpStartTime = 0;
unsigned long lastFlameTime = 0;  // To remember when flame last detected
bool pumpActive = false;
bool flameDetected = false;

void setup() {
  pinMode(FLAME_LEFT, INPUT);
  pinMode(FLAME_CENTER, INPUT);
  pinMode(FLAME_RIGHT, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  // Relay OFF initially (Active HIGH)
  digitalWrite(RELAY_PIN, LOW);

  waterServo.attach(SERVO_PIN);
  waterServo.write(currentAngle);

  Serial.begin(9600);
  Serial.println("🔥 Fire Detection System (Holds Position Until Flame Gone) 🔥");
}

void loop() {
  int left = digitalRead(FLAME_LEFT);
  int center = digitalRead(FLAME_CENTER);
  int right = digitalRead(FLAME_RIGHT);

  bool currentFlame = false;

  // ===== Flame Detection Logic =====
  if (center == LOW) {
    currentFlame = true;
    targetAngle = 90;
    Serial.println("🔥 Flame at CENTER!");
  } 
  else if (left == LOW && right == HIGH) {
    currentFlame = true;
    targetAngle = 140; // LEFT
    Serial.println("🔥 Flame at LEFT!");
  } 
  else if (right == LOW && left == HIGH) {
    currentFlame = true;
    targetAngle = 20;  // RIGHT
    Serial.println("🔥 Flame at RIGHT!");
  } 
  else if (left == LOW && right == LOW) {
    currentFlame = true;
    targetAngle = 90;
    Serial.println("🔥 Flame on BOTH sides!");
  }

  // ===== If flame detected, update lastFlameTime =====
  if (currentFlame) {
    flameDetected = true;
    lastFlameTime = millis();
  } else {
    // If no flame for 2 seconds, then reset to center
    if (millis() - lastFlameTime > 2000) {
      flameDetected = false;
      targetAngle = 90;
    }
  }

  // ===== LED + BUZZER =====
  if (flameDetected) {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER, HIGH);

    // Turn ON pump for 3 seconds
    if (!pumpActive) {
      digitalWrite(RELAY_PIN, HIGH);
      pumpStartTime = millis();
      pumpActive = true;
      Serial.println("💧 Pump ON (3 seconds)");
    }
  } else {
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER, LOW);
  }

  // ===== Turn OFF Pump after 3 seconds =====
  if (pumpActive && millis() - pumpStartTime >= 3000) {
    digitalWrite(RELAY_PIN, LOW);
    pumpActive = false;
    Serial.println("💧 Pump OFF");
  }

  // ===== Smooth Servo Movement =====
  if (currentAngle < targetAngle) {
    currentAngle += 2;
  } else if (currentAngle > targetAngle) {
    currentAngle -= 2;
  }
  waterServo.write(currentAngle);

  delay(20);
}
