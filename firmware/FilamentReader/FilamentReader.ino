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
float gearDiameter = 10.9495;

// Number of pulses produces by the encoder for one full rotation.
// This is normally mentioed in the spec sheet.
unsigned int encoderRotationCount = 2400;

// Calculate the encoder pulses required for 1mm of movement.
float encoderCountPerMM = encoderRotationCount / (gearDiameter * 3.14159);

// Polling rate in milliseconds
unsigned int pollingRate = 1000;

// Used to track timing and stats
unsigned long startTime;
unsigned long previousTime = 0;
unsigned long previousMeasurement = 0;
bool runningMeasurement = false;

void setup() {
  // Open the serial port 9600bps is fast enough
  Serial.begin(9600);

  // Reset the encoder
  filamentEncoder.write(0);
}

void loop() {
  // Wait for movement
  if (!runningMeasurement) {
    while (filamentEncoder.read() == 0) {
      delay(10);
      startTime = millis();
    }
    runningMeasurement = true;
    previousMeasurement = 0;
  } else {
    // Record the current time
    unsigned long currentTime = millis();

    // Run measurement if polling rate has elapsed
    if (currentTime - previousTime >= pollingRate) {
      unsigned long currentMeasurement = filamentEncoder.read();

      if (currentMeasurement != previousMeasurement) {
        Serial.print(currentTime - startTime);
        Serial.print(",");
        Serial.print((currentMeasurement - previousMeasurement) / encoderCountPerMM);
        Serial.print(",");
        Serial.println(currentMeasurement / encoderCountPerMM);
        
        // Update our timestamp
        previousMeasurement = currentMeasurement;
        previousTime = currentTime;
      } else {
        // Reset everything and wait
        previousTime = 0;
        previousMeasurement = 0;
        runningMeasurement = false;
        Serial.println();

        // Sleep before resetting the encoder to avoid remeasurement whilst settling
        delay(4000);
        filamentEncoder.write(0);
      }
    }
  }
}
