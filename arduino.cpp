"#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>  // Remove if not using LCD

// ── Pin Definitions ─────────────────────────────────────
#define DHT_PIN      2       // DHT22 data pin
#define DHT_TYPE     DHT22
#define MQ135_PIN    A0      // MQ-135 analog out
#define TDS_PIN      A1      // TDS sensor analog out
#define PH_PIN       A2      // pH sensor analog out

// ── Objects ─────────────────────────────────────────────
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Remove if not using LCD

// ── Calibration constants ────────────────────────────────
const float VREF       = 5.0;   // Arduino supply voltage
const float ADC_RES    = 1023.0;

// ── Setup ────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  dht.begin();

  // LCD setup (remove block if not using LCD)
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Pollution Detect");
  lcd.setCursor(0, 1);
  lcd.print("  Starting...   ");
  delay(2000);
  lcd.clear();

  Serial.println("========================================");
  Serial.println("   Real-Time Pollution Detector");
  Serial.println("========================================");
  delay(2000);  // Let MQ-135 warm up a bit
}

// ── Helper: Read Air Quality ─────────────────────────────
String readAirQuality(float &voltage) {
  int raw = analogRead(MQ135_PIN);
  voltage = (raw / ADC_RES) * VREF;

  if (voltage < 1.0)       return "Good";
  else if (voltage < 2.0)  return "Moderate";
  else                     return "Poor";
}

// ── Helper: Read TDS (ppm) ───────────────────────────────
float readTDS() {
  int raw = analogRead(TDS_PIN);
  float voltage = (raw / ADC_RES) * VREF;
  // Common TDS module formula (adjust factor for your module)
  float tds = (voltage / VREF) * 1000.0;
  return tds;
}

// ── Helper: Read pH ──────────────────────────────────────
float readPH() {
  // Average multiple readings to reduce noise
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(PH_PIN);
    delay(10);
  }
  float avg_raw = sum / 10.0;
  float voltage = (avg_raw / ADC_RES) * VREF;

  // pH module linear mapping: ~2.5V = pH 7 (calibrate with buffer solution)
  float ph = 3.5 * voltage + 0.0;  // Adjust offset after calibration
  return ph;
}

// ── Helper: Water Status ─────────────────────────────────
String waterStatus(float tds, float ph) {
  if (tds < 300 && ph >= 6.5 && ph <= 8.5)  return "Safe";
  else if (tds < 600 && ph >= 5.5 && ph <= 9.0) return "Acceptable";
  else return "UNSAFE";
}

// ── Main Loop ────────────────────────────────────────────
void loop() {
  // Read all sensors
  float airVoltage;
  String airLabel  = readAirQuality(airVoltage);
  float  tds       = readTDS();
  float  ph        = readPH();
  String wStatus   = waterStatus(tds, ph);
  float  temp      = dht.readTemperature();
  float  humidity  = dht.readHumidity();

  // ── Serial Monitor output ──
  Serial.println("----------------------------------------");
  Serial.print("AIR     | Quality  : "); Serial.println(airLabel);
  Serial.print("        | Voltage  : "); Serial.print(airVoltage, 2); Serial.println(" V");

  if (!isnan(temp) && !isnan(humidity)) {
    Serial.print("        | Temp     : "); Serial.print(temp, 1); Serial.println(" °C");
    Serial.print("        | Humidity : "); Serial.print(humidity, 1); Serial.println(" %");
  } else {
    Serial.println("        | DHT22    : read error");
  }

  Serial.print("WATER   | TDS      : "); Serial.print(tds, 1); Serial.println(" ppm");
  Serial.print("        | pH       : "); Serial.println(ph, 2);
  Serial.print("        | Status   : "); Serial.println(wStatus);
  Serial.println("----------------------------------------");

  // ── LCD output (remove block if not using LCD) ──
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Air:");
  lcd.print(airLabel);
  lcd.print(" ");
  if (!isnan(temp)) {
    lcd.print((int)temp);
    lcd.print((char)223);  // degree symbol
    lcd.print("C");
  }
  lcd.setCursor(0, 1);
  lcd.print("H2O:");
  lcd.print(wStatus);
  lcd.print(" pH:");
  lcd.print(ph, 1);

  delay(3000);  // Read every 3 seconds
}"