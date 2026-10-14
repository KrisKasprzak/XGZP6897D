/*
 * XGZP6897D_Basic.ino
 *
 * Demonstrates the XGZP6897D library over I2C:
 *   - temperature only
 *   - pressure only
 *   - pressure and temperature together
 *   - air velocity from a Pitot tube
 *   - sleep (continuous) mode
 *
 * Wiring (I2C, address 0x6D):
 *   Sensor VDD -> 3.3V or 5V (check your part number)
 *   Sensor GND -> GND
 *   Sensor SDA -> board SDA
 *   Sensor SCL -> board SCL
 *   Use pull-up resistors on SDA/SCL if your board/module does not have them.
 */

#include <Wire.h>
#include <CF_XGZP6897D.h>

// Set this to the full scale range of your sensor in kPa
// (for example 10 for an XGZP6897D -10..10 kPa part).
#define SENSOR_RANGE_KPA 10.0f

XGZP6897D pressureSensor(XGZP6897D::calcKFactor(SENSOR_RANGE_KPA));

void setup()
{
  uint8_t partId = 0;

  Serial.begin(115200);
  while (!Serial) { }

  Serial.println(F("XGZP6897D example"));

  if (!pressureSensor.begin(Wire))
  {
    Serial.println(F("Sensor not found at 0x6D - check wiring"));
    while (true) { delay(1000); }
  }

  Serial.print(F("K factor: "));
  Serial.println(pressureSensor.getKFactor());

  if (pressureSensor.readPartId(partId))
  {
    Serial.print(F("Part ID: 0x"));
    Serial.println(partId, HEX);
  }

  // Higher oversampling = less noise, longer conversion time
  pressureSensor.setPressureOversampling(XGZP6897D_OSR_4096X);

  // Ambient (absolute) pressure used for the air density calculation
  pressureSensor.setAmbientPressure(101325.0f);
  // Pitot tube coefficient, 1.0 for a standard Prandtl tube
  pressureSensor.setPitotCoefficient(1.0f);
}

void loop()
{
  float temperatureC = 0.0f;
  float pressurePa   = 0.0f;
  float velocityMps  = 0.0f;
  uint8_t loopCount  = 0;

  // 1. Temperature only
  if (pressureSensor.readTemperature(temperatureC))
  {
    Serial.print(F("Temperature only: "));
    Serial.print(temperatureC, 2);
    Serial.println(F(" C"));
  }

  // 2. Pressure only
  if (pressureSensor.readPressure(pressurePa))
  {
    Serial.print(F("Pressure only:    "));
    Serial.print(pressurePa, 2);
    Serial.println(F(" Pa"));
  }

  // 3. Both in a single combined conversion
  if (pressureSensor.readPressureAndTemperature(pressurePa, temperatureC))
  {
    Serial.print(F("Combined:         "));
    Serial.print(pressurePa, 2);
    Serial.print(F(" Pa, "));
    Serial.print(temperatureC, 2);
    Serial.println(F(" C"));
  }
  else
  {
    Serial.print(F("Read failed, error "));
    Serial.println(pressureSensor.getLastError());
  }

  // 4. Air velocity (uses measured temperature for air density)
  if (pressureSensor.readAirVelocity(velocityMps, pressurePa, temperatureC))
  {
    Serial.print(F("Air velocity:     "));
    Serial.print(velocityMps, 2);
    Serial.print(F(" m/s  (density "));
    Serial.print(pressureSensor.calcAirDensity(temperatureC), 4);
    Serial.println(F(" kg/m^3)"));
  }

  // 5. Sleep mode: the sensor converts on its own, we just read results
  pressureSensor.startSleepMode(XGZP6897D_SLEEP_125MS);
  for (loopCount = 0; loopCount < 3; loopCount++)
  {
    delay(200);
    if (pressureSensor.readLatest(pressurePa, temperatureC))
    {
      Serial.print(F("Sleep mode:       "));
      Serial.print(pressurePa, 2);
      Serial.print(F(" Pa, "));
      Serial.print(temperatureC, 2);
      Serial.println(F(" C"));
    }
  }
  pressureSensor.stopSleepMode();

  Serial.println();
  delay(2000);
}
