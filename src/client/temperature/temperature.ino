// Pin
const int THERM_PIN = A0;

// Constants
const float SERIES_RESISTOR     = 10000.0; // 10k resistor
const float NOMINAL_RESISTANCE  = 10000.0; // 10k thermistor at 25°C
const float NOMINAL_TEMPERATURE = 25.0;    // °C
const float BETA_COEFFICIENT    = 3950.0;  // common value (check datasheet)

// ADC
const int ADC_MAX = 1023;

void setup() { Serial.begin(115200); }

void loop() {
  int adc = analogRead(THERM_PIN);

  // Convert ADC value to resistance
  float voltageRatio = (float)adc / ADC_MAX;
  float resistance = SERIES_RESISTOR * (1.0 / voltageRatio - 1.0);

  float tempC;
  tempC = resistance / NOMINAL_RESISTANCE;       // (R/Ro)
  tempC = log(tempC);                            // ln(R/Ro)
  tempC /= BETA_COEFFICIENT;                     // 1/B * ln(...)
  tempC += 1.0 / (NOMINAL_TEMPERATURE + 273.15); // + (1/To)
  tempC = 1.0 / tempC;                           // invert
  tempC -= 273.15;                               // K → °C

  float tempF = tempC * 9.0 / 5.0 + 32.0;

  Serial.print("tempC: ");
  Serial.print(tempC);
  Serial.print(" C");
  Serial.print(" (");
  Serial.print(tempF);
  Serial.println(" F)");

  delay(1000);
}