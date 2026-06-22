// =====================================================
// BLYNK CONFIGURATION
// =====================================================
#define BLYNK_TEMPLATE_ID "TMPL6mbQkglXH"
#define BLYNK_TEMPLATE_NAME "Trần Quang Khả"
#define BLYNK_AUTH_TOKEN "B70EcFrotvg4_dddLBjGaZhOnAy-w-tx"

// =====================================================
// LIBRARIES
// =====================================================
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <DIYables_LCD_I2C.h>

// =====================================================
// WIFI CONFIGURATION
// =====================================================
char ssid[] = "VGU_Student_Guest";
char pass[] = "";

// =====================================================
// LCD I2C
// =====================================================
DIYables_LCD_I2C lcd(0x27, 16, 2);

// =====================================================
// PINS
// =====================================================
#define SOIL_PIN 34
#define RELAY_PIN 26

// =====================================================
// SENSOR CALIBRATION
// =====================================================
int dryValue = 2450;
int wetValue = 1100;

// =====================================================
// THRESHOLDS
// =====================================================
int pumpOnThreshold = 75;   // dưới 75% thì bơm
int pumpOffThreshold = 80;  // từ 80% trở lên thì dừng

// =====================================================
// TIMING
// =====================================================
unsigned long settleDelay = 1500;
unsigned long pumpDuration = 2000;
unsigned long afterPumpDelay = 1500;
unsigned long taskInterval = 5000;

// =====================================================
// SYSTEM STATE
// =====================================================
bool pumpRunning = false;
bool autoMode = true;
bool manualPumpRequest = false;

BlynkTimer timer;

// =====================================================
// PUMP CONTROL
// Relay: HIGH = ON, LOW = OFF
// =====================================================
void setPump(bool state) {
  if (state) {
    digitalWrite(RELAY_PIN, HIGH);
    pumpRunning = true;
  } else {
    digitalWrite(RELAY_PIN, LOW);
    pumpRunning = false;
  }

  Blynk.virtualWrite(V1, pumpRunning ? 1 : 0);
}

// =====================================================
// READ SENSOR
// =====================================================
int readSoilAverage() {
  long total = 0;
  int samples = 30;

  for (int i = 0; i < samples; i++) {
    total += analogRead(SOIL_PIN);
    delay(10);
  }

  return total / samples;
}

// =====================================================
// RAW TO MOISTURE
// =====================================================
int getMoisturePercent(int raw) {
  int moisturePercent = map(raw, dryValue, wetValue, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100);
  return moisturePercent;
}

// =====================================================
// SOIL STATUS
// =====================================================
String getSoilStatus(int moisturePercent) {
  if (moisturePercent < pumpOnThreshold) {
    return "DRY";
  } else if (moisturePercent >= pumpOffThreshold) {
    return "WET";
  } else {
    return "MID";
  }
}

// =====================================================
// LCD
// =====================================================
void updateLCD(int raw, int moisturePercent, String soilStatus, String actionText) {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("M:");
  lcd.print(moisturePercent);
  lcd.print("% ");
  lcd.print(soilStatus);

  lcd.setCursor(0, 1);
  lcd.print("R:");
  lcd.print(raw);
  lcd.print(" P:");
  lcd.print(pumpRunning ? "ON " : "OFF");
}

// =====================================================
// BLYNK SEND
// V0 = Soil Moisture %
// V1 = Pump Status
// V2 = Manual Pump
// V3 = Auto Mode
// =====================================================
void sendStatusToBlynk(int moisturePercent) {
  Blynk.virtualWrite(V0, moisturePercent);
  Blynk.virtualWrite(V1, pumpRunning ? 1 : 0);
  Blynk.virtualWrite(V3, autoMode ? 1 : 0);
}

// =====================================================
// SERIAL
// =====================================================
void printStatus(int raw, int moisturePercent, String soilStatus) {
  Serial.print("Raw: ");
  Serial.print(raw);

  Serial.print(" | Moisture: ");
  Serial.print(moisturePercent);
  Serial.print("%");

  Serial.print(" | Soil: ");
  Serial.print(soilStatus);

  Serial.print(" | Mode: ");
  Serial.print(autoMode ? "AUTO" : "MANUAL");

  Serial.print(" | Pump: ");
  Serial.println(pumpRunning ? "ON" : "OFF");
}

// =====================================================
// BLYNK V2 - MANUAL PUMP
// =====================================================
BLYNK_WRITE(V2) {
  manualPumpRequest = param.asInt();

  if (!autoMode) {
    setPump(manualPumpRequest);
  }
}

// =====================================================
// BLYNK V3 - AUTO MODE
// =====================================================
BLYNK_WRITE(V3) {
  autoMode = param.asInt();

  if (autoMode) {
    setPump(false);
  } else {
    setPump(manualPumpRequest);
  }
}

// =====================================================
// WHEN BLYNK CONNECTED
// =====================================================
BLYNK_CONNECTED() {
  Blynk.virtualWrite(V1, pumpRunning ? 1 : 0);
  Blynk.virtualWrite(V2, manualPumpRequest ? 1 : 0);
  Blynk.virtualWrite(V3, autoMode ? 1 : 0);
}

// =====================================================
// MAIN SMART WATERING TASK
// =====================================================
void smartWateringTask() {
  // MANUAL MODE
  if (!autoMode) {
    int raw = readSoilAverage();
    int moisturePercent = getMoisturePercent(raw);
    String soilStatus = getSoilStatus(moisturePercent);

    printStatus(raw, moisturePercent, soilStatus);
    updateLCD(raw, moisturePercent, soilStatus, "MAN");
    sendStatusToBlynk(moisturePercent);

    return;
  }

  // AUTO MODE:
  // Tắt bơm trước khi đọc sensor để tránh nhiễu
  setPump(false);
  delay(settleDelay);

  int raw = readSoilAverage();
  int moisturePercent = getMoisturePercent(raw);
  String soilStatus = getSoilStatus(moisturePercent);

  printStatus(raw, moisturePercent, soilStatus);
  updateLCD(raw, moisturePercent, soilStatus, "CHECK");
  sendStatusToBlynk(moisturePercent);

  // Nếu dưới 75% thì bơm một nhịp
  if (moisturePercent < pumpOnThreshold) {
    Serial.println("Action: Moisture below 75% -> Pump ON for 2 seconds");

    setPump(true);
    updateLCD(raw, moisturePercent, soilStatus, "WATER");
    sendStatusToBlynk(moisturePercent);

    delay(pumpDuration);

    setPump(false);
    updateLCD(raw, moisturePercent, soilStatus, "WAIT");
    sendStatusToBlynk(moisturePercent);

    delay(afterPumpDelay);
  } else {
    Serial.println("Action: Moisture enough -> Pump OFF");

    setPump(false);
    updateLCD(raw, moisturePercent, soilStatus, "OK");
    sendStatusToBlynk(moisturePercent);
  }

  Serial.println("---------------------------------------");
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(2000);

  analogReadResolution(12);
  analogSetPinAttenuation(SOIL_PIN, ADC_11db);

  pinMode(RELAY_PIN, OUTPUT);
  setPump(false);

  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Smart Watering");
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi");

  Serial.println("Connecting to WiFi and Blynk...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Blynk Connected");
  lcd.setCursor(0, 1);
  lcd.print("System Ready");

  delay(1500);

  timer.setInterval(taskInterval, smartWateringTask);

  Serial.println("=======================================");
  Serial.println("Smart Watering + LCD + Blynk");
  Serial.println("WiFi: VGU_Student_Guest");
  Serial.println("AUTO: pump OFF before sensor reading");
  Serial.println("Pump ON when moisture < 75%");
  Serial.println("Relay logic: HIGH = ON, LOW = OFF");
  Serial.println("=======================================");
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  Blynk.run();
  timer.run();
}
