/**
 * @file
 * @brief This example collects sensor data and posts it to NoteHub.
 * @copyright SPDX-License-Identifier: MIT
 * @author Alex Brudner, SparkFun Electronics
 * @date 2022-11-02
 * @note Based on Example01 from the MicroMod Environmental Function Board by 
 * Ho Yun "Bobby" Chan. This example code combines example codes from the 
 * SHTC3, STC31, and SGP40 libraries. Most of the steps to obtain the 
 * measurements are the same as the example code. Generic object names were 
 * renamed (e.g. mySensor => mySGP40 and mySTC3x).
 *
 *    Example 1: Basic Relative Humidity and Temperature Readings  w/ SHTC3
 *       Written by Owen Lyke
 *    Example 2: PHT (SHTC3) Compensated CO2 Readings w/ STC31
 *       Written by Paul Clark and based on earlier code by Nathan Seidle
 *    Example 1: Basic VOC Index w/ SGP40
 *       Written by Paul Clark
 *
 * Open a Serial Monitor at 115200 baud to view the readings too!
 *
 * Note: You may need to wait about ~5 minutes after starting up the code 
 * before VOC index
 * has any values.
 *
 * ========== HARDWARE CONNECTIONS ==========
 * MicroMod STM32 Processor Board => MicroMod Main Board => MicroMod 
 *  Environmental Function Board (with SHTC3, STC31, and SGP40)
 * MicroMod STM32 Processor Board => MicroMod Main Board => MicroMod 
 *  Function Board Blues Wireless Notecarrier
 *
 * Feel like supporting open source hardware?
 * Buy a board from SparkFun!
 *      MicroMod Function Board Blues Wireless Notecarrier  
 *        | !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!FILL IN WITH LINK
 *      MicroMod STM32 Processor                            
 *        | https://www.sparkfun.com/products/16401
 *      MicroMod Main Board - Double                        
 *        | https://www.sparkfun.com/products/18575
 *      MicroMod Environmental Function Board               
 *        | https://www.sparkfun.com/products/18632
 *
 * You can also get the sensors individually.
 *
 *      Qwiic SHTC3 | https://www.sparkfun.com/products/16467
 *      Qwiic STC31 | https://www.sparkfun.com/products/18385
 *      Qwiic SGP40 | https://www.sparkfun.com/products/17729
 */

/* Notecard Includes */
#include <Arduino.h>
#include <Wire.h>
#include <Notecard.h>

/* Sensor Includes */
#include <SparkFun_SHTC3.h>
#include <SparkFun_STC3x_Arduino_Library.h>
#include <SparkFun_SGP40_Arduino_Library.h>

#define PRODUCT_UID "com.your-company.your-name:your_product"

/* Instantiate Notecard class */
Notecard notecard;
int ncUpdateCtr = 0;

/* Instantiate sensor interface classes */
SHTC3 mySHTC3;
STC3x mySTC3x;
SGP40 mySGP40;

float rh = 0.0;           // Relative Humidity from the SHTC3
float temperature = 0.0;  // Temperature from the SHTC3
float co2 = 0.0;          // %CO2 from STC3
int32_t voc = 0;          // VOC index from SGP40

void setup() {
  /* Begin Serial communications */
  delay(2500);
  Serial.begin(115200);
  Serial.println(F("Begin - MicroMod Notecarrier Example 01: Sensor Data."));
  notecard.setDebugOutputStream(Serial);

  /* Begin I2C communications */
  Wire.begin();

  /* Initialize Notecard */
  notecard.begin();

  J *req = notecard.newRequest("hub.set");
  JAddStringToObject(req, "product", PRODUCT_UID);
  JAddStringToObject(req, "mode", "continuous");
  notecard.sendRequest(req);

  /* Find and initialize sensors */
  if (mySHTC3.begin() != SHTC3_Status_Nominal)
  {
    Serial.println(F("SHTC3 not detected. Please check wiring. Freezing..."));
    while (1)
      ; // Do nothing more
  }

  if (!mySTC3x.begin())
  {
    Serial.println(F("STC3x not detected. Please check wiring. Freezing..."));
    while (1)
      ; // Do nothing more
  }

  if (!mySGP40.begin())
  {
    Serial.println(F("SGP40 not detected. Check connections. Freezing..."));
    while (1)
      ; // Do nothing more
  }

  /**
   * We need to tell the STC3x what binary gas and full range we are using
   * Possible values are:
   * STC3X_BINARY_GAS_CO2_N2_100   : Set binary gas to CO2 in N2.  
   *    Range: 0 to 100 vol%
   * STC3X_BINARY_GAS_CO2_AIR_100  : Set binary gas to CO2 in Air. 
   *    Range: 0 to 100 vol%
   * STC3X_BINARY_GAS_CO2_N2_25    : Set binary gas to CO2 in N2.  
   *    Range: 0 to 25 vol%
   * STC3X_BINARY_GAS_CO2_AIR_25   : Set binary gas to CO2 in Air. 
   *    Range: 0 to 25 vol% 
   */
  if (!mySTC3x.setBinaryGas(STC3X_BINARY_GAS_CO2_AIR_25))
  {
    Serial.println(F("ST3x - Could not set the binary gas! Freezing..."));
    while (1)
      ; // Do nothing more
  }

  /**
   * Get an initial measurement so we can compensate for temperature and 
   * relative humidity using the readings from the SHTC3 
   */
  if (mySHTC3.update() != SHTC3_Status_Nominal) // Request a measurement
  {
    Serial.println(F("Could not read the RH and T from the SHTC3! \\
Freezing..."));
    while (1)
      ; // Do nothing more
  }

  /**
   * Configure STC31 temperature compensation based on previous measurement.
   * In case the ‘Set temperature command’ has been used prior to the above 
   * measurement command, this will set a new temperature compensation point.
   */
  temperature = mySHTC3.toDegC(); 
  Serial.print(F("Setting STC3x temperature to "));
  Serial.print(temperature, 2);
  Serial.print(F("C was "));
  if (!mySTC3x.setTemperature(temperature))
    Serial.print(F("not "));
  Serial.println(F("successful"));

  /**
   * Configure STC31 humidity compensation based on previous measurement.
   * In case the ‘Set humidity command’ has been used prior to the above 
   * measurement command, this will set a new humidity compensation point.
   */
  rh = mySHTC3.toPercent();
  Serial.print(F("Setting STC3x RH to "));
  Serial.print(rh, 2);
  Serial.print(F("% was "));
  if (!mySTC3x.setRelativeHumidity(rh))
    Serial.print(F("not "));
  Serial.println(F("successful"));

  /** 
   * If we have a pressure sensor available, we can compensate for ambient 
   * pressure too.
   * As an example, let's set the pressure to 840 mbar (== SF Headquarters) 
   */
  uint16_t pressure = 840;
  Serial.print(F("Setting STC3x pressure to "));
  Serial.print(pressure);
  Serial.print(F("mbar was "));
  if (mySTC3x.setPressure(pressure) == false)
    Serial.print(F("not "));
  Serial.println(F("successful"));

  Serial.println(F("Note: Relative humidity and temperature compensation for the STC31 will be updated frequently in the main loop() function."));

}

void loop() {

  //================================
  //==========READ SHTC3============
  //================================
  /* minimum update rate = ~100Hz */

  SHTC3_Status_TypeDef result = mySHTC3.update();           // Call "update()" to command a measurement, wait for measurement to complete, and update the RH and T members of the object

  rh = mySHTC3.toPercent();                                 // "toPercent" returns the percent humidity as a floating point number
  Serial.print(F("RelH: "));
  Serial.print(rh);
  Serial.println(F("%"));

  temperature = mySHTC3.toDegC();
  Serial.print(F("T: "));
  Serial.print(temperature);
  Serial.print(F(" deg C"));

  if (mySHTC3.lastStatus == SHTC3_Status_Nominal)           // You can also assess the status of the last command by checking the ".lastStatus" member of the object
  {
    Serial.println("");                                         //Sample data good, no need to output a message
  }
  else {
    Serial.print(F(",     Update failed, error: "));        //notify user if there is an error
    errorDecoder(mySHTC3.lastStatus);
    Serial.println("");
  }



  //==============================
  //==========READ STC31==========
  //==============================
  /* minimum update rate = 1Hz */


  if (!mySTC3x.setRelativeHumidity(rh))
    Serial.print(F("Unable to set STC31 Relative Humidity with SHTC3."));

  if (!mySTC3x.setTemperature(temperature))
    Serial.println(F("Unable to set STC31 Temperature with SHTC3."));


  Serial.print(F("CO2(%): "));

  if (mySTC3x.measureGasConcentration())                   // measureGasConcentration will return true when fresh data is available
  {
    co2 = mySTC3x.getCO2();
    Serial.println(co2, 2);
  }
  else
  {
    co2 = mySTC3x.getCO2();
    Serial.print(co2, 2);
    Serial.println(F(",     (old STC3 sample reading, STC31 was not able to get fresh data yet)"));  //output this note to indicate  when we are not able to obtain a new measurement
  }



  //==============================
  //==========READ SGP40==========
  //==============================
  /* minimum update rate = 1Hz */
  Serial.print(F("VOC: "));
  voc = mySGP40.getVOCindex(rh, temperature);
  Serial.println(voc); 

  /* Update the notecard about every 15s.*/
  if(++ncUpdateCtr >= 14)
  {
    ncUpdateCtr = 0;
    //==============================
    //=========UPLOAD DATA==========
    //==============================
    J *req = notecard.newRequest("note.add");
    if (req != NULL) {
      JAddStringToObject(req, "file", "sensors.qo");
      JAddBoolToObject(req, "sync", true);

      J *body = JCreateObject();
      if (body != NULL) {
        JAddNumberToObject(body, "temperature", temperature);
        JAddNumberToObject(body, "humidity", rh);
        JAddNumberToObject(body, "CO2%", co2);
        JAddNumberToObject(body, "VOC", voc);
        JAddItemToObject(req, "body", body);
      }

      notecard.sendRequest(req);
    }    
  }

  //================================
  //=========SPACE & DELAY==========
  //================================
  Serial.println("");// Uncomment this line to add some space between readings for the Serial Monitor
  delay(1000); //Wait 1 second - the Sensirion VOC and CO2 algorithms expects a sample rate of 1Hz

}

void errorDecoder(SHTC3_Status_TypeDef message)                             // The errorDecoder function prints "SHTC3_Status_TypeDef" results in a human-friendly way
{
  switch (message)
  {
    case SHTC3_Status_Nominal : Serial.print("Nominal"); break;
    case SHTC3_Status_Error : Serial.print("Error"); break;
    case SHTC3_Status_CRC_Fail : Serial.print("CRC Fail"); break;
    default : Serial.print("Unknown return code"); break;
  }
}
