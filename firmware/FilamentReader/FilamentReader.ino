/* Filament Reader
 * 
 * Copyright (c) 2021 Simon Davie <nexx@nexxdesign.co.uk>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <EEPROM.h>
#include <Encoder.h>
Encoder filamentEncoder(2, 3);

// Constant vars
const char compile_date[] = __DATE__ " " __TIME__;
const float compile_version = 1.1;

// Diameter of the gear or wheel attached to the encoder, this can be
// tweaked as necessary to achieve accurate results.
float gearDiameter = 14.30;

// Number of pulses produces by the encoder for one full rotation.
// This is normally mentioed in the spec sheet.
const unsigned int encoderRotationCount = 2400;

// Used to store the calculated encoder per MM value.
float encoderCountPerMM = 0.00;

// Polling intverval in ms
int interval = 500;

// State tracking
bool measureActive = false;
String serialData = "";
bool serialDataComplete = false;

// Used to track extruded amount
unsigned long currentMeasurement = 0;
unsigned long previousMeasurement = 0;

// Mode selection
bool report_relative = false;

void setup() {
  // Open the serial port at 9600 baud.
  Serial.begin(9600);
  serialData.reserve(32);

  // Check the EEPROM for the calibrated value, use that if
  // it exists and is valid.
  float calEEPROM = 0.00;
  EEPROM.get(0, calEEPROM);
  if (!isnan(calEEPROM) && calEEPROM != 0) {
    gearDiameter = calEEPROM;
  }

  // Calculate the encoder pulses required for 1mm of movement.
  encoderCountPerMM = encoderRotationCount / (gearDiameter * PI);
  
  // Reset the encoder
  resetEncoder();
}

void loop() {
  // Capture the encoder reading prior to anything else
  currentMeasurement = filamentEncoder.read();
  float result = 0;

  // Handle incoming serial data
  if (serialDataComplete) {
    if (serialData.startsWith("INFO")) {
      // Output a welcome message
      Serial.print("NXE|");
      Serial.print(String(compile_version));
      Serial.print("|");
      Serial.print(String(compile_date));
      Serial.print("|");
      Serial.print(interval);
      Serial.print("|");
      Serial.println(gearDiameter, 6);
    }
    if (serialData.startsWith("ABS")) {
      report_relative = false;
    } else if (serialData.startsWith("REL")) {
      report_relative = true;
    } else if (serialData.startsWith("START")) {
      resetEncoder();
      measureActive = true;
    } else if (serialData.startsWith("INTERVAL")) {
      serialData = serialData.substring(8);
      if (serialData.toInt()) {
        interval = serialData.toInt();
      }
    } else if (serialData.startsWith("MEASURE")) {
      // Only reply if we're not actively measuring
      if (!measureActive) {
        Serial.println(currentMeasurement / encoderCountPerMM, 4);
      }
    } else if (serialData.startsWith("RESET")) {
      resetEncoder();
    } else if (serialData.startsWith("STOP")) {
      measureActive = false;
    } else if (serialData.startsWith("CAL")) {
      serialData = serialData.substring(3);
      if (serialData.toFloat()) {
        float calDistance = serialData.toFloat();
        float calDiameter = ((calDistance / currentMeasurement) * 2400) / PI;
        Serial.print("CALIBRATED DIAMETER: ");
        Serial.println(calDiameter, 6);

        // Save calibration to EEPROM
        EEPROM.put(0, calDiameter);
        float calEEPROM = 0.00f;
        EEPROM.get(0, calEEPROM);
        if (calEEPROM != calDiameter) {
          Serial.print("ERROR SAVING TO EEPROM. EXPECTED: ");
          Serial.print(calDiameter);
          Serial.print(" RETRIEVED: ");
          Serial.print(calEEPROM);
        } else {
          Serial.println("CALIBRATION SAVED TO EEPROM");
        }
        encoderCountPerMM = encoderRotationCount / (calDiameter * PI);
      }
    }

    // Clear serial data and flag
    serialData = "";
    serialDataComplete = false;
  }

  // Run measurement if enabled
  // FIXME: Catch and handle the encoder rollover 0 > 4294967295
  if (measureActive) {
    if (report_relative) {
      if (currentMeasurement > previousMeasurement) {
        // We've extruded filament
        result = (currentMeasurement - previousMeasurement) / encoderCountPerMM;
      } else if (currentMeasurement < previousMeasurement) {
        // We've retracted filament
        result = 0 - ((previousMeasurement - currentMeasurement) / encoderCountPerMM);
      }
    } else {
      result = (currentMeasurement / encoderCountPerMM);
    }
    
    // Output result to the serial console to 4 decimal accuracy
    Serial.println(result, 4);
    previousMeasurement = currentMeasurement;
    delay(interval);
  }
}

// Used to reset the encoder stats to 0
void resetEncoder() {
  // Reset the encoder to 0
  filamentEncoder.write(0);

  // Reset our tracking variables
  currentMeasurement = 0;
  previousMeasurement = 0;
}

// Handle incoming serial data
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    serialData += inChar;
    if (inChar == '\n') {
      serialDataComplete = true;
    }
  }
}
