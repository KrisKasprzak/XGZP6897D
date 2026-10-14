# CF_XGZP6897D Arduino Library

An Arduino library for the **CFSensor XGZP6897D** differential pressure sensor. The sensor talks over **I2C** at the fixed address `0x6D`.

## ⚠️ Sensor version

CFSensor has sold **two incompatible sensors under the XGZP6897D name**:

| Version  | I2C address | Datasheet | Supported |
|----------|-------------|-----------|-----------|
| Original | `0x6D`      | V2.6      | **Yes**   |
| Newer    | `0x58`      | Later revision with different registers and commands | No |

This library implements the **0x6D** register map. The 0x6D versions of the XGZP6899D, XGZP6847D, XGZP6857D, XGZP6859D, XGZP6869D, XGZP6877D, XGZP6887D and XGZP6858D use the same register map, so the library works with them too. To see which version you have, run an I2C scanner.

## Features

- Read temperature only, pressure only, or both in one combined conversion
- Single-shot mode and sleep mode (the sensor converts continuously on its own)
- Pressure oversampling setting (256X to 32768X)
- Read raw 24-bit pressure and 16-bit temperature counts
- Read and write any register directly
- Air velocity from a Pitot or Prandtl tube, with air density corrected for temperature
- Works with any `TwoWire` bus (`Wire`, `Wire1`, ...)
- Error reporting through return values and `getLastError()`

## Installation

1. Copy this folder into your Arduino `libraries` directory, or
2. Zip the folder and use **Sketch → Include Library → Add .ZIP Library...**

## Wiring

| Sensor | Arduino      |
|--------|--------------|
| VDD    | 3.3V / 5V (check your part number) |
| GND    | GND          |
| SDA    | SDA          |
| SCL    | SCL          |

Add pull-up resistors on SDA and SCL (typically 4.7 kΩ) if your board or module does not already have them.

## K factor

The sensor's raw pressure count is divided by a **K factor** that depends on its full-scale range:

| Range (kPa)        | K    |
|--------------------|------|
| 500 < P ≤ 1000     | 8    |
| 260 < P ≤ 500      | 16   |
| 131 < P ≤ 260      | 32   |
| 65 < P ≤ 131       | 64   |
| 32 < P ≤ 65        | 128  |
| 16 < P ≤ 32        | 256  |
| 8 < P ≤ 16         | 512  |
| 4 < P ≤ 8          | 1024 |
| 2 ≤ P ≤ 4          | 2048 |
| 1 ≤ P < 2          | 4096 |
| P < 1              | 8192 |

You can pass K directly, or let `XGZP6897D::calcKFactor(rangeKPa)` look it up:

```cpp
XGZP6897D sensor(XGZP6897D::calcKFactor(10.0));  // -10..10 kPa part -> K = 512
```

## Quick start

```cpp
#include <Wire.h>
#include <CF_XGZP6897D.h>

XGZP6897D sensor(XGZP6897D::calcKFactor(10.0));

void setup() {
  Serial.begin(115200);
  if (!sensor.begin()) {
    Serial.println("Sensor not found");
    while (true) {}
  }
}

void loop() {
  float pressurePa, temperatureC;
  if (sensor.readPressureAndTemperature(pressurePa, temperatureC)) {
    Serial.print(pressurePa);   Serial.print(" Pa  ");
    Serial.print(temperatureC); Serial.println(" C");
  }
  Serial.print("Velocity: ");
  Serial.print(sensor.getAirVelocity());
  Serial.println(" m/s");
  delay(500);
}
```

A complete example is in [examples/XGZP6897D_Basic](examples/XGZP6897D_Basic/XGZP6897D_Basic.ino).

## API reference

Unless noted otherwise, `bool` methods return `true` on success. When one fails, call `getLastError()` to find out why.

### Setup

| Method | Description |
|--------|-------------|
| `XGZP6897D(uint16_t kFactor, uint8_t i2cAddress = 0x6D)` | Constructor. |
| `bool begin(TwoWire &wirePort = Wire)` | Starts the I2C bus and checks that the sensor answers. |
| `bool isConnected()` | Checks that the sensor ACKs at its address. |
| `uint8_t getAddress()` | Returns the I2C address in use. |
| `void setKFactor(uint16_t)` / `uint16_t getKFactor()` | Sets or gets the pressure scale factor. |
| `static uint16_t calcKFactor(float rangeKPa)` | Looks up the K factor for a full-scale range. |
| `void setTimeout(uint16_t ms)` / `uint16_t getTimeout()` | Sets or gets how long to wait for a conversion (default 100 ms). |
| `uint8_t getLastError()` | Returns the last `XGZP6897D_Error` code. |

### Measurements

| Method | Description |
|--------|-------------|
| `bool readTemperature(float &temperatureC)` | Single temperature conversion. |
| `bool readPressure(float &pressurePa)` | Single pressure conversion. |
| `bool readPressureAndTemperature(float &pressurePa, float &temperatureC)` | Single combined conversion. |
| `float getTemperature()` | Temperature in °C, or `NAN` on failure. |
| `float getTemperatureF()` | Temperature in °F, or `NAN` on failure. |
| `float getPressure()` | Pressure in Pa, or `NAN` on failure. |
| `bool readLatest(float &pressurePa, float &temperatureC)` | Reads the data registers without starting a new conversion. Use this in sleep mode. |

### Raw data

| Method | Description |
|--------|-------------|
| `bool readRawPressure(int32_t &raw)` | Signed 24-bit pressure count. |
| `bool readRawTemperature(int16_t &raw)` | Signed 16-bit temperature count (1/256 °C). |
| `bool readRawData(int32_t &rawP, int16_t &rawT)` | Reads both counts in one burst. |

### Conversion control

| Method | Description |
|--------|-------------|
| `bool startConversion(XGZP6897D_Mode mode)` | Starts a conversion without waiting for it: `XGZP6897D_MODE_TEMPERATURE`, `_PRESSURE`, `_COMBINED` or `_SLEEP`. |
| `bool isConversionComplete()` | `true` once the SCO bit has cleared. |
| `bool waitForConversion()` | Polls until the conversion finishes or the timeout expires. |
| `bool startSleepMode(XGZP6897D_SleepTime t)` | Starts periodic combined conversions, with `t` = `XGZP6897D_SLEEP_0MS` … `XGZP6897D_SLEEP_1000MS` between them. |
| `bool stopSleepMode()` | Stops periodic conversions. |

### Configuration and registers

| Method | Description |
|--------|-------------|
| `bool setPressureOversampling(XGZP6897D_Oversampling osr)` | Sets OSR_P (register 0xA6, bits 2:0). Other bits are left unchanged. |
| `bool getPressureOversampling(XGZP6897D_Oversampling &osr)` | Reads the current OSR_P setting. |
| `bool readPartId(uint8_t &id)` | Reads register 0x01. |
| `bool readStatus(uint8_t &status)` | Reads register 0x02. |
| `bool readRegister(uint8_t reg, uint8_t &value)` | Reads one register. |
| `bool readRegisters(uint8_t reg, uint8_t *buf, uint8_t count)` | Reads several registers in one burst. |
| `bool writeRegister(uint8_t reg, uint8_t value)` | Writes one register. |

### Air velocity

Velocity comes from the Pitot equation:

```
rho = P_ambient / (287.058 * (T + 273.15))      // dry-air density, kg/m^3
v   = C * sqrt(2 * |dP| / rho)                  // m/s, signed like dP
```

| Method | Description |
|--------|-------------|
| `float getAirVelocity()` | Measures dP and T and returns velocity in m/s (`NAN` on failure). |
| `float getAirVelocity(float airDensity)` | Measures dP only and uses the air density you pass in (kg/m³). |
| `bool readAirVelocity(float &v, float &dP, float &T)` | Returns velocity, pressure and temperature from one conversion. |
| `void setAmbientPressure(float pa)` / `float getAmbientPressure()` | Absolute pressure used for density (default 101325 Pa). |
| `void setPitotCoefficient(float c)` / `float getPitotCoefficient()` | Pitot tube coefficient C (default 1.0). |
| `float calcAirDensity(float temperatureC)` | Dry-air density at the ambient pressure you set. |
| `float calcAirVelocity(float dPa, float density)` | Pure calculation, no I2C. |

To convert m/s to other units, multiply by 3.6 for km/h, 196.85 for ft/min, or 2.237 for mph.

### Error codes

| Code | Meaning |
|------|---------|
| `XGZP6897D_OK` (0) | Success |
| `XGZP6897D_ERR_I2C_WRITE` (1) | NACK or bus error during a write |
| `XGZP6897D_ERR_I2C_READ` (2) | Fewer bytes arrived than were requested |
| `XGZP6897D_ERR_TIMEOUT` (3) | The conversion did not finish within the timeout |
| `XGZP6897D_ERR_NOT_INIT` (4) | `begin()` has not been called |

## License

MIT License. The full text is in [src/CF_XGZP6897D.h](src/CF_XGZP6897D.h).

## Revision history

| Version | Author                                   | Date       | Description     |
|---------|------------------------------------------|------------|-----------------|
| 1.0.0   | Claude Code opus-5-5, tested by Kasprzak  |2026-10-14 | Initial release |
