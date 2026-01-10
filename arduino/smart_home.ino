#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <Servo.h>
#include <ArduinoBLE.h>

#define DHTPIN 2
#define DHTTYPE DHT11
#define MQ2PIN A0
#define FLAME_ANALOG A1
#define SERVOPIN 6
#define TRIGPIN 10
#define ECHOPIN 11
#define BUZZERPIN 9

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 20, 4);
Servo garageServo;

BLEService edgeService("180C");
BLEStringCharacteristic edgeChar("2A56", BLERead | BLEWrite | BLENotify, 128);

bool emergencyActive = false;
bool garageBusy = false;
int gasThreshold = 400;

void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.init(); lcd.backlight();
  
  pinMode(TRIGPIN, OUTPUT); pinMode(ECHOPIN, INPUT);
  pinMode(MQ2PIN, INPUT); pinMode(FLAME_ANALOG, INPUT);
  pinMode(BUZZERPIN, OUTPUT);

  if (!BLE.begin()) { while (1); }
  BLE.setLocalName("UNO_R4_EDGE");
  edgeService.addCharacteristic(edgeChar);
  BLE.addService(edgeService);
  BLE.advertise();

  closeGarage(); 
  lcd.clear();
  lcd.print("SYSTEM READY");
}

void loop() {
  BLE.poll();
  unsigned long now = millis();

  // 1. PROCESS PI COMMANDS
  if (edgeChar.written()) {
    String cmd = edgeChar.value();
    cmd.trim(); cmd.toUpperCase();
    Serial.println("PI COMMAND: " + cmd);

    if (cmd == "FIRE") {
      emergencyActive = true;
      openGarage(); 
    } 
    else if (cmd == "SAFE") {
      emergencyActive = false;
      noTone(BUZZERPIN);
      closeGarage();
    }
  }

  // 2. EMERGENCY MAINTAINER (Non-blocking)
  if (emergencyActive) {
    tone(BUZZERPIN, 3000); 
    lcd.setCursor(0,2); lcd.print("Garage: EMERGENCY  ");
    lcd.setCursor(0,3); lcd.print("!!! FIRE ACTIVE !!! ");
    // Removed 'return' so sensor data keeps sending to Pi
  }

  // 3. NORMAL OPERATION (Only if no fire and not busy)
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int g = analogRead(MQ2PIN);
  int f = analogRead(FLAME_ANALOG);
  long dist = readDistance();

  if (!emergencyActive && !garageBusy) {
    // Local caution beep
    if (f < 500) {
      static unsigned long lastBeep = 0;
      if (now - lastBeep > 1500) { tone(BUZZERPIN, 1000, 100); lastBeep = now; }
    }

    // Hand trigger
    if (dist > 0 && dist <= 5) {
      runNormalCycle();
    }
  }

  // 4. DATA UPDATE TO PI (Runs ALWAYS so Pi can see when fire is gone)
  static unsigned long lastUpdate = 0;
  if (now - lastUpdate >= 1000) {
    lastUpdate = now;
    
    // Only update LCD if not in emergency to prevent flickering
    if (!emergencyActive) {
      updateLCD(t, h, g, f, dist);
    }
    
    String payload = "{\"t\":" + String(t,1) + ",\"h\":" + String(h,0) + ",\"g\":" + String(g) + ",\"f\":" + String(f) + "}";
    edgeChar.writeValue(payload.c_str());
    Serial.println("Sent: " + payload);
  }
}

void openGarage() {
  garageServo.attach(SERVOPIN);
  garageServo.write(150);
  delay(500); 
  garageServo.detach(); 
}

void closeGarage() {
  garageServo.attach(SERVOPIN);
  garageServo.write(0);
  delay(800);
  garageServo.detach();
}

void runNormalCycle() {
  garageBusy = true;
  tone(BUZZERPIN, 2000, 100);
  openGarage();
  for(int i=17; i > 0; i--) { 
    lcd.setCursor(0,2); lcd.print("Garage: ACTIVE     ");
    lcd.setCursor(0,3); lcd.print("CLOSE IN: "); lcd.print(i); lcd.print("s    ");
    delay(1000); 
  }
  closeGarage();
  garageBusy = false;
}

void updateLCD(float t, float h, int g, int f, long dist) {
  lcd.setCursor(0,0);
  lcd.print("Temp:"); lcd.print(t,1); lcd.print("C Hum:"); lcd.print(h,0); lcd.print("%   ");
  lcd.setCursor(0,1);
  lcd.print("Gas:"); lcd.print(g); lcd.print(" Flame:"); lcd.print(f); lcd.print("   ");
  lcd.setCursor(0,2);
  lcd.print("Garage:"); lcd.print(dist <= 5 ? "ACTIVE " : "READY  ");
  lcd.setCursor(0,3);
  if (g > gasThreshold || f < 500) {
    lcd.print("!!! DANGER ALERT !!!");
  } else {
    lcd.print("Status: MONITORING  ");
  }
}

long readDistance() {
  digitalWrite(TRIGPIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);
  long duration = pulseIn(ECHOPIN, HIGH, 20000);
  return (duration == 0) ? 999 : (duration * 0.034 / 2);
}
