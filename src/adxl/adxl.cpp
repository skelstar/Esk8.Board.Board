/*  ********************************************* 
 *  SparkFun_ADXL345_Example
 *  Triple Axis Accelerometer Breakout - ADXL345 
 *  Hook Up Guide Example 
 *  
 *  Utilizing Sparkfun's ADXL345 Library
 *  Bildr ADXL345 source file modified to support 
 *  both I2C and SPI Communication
 *  
 *  E.Robert @ SparkFun Electronics
 *  Created: Jul 13, 2016
 *  Updated: Sep 06, 2016
 *  
 *  Development Environment Specifics:
 *  Arduino 1.6.11
 *  
 *  Hardware Specifications:
 *  SparkFun ADXL345
 *  Arduino Uno
 *  *********************************************/

#include <SparkFun_ADXL345.h> // SparkFun ADXL345 Library

/*********** COMMUNICATION SELECTION ***********/
/*    Comment Out The One You Are Not Using    */
ADXL345 adxl = ADXL345(); // USE FOR SPI COMMUNICATION, ADXL345(CS_PIN);
//ADXL345 adxl = ADXL345();             // USE FOR I2C COMMUNICATION

/****************** INTERRUPT ******************/
/*      Uncomment If Attaching Interrupt       */
//int interruptPin = 2;                 // Setup pin 2 to be the interrupt pin (for most Arduino Boards)

void ADXL_ISR();

/******************** SETUP ********************/
/*          Configure ADXL345 Settings         */
void setup()
{

  Serial.begin(115200); // Start the serial terminal
  Serial.println("SparkFun ADXL345 Accelerometer Hook Up Guide Example");
  Serial.println();

  adxl.powerOn(); // Power on the ADXL345

  adxl.setRangeSetting(8); // Give the range settings
                           // Accepted values are 2g, 4g, 8g or 16g
                           // Higher Values = Wider Measurement Range
                           // Lower Values = Greater Sensitivity

  adxl.setActivityXYZ(1, 0, 0);  // Set to activate movement detection in the axes "adxl.setActivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
  adxl.setActivityThreshold(75); // 62.5mg per increment   // Set activity   // Inactivity thresholds (0-255)

  adxl.setInactivityXYZ(1, 0, 0);  // Set to detect inactivity in all the axes "adxl.setInactivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
  adxl.setInactivityThreshold(75); // 62.5mg per increment   // Set inactivity // Inactivity thresholds (0-255)
  adxl.setTimeInactivity(10);      // How many seconds of no activity is inactive?

  adxl.setTapDetectionOnXYZ(1, 0, 0); // Detect taps in the directions turned ON "adxl.setTapDetectionOnX(X, Y, Z);" (1 == ON, 0 == OFF)

  // Set values for what is considered a TAP and what is a DOUBLE TAP (0-255)
  adxl.setTapThreshold(50);     // 62.5 mg per increment
  adxl.setTapDuration(15);      // 625 μs per increment
  adxl.setDoubleTapLatency(80); // 1.25 ms per increment
  adxl.setDoubleTapWindow(200); // 1.25 ms per increment

  // Set values for what is considered FREE FALL (0-255)
  adxl.setFreeFallThreshold(7); // (5 - 9) recommended - 62.5mg per increment
  adxl.setFreeFallDuration(30); // (20 - 70) recommended - 5ms per increment

  // Setting all interupts to take place on INT1 pin
  //adxl.setImportantInterruptMapping(1, 1, 1, 1, 1);     // Sets "adxl.setEveryInterruptMapping(single tap, double tap, free fall, activity, inactivity);"
  // Accepts only 1 or 2 values for pins INT1 and INT2. This chooses the pin on the ADXL345 to use for Interrupts.
  // This library may have a problem using INT2 pin. Default to INT1 pin.

  // Turn on Interrupts for each mode (1 == ON, 0 == OFF)
  adxl.InactivityINT(1);
  adxl.ActivityINT(1);
  adxl.FreeFallINT(1);
  adxl.doubleTapINT(1);
  adxl.singleTapINT(1);

  //attachInterrupt(digitalPinToInterrupt(interruptPin), ADXL_ISR, RISING);   // Attach Interrupt
}

/****************** MAIN CODE ******************/
/*     Accelerometer Readings and Interrupt    */

#include <elapsedMillis.h>

elapsedMillis sinceRead;

int max_y = 220, min_y = 220, start_y = -1;

void loop()
{
  if (sinceRead > 200)
  {
    sinceRead = 0;

    // Accelerometer Readings
    int x, y, z;

    adxl.readAccel(&x, &y, &z); // Read the accelerometer values and store them in variables declared above x,y,z

    if (start_y == -1)
    {
      min_y = y;
      max_y = y;
      start_y = y;
    }
    max_y = max_y < y ? y : max_y;
    min_y = min_y > y ? y : min_y;

    // Output Results to Serial
    /* UNCOMMENT TO VIEW X Y Z ACCELEROMETER VALUES */

    Serial.printf("x=%04d y=%04d z=%04d |  min=%04d start=%04d max=%04d  |  %s \n",
                  x, y, z, min_y, start_y, max_y,
                  y > 80 ? "UP" : "FLAT");

    ADXL_ISR();
  }

  vTaskDelay(10);

  // You may also choose to avoid using interrupts and simply run the functions within ADXL_ISR();
  //  and place it within the loop instead.
  // This may come in handy when it doesn't matter when the action occurs.
}

/********************* ISR *********************/
/* Look for Interrupts and Triggered Action    */
void ADXL_ISR()
{

  // getInterruptSource clears all triggered actions after returning value
  // Do not call again until you need to recheck for triggered actions
  byte interrupts = adxl.getInterruptSource();

  // Free Fall Detection
  if (adxl.triggered(interrupts, ADXL345_FREE_FALL))
  {
    Serial.println("*** FREE FALL ***");
    //add code here to do when free fall is sensed
  }

  // Inactivity
  if (adxl.triggered(interrupts, ADXL345_INACTIVITY))
  {
    Serial.println("*** INACTIVITY ***");
    //add code here to do when inactivity is sensed
  }

  // Activity
  if (adxl.triggered(interrupts, ADXL345_ACTIVITY))
  {
    Serial.println("*** ACTIVITY ***");
    //add code here to do when activity is sensed
  }

  // Double Tap Detection
  if (adxl.triggered(interrupts, ADXL345_DOUBLE_TAP))
  {
    Serial.println("*** DOUBLE TAP ***");
    //add code here to do when a 2X tap is sensed
  }

  // Tap Detection
  if (adxl.triggered(interrupts, ADXL345_SINGLE_TAP))
  {
    Serial.println("*** TAP ***");
    //add code here to do when a tap is sensed
  }
}
