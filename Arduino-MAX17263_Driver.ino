/*
MIT License 

Documents:
UG6597 MAX1726x ModelGauge m5 EZ User Guide 48p 
UG6595 MAX1726x Software Implementation Guide 15p
MAX17263 Single/Multi-Cell Fuel Gauge with ModelGauge m5 EZ and Integrated LED Control
*/

#include "Arduino-MAX17263_Driver.h"
#include <Wire.h>
#include <Albert.h>

// Public Methods

bool MAX17263::batteryPresent()
{ bool bst = getStatus() & 0x0008; // check battery status Bst flag = 0, UG6597 page 33 
  Serial << F("\nBattery present: ") << !bst << " status bin: " << _BIN(getStatus()) << " " << millis();
  return(!bst); 
}

bool MAX17263::powerOnResetEvent()
{ bool por = getStatus() & 0x0002; // check POR bit
  Serial << F("\nPower On Reset event: ") << por << " " << millis();
  return(por);  
}

void MAX17263::initialize()
{ waitForDNRdataNotReady(); // Step 1
  storeHibernateCFG();
  exitHibernate(); // Step 2: Initialize Configuration 
  setEZconfig(); // Step 2.1 EZ Config (No INI file is needed):
  restoreHibernateCFG();
  clearPORpowerOnReset(); // Step 3: Initialization Complete 
  setLEDCfg1();
  setLEDCfg2();
}

void MAX17263::setEZconfig()
{ calcMultipliers(rSense);    
  setDesignCap_mAh(designCap_mAh); // max 163000mAh 
  refreshModelCFG(r100, vChg, modelID);
  waitforModelCFGrefreshReady();
  /* setIchgTerm(ichgTerm); // leave default */
  /* setVEmpty(vEmpty); // leave default */
}

float MAX17263::getCapacity_mAh()
{ uint16_t capacity_raw = readReg16Bit(regRepCap);
	return (capacity_raw * capacity_multiplier_mAH);
}

float MAX17263::getCurrent()
{ int16_t current_raw = readReg16Bit(regCurrent);
	return current_raw * current_multiplier_mV;
}

float MAX17263::getVcell()
{ uint16_t voltage_raw = readReg16Bit(regVCell);
	return voltage_raw * voltage_multiplier_V;
}

float MAX17263::getSOC() // at new battery: SOC = 0%, after 10min. 256.0% than 99% 
{ uint16_t SOC_raw = readReg16Bit(regRepSOC);
	return SOC_raw * SOC_multiplier;
}

float MAX17263::getTimeToEmpty()
{ uint16_t TTE_raw = readReg16Bit(regTimeToEmpty);
	return TTE_raw * time_multiplier_Hours;
}

float MAX17263::getTemp()
{ return readReg16Bit(regTemp)/256.0; 
}

float MAX17263::getAvgVCell()
{ return readReg16Bit(regAvgVCell) * voltage_multiplier_V;
}

// Private Methods

uint16_t MAX17263::getStatus() // MAX1726x-ModelGauge-m5-EZ-user-guide
{ return readReg16Bit(regStatus); 
}

void MAX17263::writeReg16Bit(byte reg, uint16_t value)
{ Wire.beginTransmission(I2CAddress); // write LSB first, refer to AN635 pg 35 figure 
  Wire.write(reg);
  Wire.write( value       & 0xFF); // value low byte
  Wire.write((value >> 8) & 0xFF); // value high byte
  byte last_status = Wire.endTransmission();
}

uint16_t MAX17263::readReg16Bit(byte reg)
{ uint16_t value = 0;  
  Wire.beginTransmission(I2CAddress); 
  Wire.write(reg);
  byte last_status = Wire.endTransmission(false);
  
  Wire.requestFrom(I2CAddress, (byte)2); 
  value  = Wire.read();
  value |= (uint16_t)Wire.read() << 8; // value low byte
  return value;
}

bool MAX17263::waitForDNRdataNotReady()
{ Serial << "\nWait for Data Not Ready: " << (bool)(readReg16Bit(regFStat) & 0x1) << " " << millis();
  while(readReg16Bit(regFStat) & 0x1) delay(10); // FSTAT.DNR bit 
  Serial << "\nData Not Ready: " << (bool)(readReg16Bit(regFStat) & 0x1) << " " << millis(); 
}

void MAX17263::calcMultipliers(float rSense) 
{ capacity_multiplier_mAH = (5e-3)/rSense; // UG6595 page 4, 2.5 at 2mOhm, 
  current_multiplier_mV = (1.5625e-3)/rSense; // UG6595 page 4 
  Serial << "\ncapacity_multiplier_mAH " << _FLOAT(capacity_multiplier_mAH, 4);
  Serial << "\ncurrent_multiplier_mV " << _FLOAT(current_multiplier_mV, 4);
}

void MAX17263::setDesignCap_mAh(long c) // max 65,535*2,5 = 163000mAh, uint16_t was wrong
{ writeReg16Bit(regDesignCap, c/capacity_multiplier_mAH); // was batteryCapacity*2
  Serial << "\nSet designCap " << readReg16Bit(regDesignCap) * capacity_multiplier_mAH;
}  

void MAX17263::setIchgTerm(uint16_t i)
{ writeReg16Bit(regIchgTerm, i); // todo not tested
  Serial << "\nSet IchgTerm " << _HEX(readReg16Bit(regIchgTerm));
}

void MAX17263::setVEmpty(float vf) 
{ Serial << "\nVEmpty old " << _BIN(readReg16Bit(regVEmpty)); // default 0xA561 (3.3V/3.88V)
  uint16_t ve = vf*100;
  uint16_t val = readReg16Bit(regVEmpty);
  val &= 0x7F; // 0000.0000.0111.1111 leave VR page 16/37
  val |= ve << 7; // 3.3V = 330 = 101001010
  writeReg16Bit(regVEmpty, val); 
  Serial << "\nVEmpty new " << _BIN(readReg16Bit(regVEmpty));
}

void MAX17263::refreshModelCFG(bool r100, bool vChg, byte modelID)
{ uint16_t cfgVal = 0;
  cfgVal |=  modelID << 4;
  cfgVal |=  vChg << 10; 
  cfgVal |=  r100 << 13;
  cfgVal |=  0x8000; // set refresh bit to 1 for model refresh
  writeReg16Bit(regModelCfg, cfgVal); 
  Serial << "\nRefresh modelCFG " << _BIN(readReg16Bit(regModelCfg));
}

bool MAX17263::waitforModelCFGrefreshReady() 
{ Serial << "\nWait for ModelCFG ready: " << (bool)(readReg16Bit(regModelCfg) & 0x8000) << " " << millis(); 
  while(readReg16Bit(regModelCfg) & 0x8000) delay(10); // after ModelCFG reload the MAX1726x clears Refresh bit to 0 
  Serial << "\nModelCFG ready: " << (bool)(readReg16Bit(regModelCfg) & 0x8000) << " " << millis(); 
}

void MAX17263::exitHibernate() // UG6595 page 7
{ writeReg16Bit(0x60, 0x90); // Exit Hibernate Mode step 1
  writeReg16Bit(regHibCfg, 0x0); // Exit Hibernate Mode step 2
  writeReg16Bit(0x60, 0x0); // Exit Hibernate Mode step 3
}

void MAX17263::storeHibernateCFG()
{ originalHibernateCFG = readReg16Bit(regHibCfg); // store original HibCFG value
  Serial << "\nStore HibCfg: " << _HEX(originalHibernateCFG); 
} 

void MAX17263::restoreHibernateCFG()
{ writeReg16Bit(regHibCfg, originalHibernateCFG); // Restore original HibCFG value
  Serial << "\nRestore HibCfg: " << _HEX(originalHibernateCFG); 
} 

void MAX17263::clearPORpowerOnReset()
{ uint16_t val = readReg16Bit(regStatus); // read status 
  writeReg16Bit(regStatus, val & 0xFFFD); // clear POR bit 1111111111111101   
  Serial << "\nClear POR power on reset: " << _HEX(val & 0xFFFD);  
}

void MAX17263::setLEDCfg1() 
{ uint16_t val = readReg16Bit(regLedCfg1); // default 0x6070 page 29/37
  Serial << "\nLEDCfg1 old: " << _HEX(val);  
  const byte LEDTimer = 2; // 0.6s because of T1 overheath
  const bool LChg = 0; // default=1, no LEDs on while charging because of T1 overheath
  const byte NBARS = 10; // using larger LED resistors may not achieve correct auto-count upon start up
  val &= 0x1FD0; // 0001.1111.1101.0000, set variables to 0
  val = val | LEDTimer<<13 | LChg<<5 | NBARS;
  writeReg16Bit(regLedCfg1, val);  
  Serial << "\nLEDCfg1 new: " << _HEX(val);  
}

void MAX17263::setLEDCfg2() 
{ uint16_t val = readReg16Bit(regLedCfg2); // default 0x011f page 30/37
  Serial << "\nLEDCfg2 old: " << _HEX(val);  
  const bool EnAutoLEDCnt = 0; // 1->0 using larger LED resistors may not achieve correct auto-count upon start up
  const bool EBlink = 1; // 0->1 blink lowest LED when empty is detected
  const byte Brightness = 31; // max 31
  val &= 0xFEA0; // 1111.1110.1010.0000, set variables to 0
  val = val | EnAutoLEDCnt<<8 | EBlink<<6 | Brightness;
  writeReg16Bit(regLedCfg2, val);  
  Serial << "\nLEDCfg2 new: " << _HEX(val);  
}

void MAX17263::productionTest() // use UG6365 MAX17055 Software Implementation Guide (G6595 MAX1726x page 12 is WRONG)
{ Serial << "\nProduction Test "; 
  uint16_t val1, val2; //kan anders
// Step T2. Verify there are no memory leaks during Quickstart writing 
  do
  { val1 = readReg16Bit(regMiscCfg); // Step T1. Set the Quickstart and Verify bits
    writeReg16Bit(regMiscCfg, val1 | 0x1400); // Set bits 10 and 12
  
    val2 = readReg16Bit(regMiscCfg); // Step T2. Verify there are no memory leaks during Quickstart writing 
    val2 &= 0x1000; 
  } while(!val2==0x1000); // Quickstart Success

 //Step T2: Wait for Quick Start to Complete
//Poll MiscCFG.QS(0x0400) and FSTAT.DNR until they becomes 0 to confirm Quickstart is finished
 // while(readReg16Bit(regMiscCfg) & 0x0400 and ReadRegister(regFStat) & 1) delay(10)； 

 //do not continue quickstart is complete
//Step T3: Read and Verify Outputs
//The RepCap and RepSOC register locations should now contain accurate fuel-gauge results based on a battery voltage of 3.900V. The RepSOC (as a percent) is read from memory location 0x06h and the RepCap (in mAHrs) is read from memory location 0x05h.
//RepCap = ReadRegister(0x05) ; //Read RepCap
//RepSOC = ReadRegister(0x06) ; //Read RepSOC

}
