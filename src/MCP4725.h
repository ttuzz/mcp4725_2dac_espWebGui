#pragma once
//
//    FILE: MCP4725.h
//  AUTHOR: Rob Tillaart (Enhanced by ttuzz)
// PURPOSE: Arduino library for 12 bit I2C DAC - MCP4725
// VERSION: 0.4.0 (Enhanced)
//     URL: https://github.com/RobTillaart/MCP4725
//

#include "Wire.h"
#include "Arduino.h"

#define MCP4725_VERSION         (F("0.4.0"))

// constants
#define MCP4725_MAXVALUE        4095

// errors
#define MCP4725_OK              0
#define MCP4725_VALUE_ERROR     -999
#define MCP4725_REG_ERROR       -998
#define MCP4725_NOT_CONNECTED   -997
#define MCP4725_VOLTAGE_ERROR   -996

// Power Down Mode
#define MCP4725_PDMODE_NORMAL   0x00
#define MCP4725_PDMODE_1K       0x01
#define MCP4725_PDMODE_100K     0x02
#define MCP4725_PDMODE_500K     0x03

class MCP4725
{
  public:
    explicit MCP4725(const uint8_t deviceAddress, TwoWire *wire = &Wire);

    // Basic functions
    bool     begin();
    bool     isConnected();

    // DAC Value functions (0-4095)
    int      setValue(const uint16_t value = 0);
    uint16_t getValue();

    // Voltage functions (NEW!)
    int      setVoltage(float voltage, float vref = 3.3);
    float    getVoltage(float vref = 3.3);
    int      setMilliVolt(uint16_t milliVolt, float vref = 3.3);
    uint16_t getMilliVolt(float vref = 3.3);

    // EEPROM functions
    int      writeDAC(const uint16_t value, const bool EEPROM = false);
    bool     ready();
    uint32_t getLastWriteEEPROM() { return _lastWriteEEPROM; }
    uint16_t readDAC();
    uint16_t readEEPROM();

    // Power management
    int      writePowerDownMode(const uint8_t PDM, const bool EEPROM = false);
    uint8_t  readPowerDownModeEEPROM();
    uint8_t  readPowerDownModeDAC();
    int      powerOnReset();
    int      powerOnWakeUp();

    // Utility functions (NEW!)
    bool     isValidValue(uint16_t value) { return value <= MCP4725_MAXVALUE; }
    bool     isValidVoltage(float voltage, float vref = 3.3) { return (voltage >= 0 && voltage <= vref); }
    String   getErrorDescription(int errorCode);

    // Empedans kontrol fonksiyonları (NEW!)
    void     setLowImpedance() { writePowerDownMode(MCP4725_PDMODE_1K); }
    void     setMidImpedance() { writePowerDownMode(MCP4725_PDMODE_100K); }
    void     setHighImpedance() { writePowerDownMode(MCP4725_PDMODE_500K); }
    void     setNormalMode() { writePowerDownMode(MCP4725_PDMODE_NORMAL); }
    String   getImpedanceMode();

  private:
    uint8_t  _deviceAddress;
    uint16_t _lastValue;
    uint8_t  _powerDownMode;
    uint32_t _lastWriteEEPROM;
    TwoWire* _wire;

    // Internal functions
    int      _writeFastMode(const uint16_t value);
    int      _writeRegisterMode(const uint16_t value, uint8_t reg);
    uint8_t  _readRegister(uint8_t* buffer, const uint8_t length);
    int      _generalCall(const uint8_t gc);

    // Utility functions
    uint16_t _voltageToDAC(float voltage, float vref);
    float    _dacToVoltage(uint16_t dacValue, float vref);
};

// -- END OF FILE --
