/*
 * CF_XGZP6897D.h - Arduino library for the CFSensor XGZP6897D I2C
 *               differential pressure sensor.
 *
 * ---------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2026 kasprzak
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * ---------------------------------------------------------------------------
 *
 * Revision History
 * ---------------------------------------------------------------------------
 * | Version | Date       | Author   | Description                          |
 * |---------|------------|----------|--------------------------------------|
 * | 1.0.0   | 2026-10-14 | kasprzak | Initial release                      |
 * ---------------------------------------------------------------------------
 *
 * IMPORTANT - sensor version
 *   CFSensor has shipped two incompatible parts under the XGZP6897D name:
 *     - Original version, I2C address 0x6D, datasheet V2.6  <-- THIS LIBRARY
 *     - Newer version,    I2C address 0x58, different registers/commands
 *   This library implements the 0x6D register map only. The same map is also
 *   used by the XGZP6899D, XGZP6847D, XGZP6857D, XGZP6859D, XGZP6869D,
 *   XGZP6877D, XGZP6887D and XGZP6858D (0x6D versions).
 *
 * Sensor overview
 *   - I2C interface, 7-bit address 0x6D
 *   - 24-bit signed pressure output, scaled by a range dependent K factor
 *   - 16-bit signed temperature output, 1/256 degC per LSB
 *   - Conversion modes: temperature only, pressure only, combined,
 *     and sleep mode (periodic combined conversions)
 */

#ifndef CF_XGZP6897D_H
#define CF_XGZP6897D_H

#include <Arduino.h>
#include <Wire.h>

// I2C address of the 0x6D (datasheet V2.6) version of the sensor
#define XGZP6897D_I2C_ADDRESS        0x6D

// Register map
#define XGZP6897D_REG_SPI_CTRL       0x00
#define XGZP6897D_REG_PART_ID        0x01
#define XGZP6897D_REG_STATUS         0x02
#define XGZP6897D_REG_PRESSURE_MSB   0x06
#define XGZP6897D_REG_PRESSURE_CSB   0x07
#define XGZP6897D_REG_PRESSURE_LSB   0x08
#define XGZP6897D_REG_TEMP_MSB       0x09
#define XGZP6897D_REG_TEMP_LSB       0x0A
#define XGZP6897D_REG_CMD            0x30
#define XGZP6897D_REG_SYS_CONFIG     0xA5
#define XGZP6897D_REG_P_CONFIG       0xA6

// CMD register bit fields
#define XGZP6897D_CMD_SCO_BIT        0x08   // start of conversion, self clearing
#define XGZP6897D_CMD_MODE_MASK      0x07
#define XGZP6897D_CMD_SLEEP_MASK     0xF0

// P_CONFIG register bit fields
#define XGZP6897D_OSR_P_MASK         0x07

// Physical constants used for air velocity
#define XGZP6897D_GAS_CONSTANT_AIR   287.058f   // J/(kg*K), dry air
#define XGZP6897D_STD_PRESSURE_PA    101325.0f  // Pa, sea level standard
#define XGZP6897D_KELVIN_OFFSET      273.15f

// Default maximum time to wait for a conversion to complete (ms)
#define XGZP6897D_DEFAULT_TIMEOUT_MS 100

// Measurement mode, written to CMD bits [2:0]
enum XGZP6897D_Mode
{
  XGZP6897D_MODE_TEMPERATURE = 0x00,  // single temperature conversion
  XGZP6897D_MODE_PRESSURE    = 0x01,  // single pressure conversion
  XGZP6897D_MODE_COMBINED    = 0x02,  // single temperature + pressure
  XGZP6897D_MODE_SLEEP       = 0x03   // periodic combined conversions
};

// Sleep time between conversions in sleep mode, written to CMD bits [7:4]
enum XGZP6897D_SleepTime
{
  XGZP6897D_SLEEP_0MS     = 0x00,
  XGZP6897D_SLEEP_62_5MS  = 0x01,
  XGZP6897D_SLEEP_125MS   = 0x02,
  XGZP6897D_SLEEP_187_5MS = 0x03,
  XGZP6897D_SLEEP_250MS   = 0x04,
  XGZP6897D_SLEEP_312_5MS = 0x05,
  XGZP6897D_SLEEP_375MS   = 0x06,
  XGZP6897D_SLEEP_437_5MS = 0x07,
  XGZP6897D_SLEEP_500MS   = 0x08,
  XGZP6897D_SLEEP_562_5MS = 0x09,
  XGZP6897D_SLEEP_625MS   = 0x0A,
  XGZP6897D_SLEEP_687_5MS = 0x0B,
  XGZP6897D_SLEEP_750MS   = 0x0C,
  XGZP6897D_SLEEP_812_5MS = 0x0D,
  XGZP6897D_SLEEP_875MS   = 0x0E,
  XGZP6897D_SLEEP_1000MS  = 0x0F
};

// Pressure oversampling ratio, written to P_CONFIG bits [2:0]
enum XGZP6897D_Oversampling
{
  XGZP6897D_OSR_1024X  = 0x00,
  XGZP6897D_OSR_2048X  = 0x01,
  XGZP6897D_OSR_4096X  = 0x02,
  XGZP6897D_OSR_8192X  = 0x03,
  XGZP6897D_OSR_256X   = 0x04,
  XGZP6897D_OSR_512X   = 0x05,
  XGZP6897D_OSR_16384X = 0x06,
  XGZP6897D_OSR_32768X = 0x07
};

// Result / error codes
enum XGZP6897D_Error
{
  XGZP6897D_OK            = 0,
  XGZP6897D_ERR_I2C_WRITE = 1,  // NACK or bus error while writing
  XGZP6897D_ERR_I2C_READ  = 2,  // fewer bytes returned than requested
  XGZP6897D_ERR_TIMEOUT   = 3,  // conversion did not complete in time
  XGZP6897D_ERR_NOT_INIT  = 4   // begin() has not been called
};

class XGZP6897D
{
public:
  // kFactor is the pressure scale factor from the datasheet for the
  // specific sensor range. Use calcKFactor() if you only know the range.
  // i2cAddress defaults to 0x6D, the address of the supported version.
  XGZP6897D(uint16_t kFactor, uint8_t i2cAddress = XGZP6897D_I2C_ADDRESS);

  // ---- Setup -------------------------------------------------------------
  bool    begin(TwoWire &wirePort = Wire);
  bool    isConnected();
  uint8_t getAddress();
  void    setKFactor(uint16_t kFactor);
  uint16_t getKFactor();
  static uint16_t calcKFactor(float rangeKPa);
  void    setTimeout(uint16_t timeoutMs);
  uint16_t getTimeout();
  uint8_t getLastError();

  // ---- Single shot measurements (start conversion, wait, read) ----------
  bool    readTemperature(float &temperatureC);
  bool    readPressure(float &pressurePa);
  bool    readPressureAndTemperature(float &pressurePa, float &temperatureC);

  // Convenience versions, return NAN on failure
  float   getTemperature();
  float   getTemperatureF();
  float   getPressure();

  // ---- Raw register data -------------------------------------------------
  bool    readRawTemperature(int16_t &rawTemperature);
  bool    readRawPressure(int32_t &rawPressure);
  bool    readRawData(int32_t &rawPressure, int16_t &rawTemperature);

  // Read the latest data registers without starting a new conversion
  // (use with sleep mode or after startConversion()/waitForConversion()).
  bool    readLatest(float &pressurePa, float &temperatureC);

  // ---- Conversion control ------------------------------------------------
  bool    startConversion(XGZP6897D_Mode measureMode);
  bool    isConversionComplete();
  bool    waitForConversion();
  bool    startSleepMode(XGZP6897D_SleepTime sleepTime);
  bool    stopSleepMode();

  // ---- Configuration -----------------------------------------------------
  bool    setPressureOversampling(XGZP6897D_Oversampling osrValue);
  bool    getPressureOversampling(XGZP6897D_Oversampling &osrValue);
  bool    readPartId(uint8_t &partId);
  bool    readStatus(uint8_t &statusValue);

  // ---- Generic register access ------------------------------------------
  bool    readRegister(uint8_t regAddress, uint8_t &regValue);
  bool    readRegisters(uint8_t regAddress, uint8_t *dataBuffer, uint8_t byteCount);
  bool    writeRegister(uint8_t regAddress, uint8_t regValue);

  // ---- Air velocity (Pitot tube / Prandtl tube) --------------------------
  void    setAmbientPressure(float ambientPa);
  float   getAmbientPressure();
  void    setPitotCoefficient(float pitotCoeff);
  float   getPitotCoefficient();
  float   calcAirDensity(float temperatureC);
  float   calcAirVelocity(float pressurePa, float airDensity);

  // Measures pressure and temperature and returns air velocity in m/s.
  // Sign follows the sign of the differential pressure. NAN on failure.
  float   getAirVelocity();
  // Same, but with a caller supplied air density in kg/m^3.
  float   getAirVelocity(float airDensity);
  bool    readAirVelocity(float &velocityMps, float &pressurePa, float &temperatureC);

private:
  float   rawToPressure(int32_t rawPressure);
  float   rawToTemperature(int16_t rawTemperature);

  TwoWire *_wirePort;
  uint8_t  _i2cAddress;
  uint16_t _kFactor;
  uint16_t _timeoutMs;
  uint8_t  _lastError;
  float    _ambientPa;
  float    _pitotCoeff;
};

#endif // CF_XGZP6897D_H
