bool fuelGaugeTest = 1;
/*
Ignore warning avrdude: warning at C:\Users\Albert\AppData\Local\Arduino15\packages.......
byte = uint8_t / 16bit uint16_t
*/

#include "Arduino-MAX17263_Driver.h"
#include <Wire.h>
#include <Streaming.h>

MAX17263 max17263;

void initBatteryParameters()
{ max17263.rSense = 0.002;    
  max17263.designCap_mAh = 90000; // max 163000mAh 
  max17263.r100 = 0; // if NTC > 100k
  max17263.vChg = 0; // if charge voltage > 4.25V (4.3V–4.4V) 
  max17263.modelID = 6; // 0110 for LiFePO4
  // 0: for most lithium cobalt-oxide variants. Supported by EZ without characterization
  // 2: for lithium NCR or NCA cells such as Panasonic. Supported by EZ without characterization
  // 6: for LiFePO4, custom characterization is recommended, instead of an EZ configuration
  max17263.ichgTerm = 0x0640; // 250mA on 10mΩ, leave default 
  max17263.vEmpty = 3.3; // leave default 
}

void printFuelGaugeResults()
{ Serial << "\n\nFuelGaugeResults:";
  Serial << "\nCapacity: " << _FLOAT(max17263.getCapacity_mAh(), 1) << " mAH";
  Serial << "\nSOC: " << _FLOAT(max17263.getSOC(), 1) << " %";
  Serial << "\nVcell: " << _FLOAT(max17263.getVcell(), 2) << " V";
  Serial << "\nCurrent: " << _FLOAT(max17263.getCurrent(), 2) << " mA"; 
  Serial << "\nTTE Time to empty: " << _FLOAT(max17263.getTimeToEmpty(), 2) << " hours";
  Serial << "\nTemp: " << _FLOAT(max17263.getTemp(), 1) << " degree Celcius";
  Serial << "\nAvgVCell: " << _BIN(max17263.getAvgVCell());
  Serial << endl;
}

void setup() 
{ Wire.begin(); 
  Serial.begin(115200);
  delay(500); // just a small delay before first communications to MAX chip
  while(!Serial); // wait until serial port opens for native USB devices
  initBatteryParameters();
}

void loop() 
{ if(max17263.batteryPresent()) // without battery, status reading is 1111111111111111, so wait for Bst flag 1111111111110111 
  { if(max17263.powerOnResetEvent()) 
    { max17263.initialize(); // Step 0 and 3.2 periodically check reset and than initialize  
      //if(fuelGaugeTest) max17263.productionTest();
      // Step 3.6: if(historySaved) restoreHistory() Restoring Learned Parameters
    }  
    printFuelGaugeResults(); // Step 3.3 read the Fuel-Gauge Results 
    // Step 3.5 Save Learned Parameters every time bit 2 of the Cycles register toggles    
  }
  delay(2000);
}
