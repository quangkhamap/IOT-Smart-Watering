#define SOIL_PIN 34
#define RELAY_PIN 26

int dryValue = 2100;
int wetValue = 600;

int moistureThreshold = 40;

unsigned long readInterval = 1000;

bool pumpRunning = false;

void setPump(bool state) {
  if (state) {
    digitalWrite(RELAY_PIN, HIGH);  // Relay ON, Pump ON
    pumpRunning = true;
  } else {
    digitalWrite(RELAY_PIN, LOW);   // Relay OFF, Pump OFF
    pumpRunning = false;
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  analogReadResolution(12);

  pinMode(RELAY_PIN, OUTPUT);

  setPump(false);

  
  Serial.println("IoT Smart Watering System");
 
}

void loop() {
  int soilRaw = analogRead(SOIL_PIN);

  int moisturePercent = map(soilRaw, dryValue, wetValue, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100);

  Serial.print("Raw: ");
  Serial.print(soilRaw);

  Serial.print(" | Moisture: ");
  Serial.print(moisturePercent);
  Serial.print("%");

  if (moisturePercent < moistureThreshold) {
    Serial.print(" | Soil: Dry");

    if (!pumpRunning) {
      Serial.println(" | Pump: ON");
      setPump(true);
    } else {
      Serial.println(" | Pump: Still ON");
    }
  } else {
    Serial.print(" | Soil: Moist enough");

    if (pumpRunning) {
      Serial.println(" | Pump: OFF");
      setPump(false);
    } else {
      Serial.println(" | Pump: Already OFF");
    }
  }


  delay(readInterval);
}
