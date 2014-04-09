/**
 * StenoFW is a firmware for Stenoboard keyboards.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2014 Emanuele Caruso. See LICENSE.txt for details.
 */
 
#define ROWS 5
#define COLS 6

/* The following matrix is shown here for reference only.
char keys[ROWS][COLS] = {
  {'S', 'T', 'P', 'H', '*', Fn1},
  {'S', 'K', 'W', 'R', '*', Fn2},
  {'a', 'o', 'e', 'u', '#'},
  {'f', 'p', 'l', 't', 'd'},
  {'r', 'b', 'g', 's', 'z'}
};*/

// Configuration variables
int rowPins[ROWS] = {13, 12, 11, 10, 9};
int colPins[COLS] = {8, 7, 6, 5, 4, 2};
int ledPin = 3;
long debounceMillis = 20;

// Keyboard state variables
boolean isStrokeInProgress = false;
boolean currentChord[ROWS][COLS];
boolean currentKeyReadings[ROWS][COLS];
boolean debouncingKeys[ROWS][COLS];
unsigned long debouncingMicros[ROWS][COLS];

// Other state variables
int ledIntensity = 1; // Min 0 - Max 255

// This is called when the keyboard is connected
void setup() {
  Serial.begin(9600);
  for (int i = 0; i < COLS; i++)
    pinMode(colPins[i], INPUT_PULLUP);
  for (int i = 0; i < ROWS; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH);
  }
  pinMode(ledPin, OUTPUT);
  analogWrite(ledPin, ledIntensity);
  clearBooleanMatrixes();
}

// Read key states and handle all chord events
void loop() {
  readKeys();
  
  boolean isAnyKeyPressed = true;
  
  // If stroke is not in progress, check debouncing keys
  if (!isStrokeInProgress) {
    checkAlreadyDebouncingKeys();
    if (!isStrokeInProgress) checkNewDebouncingKeys();
  }
  
  // If any key was pressed, record all pressed keys
  if (isStrokeInProgress) {
    isAnyKeyPressed = recordCurrentKeys();
  }
  
  // If all keys have been released, send the chord and reset global state
  if (!isAnyKeyPressed) {
    sendChord();
    clearBooleanMatrixes();
    isStrokeInProgress = false;
  }
}

// Record all pressed keys into current chord. Return false if no key is currently pressed
boolean recordCurrentKeys() {
  boolean isAnyKeyPressed = false;
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      if (currentKeyReadings[i][j] == true) {
        currentChord[i][j] = true;
        isAnyKeyPressed = true;
      }
    }
  }
  return isAnyKeyPressed;
}

// If a key is pressed, add it to debouncing keys and record the time
void checkNewDebouncingKeys() {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      if (currentKeyReadings[i][j] == true && debouncingKeys[i][j] == false) {
        debouncingKeys[i][j] = true;
        debouncingMicros[i][j] = micros();
      }
    }
  }
}

// Check already debouncing keys. If a key debounces, start chord recording.
void checkAlreadyDebouncingKeys() {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      if (debouncingKeys[i][j] == true && currentKeyReadings[i][j] == false) {
        debouncingKeys[i][j] = false;
        continue;
      }
      if (debouncingKeys[i][j] == true && micros() - debouncingMicros[i][j] / 1000 > debounceMillis) {
        isStrokeInProgress = true;
        currentChord[i][j] = true;
        return;
      }
    }
  }
}

// Set all values of all boolean matrixes to false
void clearBooleanMatrixes() {
  clearBooleanMatrix(currentChord, false);
  clearBooleanMatrix(currentKeyReadings, false);
  clearBooleanMatrix(debouncingKeys, false);
}

// Set all values of the passed matrix to the given value
void clearBooleanMatrix(boolean booleanMatrix[][COLS], boolean value) {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      booleanMatrix[i][j] = value;
    }
  }
}

// Read all keys
void readKeys() {
  for (int i = 0; i < ROWS; i++) {
    digitalWrite(rowPins[i], LOW);
    for (int j = 0; j < COLS; j++)
      currentKeyReadings[i][j] = digitalRead(colPins[j]) == LOW ? true : false;
    digitalWrite(rowPins[i], HIGH);
  }
}

// Send current chord over serial using the Gemini protocol. If there are fn keys
// pressed, delegate to the corresponding function instead.
// In future versions, there should also be a way to handle fn keys presses before
// they are released, eg. for mouse emulation functionality or custom key presses.
void sendChord() {
  // Initialize chord bytes
  byte chordBytes[] = {B10000000, B0, B0, B0, B0, B0};
  
  // If fn keys have been pressed, delegate to corresponding method and return
  if (currentChord[0][5] && currentChord[1][5]) {
    fn1fn2();
    return;
  } else if (currentChord[0][5]) {
    fn1();
    return;
  } else if (currentChord[1][5]) {
    fn2();
    return;
  }
  
  // Byte 0
  if (currentChord[2][4]) {
    chordBytes[0] = B10000001;
  }
  
  // Byte 1
  if (currentChord[0][0] || currentChord[1][0]) {
    chordBytes[1] += B01000000;
  }
  if (currentChord[0][1]) {
    chordBytes[1] += B00010000;
  }
  if (currentChord[1][1]) {
    chordBytes[1] += B00001000;
  }
  if (currentChord[0][2]) {
    chordBytes[1] += B00000100;
  }
  if (currentChord[1][2]) {
    chordBytes[1] += B00000010;
  }
  if (currentChord[0][3]) {
    chordBytes[1] += B00000001;
  }
  
  // Byte 2
  if (currentChord[1][3]) {
    chordBytes[2] += B01000000;
  }
  if (currentChord[2][0]) {
    chordBytes[2] += B00100000;
  }
  if (currentChord[2][1]) {
    chordBytes[2] += B00010000;
  }
  if (currentChord[0][4] || currentChord[1][4]) {
    chordBytes[2] += B00001000;
  }
  
  // Byte 3
  if (currentChord[2][2]) {
    chordBytes[3] += B00001000;
  }
  if (currentChord[2][3]) {
    chordBytes[3] += B00000100;
  }
  if (currentChord[3][0]) {
    chordBytes[3] += B00000010;
  }
  if (currentChord[4][0]) {
    chordBytes[3] += B00000001;
  }
  
  // Byte 4
  if (currentChord[3][1]) {
    chordBytes[4] += B01000000;
  }
  if (currentChord[4][1]) {
    chordBytes[4] += B00100000;
  }
  if (currentChord[3][2]) {
    chordBytes[4] += B00010000;
  }
  if (currentChord[4][2]) {
    chordBytes[4] += B00001000;
  }
  if (currentChord[3][3]) {
    chordBytes[4] += B00000100;
  }
  if (currentChord[4][3]) {
    chordBytes[4] += B00000010;
  }
  if (currentChord[3][4]) {
    chordBytes[4] += B00000001;
  }

  // Byte 5
  if (currentChord[4][4]) {
    chordBytes[5] += B00000001;
  }

  // Send chord bytes over serial
  for (int i = 0; i < 6; i++) {
    Serial.write(chordBytes[i]);
  }
}

// This function is called when only "fn1" key has been pressed.
void fn1() {
  
}

// This function is called when only "fn2" key has been pressed.
void fn2() {
  
}

// This function is called when both "fn1" and "fn1" key has been pressed.
void fn1fn2() {
  // "HR" -> Change LED intensity
  if (currentChord[0][3] && currentChord[1][3]) {
    // "P" -> LED intensity up
    if (currentChord[3][1]) {
      if (ledIntensity == 0) ledIntensity +=1;
      else if(ledIntensity < 50) ledIntensity += 10;
      else ledIntensity += 30;
      if (ledIntensity > 255) ledIntensity = 0;
      analogWrite(ledPin, ledIntensity);
    }
    // "F" -> LED intensity down
    if (currentChord[3][0]) {
      if(ledIntensity == 0) ledIntensity = 255;
      else if(ledIntensity < 50) ledIntensity -= 10;
      else ledIntensity -= 30;
      if (ledIntensity < 1) ledIntensity = 0;
      analogWrite(ledPin, ledIntensity);
    }
  }
}
