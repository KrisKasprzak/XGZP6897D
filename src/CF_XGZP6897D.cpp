/*
 * CF_XGZP6897D.cpp - Arduino library for the CFSensor XGZP6897D I2C
 *                 differential pressure sensor.
 *
 * MIT License - see CF_XGZP6897D.h for the full license text and revision history.
 */

#include "CF_XGZP6897D.h"
#include <math.h>

XGZP6897D::XGZP6897D(uint16_t kFactor, uint8_t i2cAddress)
{
  _wirePort   = NULL;
  _i2cAddress = i2cAddress;
  _kFactor    = (kFactor == 0) ? 1 : kFactor;
  _timeoutMs  = XGZP6897D_DEFAULT_TIMEOUT_MS;
  _lastError  = XGZP6897D_OK;
  _ambientPa  = XGZP6897D_STD_PRESSURE_PA;
  _pitotCoeff = 1.0f;
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

bool XGZP6897D::begin(TwoWire &wirePort)
{
  _wirePort = &wirePort;
  _wirePort->begin();
  return isConnected();
}

bool XGZP6897D::isConnected()
{
  if (_wirePort == NULL)
  {
    _lastError = XGZP6897D_ERR_NOT_INIT;
    return false;
  }
  _wirePort->beginTransmission(_i2cAddress);
  if (_wirePort->endTransmission() != 0)
  {
    _lastError = XGZP6897D_ERR_I2C_WRITE;
    return false;
  }
  _lastError = XGZP6897D_OK;
  return true;
}

uint8_t XGZP6897D::getAddress()
{
  return _i2cAddress;
}

void XGZP6897D::setKFactor(uint16_t kFactor)
{
  _kFactor = (kFactor == 0) ? 1 : kFactor;
}

uint16_t XGZP6897D::getKFactor()
{
  return _kFactor;
}

// Returns the datasheet K factor for a sensor whose full scale range is
// rangeKPa (use the magnitude, e.g. 10 for a -10..10 kPa part).
uint16_t XGZP6897D::calcKFactor(float rangeKPa)
{
  float absRange = fabs(rangeKPa);

  if (absRange > 500.0f) return 8;
  if (absRange > 260.0f) return 16;
  if (absRange > 131.0f) return 32;
  if (absRange > 65.0f)  return 64;
  if (absRange > 32.0f)  return 128;
  if (absRange > 16.0f)  return 256;
  if (absRange > 8.0f)   return 512;
  if (absRange > 4.0f)   return 1024;
  if (absRange >= 2.0f)  return 2048;
  if (absRange >= 1.0f)  return 4096;
  return 8192;
}

void XGZP6897D::setTimeout(uint16_t timeoutMs)
{
  _timeoutMs = timeoutMs;
}

uint16_t XGZP6897D::getTimeout()
{
  return _timeoutMs;
}

uint8_t XGZP6897D::getLastError()
{
  return _lastError;
}

// ---------------------------------------------------------------------------
// Single shot measurements
// ---------------------------------------------------------------------------

bool XGZP6897D::readTemperature(float &temperatureC)
{
  int16_t rawTemperature = 0;

  if (!startConversion(XGZP6897D_MODE_TEMPERATURE)) return false;
  if (!waitForConversion()) return false;
  if (!readRawTemperature(rawTemperature)) return false;

  temperatureC = rawToTemperature(rawTemperature);
  return true;
}

bool XGZP6897D::readPressure(float &pressurePa)
{
  int32_t rawPressure = 0;

  if (!startConversion(XGZP6897D_MODE_PRESSURE)) return false;
  if (!waitForConversion()) return false;
  if (!readRawPressure(rawPressure)) return false;

  pressurePa = rawToPressure(rawPressure);
  return true;
}

bool XGZP6897D::readPressureAndTemperature(float &pressurePa, float &temperatureC)
{
  if (!startConversion(XGZP6897D_MODE_COMBINED)) return false;
  if (!waitForConversion()) return false;
  return readLatest(pressurePa, temperatureC);
}

float XGZP6897D::getTemperature()
{
  float temperatureC = NAN;
  if (!readTemperature(temperatureC)) return NAN;
  return temperatureC;
}

float XGZP6897D::getTemperatureF()
{
  float temperatureC = getTemperature();
  if (isnan(temperatureC)) return NAN;
  return temperatureC * 9.0f / 5.0f + 32.0f;
}

float XGZP6897D::getPressure()
{
  float pressurePa = NAN;
  if (!readPressure(pressurePa)) return NAN;
  return pressurePa;
}

// ---------------------------------------------------------------------------
// Raw register data
// ---------------------------------------------------------------------------

bool XGZP6897D::readRawTemperature(int16_t &rawTemperature)
{
  uint8_t dataBuffer[2];

  if (!readRegisters(XGZP6897D_REG_TEMP_MSB, dataBuffer, 2)) return false;

  rawTemperature = (int16_t)(((uint16_t)dataBuffer[0] << 8) | dataBuffer[1]);
  return true;
}

bool XGZP6897D::readRawPressure(int32_t &rawPressure)
{
  uint8_t dataBuffer[3];

  if (!readRegisters(XGZP6897D_REG_PRESSURE_MSB, dataBuffer, 3)) return false;

  rawPressure = ((int32_t)dataBuffer[0] << 16) |
                ((int32_t)dataBuffer[1] << 8)  |
                 (int32_t)dataBuffer[2];
  // 24-bit two's complement sign extension
  if (rawPressure & 0x800000L)
  {
    rawPressure -= 0x1000000L;
  }
  return true;
}

bool XGZP6897D::readRawData(int32_t &rawPressure, int16_t &rawTemperature)
{
  uint8_t dataBuffer[5];

  // Pressure (0x06..0x08) and temperature (0x09..0x0A) are contiguous
  if (!readRegisters(XGZP6897D_REG_PRESSURE_MSB, dataBuffer, 5)) return false;

  rawPressure = ((int32_t)dataBuffer[0] << 16) |
                ((int32_t)dataBuffer[1] << 8)  |
                 (int32_t)dataBuffer[2];
  if (rawPressure & 0x800000L)
  {
    rawPressure -= 0x1000000L;
  }
  rawTemperature = (int16_t)(((uint16_t)dataBuffer[3] << 8) | dataBuffer[4]);
  return true;
}

bool XGZP6897D::readLatest(float &pressurePa, float &temperatureC)
{
  int32_t rawPressure    = 0;
  int16_t rawTemperature = 0;

  if (!readRawData(rawPressure, rawTemperature)) return false;

  pressurePa   = rawToPressure(rawPressure);
  temperatureC = rawToTemperature(rawTemperature);
  return true;
}

// ---------------------------------------------------------------------------
// Conversion control
// ---------------------------------------------------------------------------

bool XGZP6897D::startConversion(XGZP6897D_Mode measureMode)
{
  uint8_t cmdValue = XGZP6897D_CMD_SCO_BIT | ((uint8_t)measureMode & XGZP6897D_CMD_MODE_MASK);
  return writeRegister(XGZP6897D_REG_CMD, cmdValue);
}

bool XGZP6897D::isConversionComplete()
{
  uint8_t cmdValue = 0;

  if (!readRegister(XGZP6897D_REG_CMD, cmdValue)) return false;
  // SCO bit is cleared by the sensor once the conversion is finished
  return (cmdValue & XGZP6897D_CMD_SCO_BIT) == 0;
}

bool XGZP6897D::waitForConversion()
{
  uint32_t startMs = millis();
  uint8_t  cmdValue = 0;

  while (true)
  {
    if (!readRegister(XGZP6897D_REG_CMD, cmdValue)) return false;
    if ((cmdValue & XGZP6897D_CMD_SCO_BIT) == 0)
    {
      _lastError = XGZP6897D_OK;
      return true;
    }
    if ((uint32_t)(millis() - startMs) >= _timeoutMs)
    {
      _lastError = XGZP6897D_ERR_TIMEOUT;
      return false;
    }
    delay(1);
  }
}

bool XGZP6897D::startSleepMode(XGZP6897D_SleepTime sleepTime)
{
  uint8_t cmdValue = (((uint8_t)sleepTime << 4) & XGZP6897D_CMD_SLEEP_MASK) |
                     XGZP6897D_CMD_SCO_BIT |
                     XGZP6897D_MODE_SLEEP;
  return writeRegister(XGZP6897D_REG_CMD, cmdValue);
}

bool XGZP6897D::stopSleepMode()
{
  // Clearing SCO and the sleep time halts periodic conversions
  return writeRegister(XGZP6897D_REG_CMD, 0x00);
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

bool XGZP6897D::setPressureOversampling(XGZP6897D_Oversampling osrValue)
{
  uint8_t regValue = 0;

  // Read-modify-write so the gain and other bits are preserved
  if (!readRegister(XGZP6897D_REG_P_CONFIG, regValue)) return false;
  regValue = (regValue & (uint8_t)~XGZP6897D_OSR_P_MASK) |
             ((uint8_t)osrValue & XGZP6897D_OSR_P_MASK);
  return writeRegister(XGZP6897D_REG_P_CONFIG, regValue);
}

bool XGZP6897D::getPressureOversampling(XGZP6897D_Oversampling &osrValue)
{
  uint8_t regValue = 0;

  if (!readRegister(XGZP6897D_REG_P_CONFIG, regValue)) return false;
  osrValue = (XGZP6897D_Oversampling)(regValue & XGZP6897D_OSR_P_MASK);
  return true;
}

bool XGZP6897D::readPartId(uint8_t &partId)
{
  return readRegister(XGZP6897D_REG_PART_ID, partId);
}

bool XGZP6897D::readStatus(uint8_t &statusValue)
{
  return readRegister(XGZP6897D_REG_STATUS, statusValue);
}

// ---------------------------------------------------------------------------
// Generic register access (I2C)
// ---------------------------------------------------------------------------

bool XGZP6897D::readRegister(uint8_t regAddress, uint8_t &regValue)
{
  return readRegisters(regAddress, &regValue, 1);
}

bool XGZP6897D::readRegisters(uint8_t regAddress, uint8_t *dataBuffer, uint8_t byteCount)
{
  uint8_t idx = 0;

  if (_wirePort == NULL)
  {
    _lastError = XGZP6897D_ERR_NOT_INIT;
    return false;
  }

  _wirePort->beginTransmission(_i2cAddress);
  _wirePort->write(regAddress);
  if (_wirePort->endTransmission(false) != 0)   // repeated start
  {
    _lastError = XGZP6897D_ERR_I2C_WRITE;
    return false;
  }

  if (_wirePort->requestFrom(_i2cAddress, byteCount) != byteCount)
  {
    // Drain anything that did arrive
    while (_wirePort->available()) _wirePort->read();
    _lastError = XGZP6897D_ERR_I2C_READ;
    return false;
  }

  for (idx = 0; idx < byteCount; idx++)
  {
    dataBuffer[idx] = (uint8_t)_wirePort->read();
  }

  _lastError = XGZP6897D_OK;
  return true;
}

bool XGZP6897D::writeRegister(uint8_t regAddress, uint8_t regValue)
{
  if (_wirePort == NULL)
  {
    _lastError = XGZP6897D_ERR_NOT_INIT;
    return false;
  }

  _wirePort->beginTransmission(_i2cAddress);
  _wirePort->write(regAddress);
  _wirePort->write(regValue);
  if (_wirePort->endTransmission() != 0)
  {
    _lastError = XGZP6897D_ERR_I2C_WRITE;
    return false;
  }

  _lastError = XGZP6897D_OK;
  return true;
}

// ---------------------------------------------------------------------------
// Air velocity
// ---------------------------------------------------------------------------

void XGZP6897D::setAmbientPressure(float ambientPa)
{
  if (ambientPa > 0.0f) _ambientPa = ambientPa;
}

float XGZP6897D::getAmbientPressure()
{
  return _ambientPa;
}

void XGZP6897D::setPitotCoefficient(float pitotCoeff)
{
  if (pitotCoeff > 0.0f) _pitotCoeff = pitotCoeff;
}

float XGZP6897D::getPitotCoefficient()
{
  return _pitotCoeff;
}

// Ideal gas law for dry air: rho = P / (R * T)
float XGZP6897D::calcAirDensity(float temperatureC)
{
  float tempKelvin = temperatureC + XGZP6897D_KELVIN_OFFSET;
  if (tempKelvin <= 0.0f) return NAN;
  return _ambientPa / (XGZP6897D_GAS_CONSTANT_AIR * tempKelvin);
}

// Bernoulli / Pitot equation: v = C * sqrt(2 * dP / rho)
// The result carries the sign of the differential pressure so flow
// direction is preserved.
float XGZP6897D::calcAirVelocity(float pressurePa, float airDensity)
{
  float velocityMps = 0.0f;

  if (isnan(pressurePa) || isnan(airDensity) || airDensity <= 0.0f) return NAN;

  velocityMps = _pitotCoeff * sqrt(2.0f * fabs(pressurePa) / airDensity);
  return (pressurePa < 0.0f) ? -velocityMps : velocityMps;
}

float XGZP6897D::getAirVelocity()
{
  float velocityMps  = NAN;
  float pressurePa   = NAN;
  float temperatureC = NAN;

  if (!readAirVelocity(velocityMps, pressurePa, temperatureC)) return NAN;
  return velocityMps;
}

float XGZP6897D::getAirVelocity(float airDensity)
{
  float pressurePa = NAN;

  if (!readPressure(pressurePa)) return NAN;
  return calcAirVelocity(pressurePa, airDensity);
}

bool XGZP6897D::readAirVelocity(float &velocityMps, float &pressurePa, float &temperatureC)
{
  if (!readPressureAndTemperature(pressurePa, temperatureC)) return false;
  velocityMps = calcAirVelocity(pressurePa, calcAirDensity(temperatureC));
  return !isnan(velocityMps);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

float XGZP6897D::rawToPressure(int32_t rawPressure)
{
  return (float)rawPressure / (float)_kFactor;
}

float XGZP6897D::rawToTemperature(int16_t rawTemperature)
{
  return (float)rawTemperature / 256.0f;
}
