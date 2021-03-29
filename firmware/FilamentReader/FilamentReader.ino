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

#include <Encoder.h>
Encoder filamentEncoder(2, 3);

// Diameter of the gear or wheel attached to the encoder, this can be
// tweaked as necessary to achieve accurate results.
float gearDiameter = 14.4615;

// Number of pulses produces by the encoder for one full rotation.
// This is normally mentioed in the spec sheet.
unsigned int encoderRotationCount = 2400;

// Calculate the encoder pulses required for 1mm of movement.
float encoderCountPerMM = encoderRotationCount / (gearDiameter * 3.14159);

// Polling intverval in ms
int interval = 100;

// State tracking
bool measureActive = false;
String serialData = "";
bool serialDataComplete = false;

// Used to track extruded amount
unsigned long currentMeasurement = 0;
unsigned long previousMeasurement = 0;

void setup() {
  // Open the serial port, use 115200 to allow ample bandwidth
  // for high sampling rates
  Serial.begin(115200);
  serialData.reserve(10);

  // Reset the encoder
  resetEncoder();
}

void loop() {
  // Capture the encoder reading prior to anything else
  currentMeasurement = filamentEncoder.read();
  float result = 0;

  // Handle incoming serial data
  if (serialDataComplete) {
    if (serialData.startsWith("B") || serialData.startsWith("b")) {
      resetEncoder();
      measureActive = true;
    } else if (serialData.startsWith("I") || serialData.startsWith("i")) {
      serialData = serialData.substring(1);
      if (serialData.toInt()) {
        interval = serialData.toInt();
      }
    } else if (serialData.startsWith("R") || serialData.startsWith("r")) {
      resetEncoder();
    } else if (serialData.startsWith("S") || serialData.startsWith("s")) {
      measureActive = false;
    }

    // Clear serial data and flag
    serialData = "";
    serialDataComplete = false;
  }

  // Run measurement if enabled
  // FIXME: Catch and handle the encoder rollover 0 > 4294967295
  if (measureActive) {
    if (currentMeasurement > previousMeasurement) {
      // We've extruded filament
      result = (currentMeasurement - previousMeasurement) / encoderCountPerMM;
    } else if (currentMeasurement < previousMeasurement) {
      // We've retracted filament
      result = 0 - ((previousMeasurement - currentMeasurement) / encoderCountPerMM);
    }
    
    // Output result to the serial console to 4 decimal accuracy
    Serial.print(result, 4);
    Serial.print(",");
    Serial.println(currentMeasurement / encoderCountPerMM);
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
