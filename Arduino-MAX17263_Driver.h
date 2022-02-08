/*
MIT License terugzetten
*/

#ifndef Arduino-MAX17263_Driver.h_h
#define Arduino-MAX17263_Driver.h_h

#include <Arduino.h>

class MAX17263
{
public:  
  const byte regStatus      = 0x00; // UG6597 page 32 Flags related to alert thresholds and battery insertion or removal
  const byte regModelCfg    = 0xdb; // UG6597 page 29 Basic options of the EZ algorithm.
  const byte regVCell       = 0x09; // VCell reports the voltage measured between BATT and CSP
  const byte regAvgVCell    = 0x19; // The AvgVCell register reports an average of the VCell register readings. 
  const byte regCurrent     = 0x0A; // Voltage between the CSP and CSN pins, and would need to convert to current
  const byte regAvgCurrent  = 0x0B; // The AvgCurrent register reports an average of Current register readings
  const byte regRepSOC      = 0x06; // The Reported State of Charge of connected battery. 
  const byte regTimeToEmpty = 0x11; // How long before battery is empty (in ms).  
  const byte regRepCap      = 0x05; // Reported Capacity. 
  const byte regDesignCap   = 0x18; // Capacity of battery inserted, not typically used for user requested capacity
  const byte regTemp        = 0x08; // Temperature
  const byte regFStat       = 0x3D; // Status of the ModelGauge m5 algorithm
  const byte regIchgTerm    = 0x1E; // Charge termination current default 0x0640 (250mA on 10mΩ) UG6597 page 29
  const byte regVEmpty      = 0x3A; // 9bit, Empty voltage target, during load, 0...5.11V, default 3.3V UG6597 page 28
  const byte regHibCfg      = 0xBA; // hibernate mode functionality UG6597 page 41
  const byte regLedCfg1     = 0x40;
  const byte regLedCfg2     = 0x4B;
  const byte regMiscCfg     = 0x2B; // enables various other functions UG6597 page 36
  const byte regLedCfg3     = 0x37; // not used
  const byte regCustLED     = 0x64; // not used 

  bool batteryPresent();
  bool powerOnResetEvent();
  void initialize();
  void productionTest();
  float getCurrent();
  float getVcell();
  float getCapacity_mAh();
  float getSOC(); // SOC = State of Charge 
  float getTimeToEmpty();
  float getTemp(); 
  float getAvgVCell(); 

  byte modelID; 
  bool refresh, r100, vChg; 
  float rSense, vEmpty;
  long designCap_mAh;
  uint16_t ichgTerm;
  
private:
  const byte I2CAddress = 0x36;
  uint16_t originalHibernateCFG;
   
  uint16_t getStatus(); 
  float capacity_multiplier_mAH; // depends on rSense
  float current_multiplier_mV; // depends on rSense
  const float voltage_multiplier_V = 7.8125e-5; // UG6595 page 4
  const float time_multiplier_Hours = 5.625/3600.0; // UG6595 page 10, lsb = 5.625 seconds
  const float SOC_multiplier = 1.0/256.0; // UG6595 page 4

  bool waitForDNRdataNotReady();
  void clearPORpowerOnReset();
  void calcMultipliers(float rSense); 
  void setDesignCap_mAh(long c); 
  void setIchgTerm(uint16_t i);
  void setVEmpty(float vf);
  void refreshModelCFG(bool r100, bool vChg, byte modelID);
  bool waitforModelCFGrefreshReady();
  void setEZconfig();
  void exitHibernate();
  void storeHibernateCFG();
  void restoreHibernateCFG();
  void setLEDCfg1();
  void setLEDCfg2();
  uint16_t readReg16Bit(byte reg);
  void writeReg16Bit(byte reg, uint16_t value);
};

#endif
