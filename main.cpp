/*
 * =========================================================================================
 * PROJECT: Dual-Memory Autonomous Maze Solver (Pure Topological + Turbo Speed Run)
 * =========================================================================================
 * * CHANGELOG & BUGFIXES:
 * - Removed Memory 3 [SOLVED]: DFS and Slot 3 have been completely purged for simplicity.
 * - Dual-Personality Mapping: Slot 1 is Pure RHR. Slot 2 is Pure LHR.
 * - Absolute Compass [RETAINED]: Memory uses global headings (0=N, 1=E, 2=S, 3=W).
 * - High-Speed Clearances: Includes all refined align-pushes and escape kicks.
 * =========================================================================================
 */

//Including Libraries
#include <Arduino.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// --- PINS ---
const int pFL = A5, pNL = A4, pM = A3, pNR = A2, pFR = A1; 
const int M1_DIR = 2, M1_PWM = 3, M2_PWM = 11, M2_DIR = 12, ledPin = 13; 

// --- TUNING & TIMING ---
float Kp = 25.0, Kd = 15.0; 

int mapBaseSpeed = 120;        
int mapTurnSpeed = 85;         
int mapAlignPush = 700; 

int speedRunStraightSpeed = 200; 
int speedRunBaseSpeed = 140;     
int speedRunTurnSpeed = 85;    
int speedRunAlignPush = 350;     

int baseSpeed = 120;        
int turnSpeed = 85;        
int brakeDur = 80;       

const int deadEndTimeout = 500; 

// --- ABSOLUTE COMPASS STACK ---
byte currentHeading = 0; 
int mapStarts[] = {50, 250}; // Adjusted for 2 slots
unsigned long runTimes[2];      
byte currentPath[150];
int pathLen = 0, pathIndex = 0, currentSlot = 0;
byte runMode = 0; 
unsigned long startTime = 0, whiteTime = 0;
float error = 0, lastError = 0;

// --- PROTOTYPES ---
void motorControl(int l, int r);
void followLinePID();
void processJunction(bool isDead);
void turn90(bool right);
void turn180();
void simplifyPathAbsolute();
int readButtons();
void startSession(int slot, String msg);
void startSpeedRun();
void escapeStartBox(); 

void setup() {
  Serial.begin(9600); 
  lcd.begin(16, 2);

  pinMode(M1_DIR, OUTPUT); pinMode(M1_PWM, OUTPUT); 
  pinMode(M2_PWM, OUTPUT); pinMode(M2_DIR, OUTPUT);
  pinMode(ledPin, OUTPUT);
  
  for(int i=0; i<2; i++) { 
    EEPROM.get(i*4 + 4, runTimes[i]); 
    if (runTimes[i] == 4294967295UL) runTimes[i] = 0; 
  }
  lcd.clear(); lcd.print("Ready (^=^)");
}

void loop() {
  if (runMode == 0) {
    int btn = readButtons();
    if (btn == 1) startSession(0, "MAPPING 1 (RHR)");
    else if (btn == 2) startSession(1, "MAPPING 2 (LHR)");
    else if (btn == 4) startSpeedRun();
  } else {
    followLinePID();
  }
}

void followLinePID() {
  bool FL = !digitalRead(pFL), NL = !digitalRead(pNL), M = !digitalRead(pM);
  bool NR = !digitalRead(pNR), FR = !digitalRead(pFR);

  if (((FL && NL) || (FR && NR)) && whiteTime == 0) {
    processJunction(false); 
    return; 
  }

  if (!FL && !NL && !M && !NR && !FR) {
    if (whiteTime == 0) whiteTime = millis();
    else if (millis() - whiteTime > deadEndTimeout) {
      motorControl(0, 0); delay(100);
      processJunction(true);
      whiteTime = 0; 
      return; 
    }
  } else { whiteTime = 0; }

  if (NL && !NR) error = 1.5; 
  else if (!NL && NR) error = -1.5; 
  else if (M) error = 0.0;
  else error = (lastError > 0) ? 2.5 : -2.5;
  
  int activeDriveSpeed = baseSpeed;
  if (runMode == 2 && error == 0.0) activeDriveSpeed = speedRunStraightSpeed;

  float turn = (Kp * error) + (Kd * (error - lastError));
  lastError = error;
  motorControl(constrain(activeDriveSpeed - turn, 0, 255), constrain(activeDriveSpeed + turn, 0, 255));
}

void processJunction(bool isDead) {
  bool vL = !digitalRead(pFL) && !digitalRead(pNL);
  bool vR = !digitalRead(pFR) && !digitalRead(pNR);
  bool vS = false, isGoal = false;

  if (!isDead) { 
    int currentPushTime = (runMode == 2) ? speedRunAlignPush : mapAlignPush;
    motorControl(baseSpeed, baseSpeed); 
    unsigned long pushStart = millis();
    while (millis() - pushStart < currentPushTime) {
      if (millis() - pushStart < 150) {
        if (!digitalRead(pFL) && !digitalRead(pNL)) vL = true; 
        if (!digitalRead(pFR) && !digitalRead(pNR)) vR = true; 
      }
    }
    if (!digitalRead(pFL) && !digitalRead(pM) && !digitalRead(pFR)) {
      isGoal = true; 
    } else {
      if (runMode == 1) { motorControl(0, 0); delay(200); }
      vS = !digitalRead(pM) || !digitalRead(pNL) || !digitalRead(pNR);
    }
  } else { vS = true; }

  if (isGoal) {
    motorControl(baseSpeed, baseSpeed); delay(350); 
    motorControl(0,0); delay(1000);
    lcd.clear(); lcd.print("GOAL REACHED!");
    if (runMode == 1) {
      EEPROM.write(mapStarts[currentSlot], pathLen);
      for(int i=0; i<pathLen; i++) EEPROM.write(mapStarts[currentSlot] + 1 + i, currentPath[i]);
      EEPROM.put(currentSlot*4 + 4, millis() - startTime);
      runTimes[currentSlot] = millis() - startTime;
      lcd.setCursor(0,1); lcd.print("MAP SAVED.");
    } else {
      lcd.setCursor(0,1); lcd.print("TIME: "); lcd.print((millis() - startTime) / 1000.0);
    }
    while(1); 
  }

  if (runMode == 1) {
    byte absR = (currentHeading + 1) % 4, absS = currentHeading;
    byte absL = (currentHeading + 3) % 4, absB = (currentHeading + 2) % 4;
    bool can[4] = {false, false, false, false};
    if (vR) can[absR] = true; if (vS && !isDead) can[absS] = true; if (vL) can[absL] = true;

    byte chosenAbs = 255; 
    if (isDead) chosenAbs = absB; 
    else {
      byte priority[3];
      if (currentSlot == 1) { priority[0] = absL; priority[1] = absS; priority[2] = absR; } // LHR
      else { priority[0] = absR; priority[1] = absS; priority[2] = absL; } // RHR
      for (int i=0; i<3; i++) { if (can[priority[i]]) { chosenAbs = priority[i]; break; } }
    }
    if (chosenAbs == 255) chosenAbs = absB;

    byte relativeTurn = (chosenAbs - currentHeading + 4) % 4;
    if (relativeTurn == 1) turn90(true);
    else if (relativeTurn == 3) turn90(false);
    else if (relativeTurn == 2) turn180();

    currentPath[pathLen++] = chosenAbs;
    currentHeading = chosenAbs;    
    simplifyPathAbsolute(); 
  } 
  else {
    if (pathIndex < pathLen) { 
      byte chosenAbs = currentPath[pathIndex++];
      byte relativeTurn = (chosenAbs - currentHeading + 4) % 4;
      if (relativeTurn == 1) turn90(true);
      else if (relativeTurn == 3) turn90(false);
      else if (relativeTurn == 0) { motorControl(speedRunStraightSpeed, speedRunStraightSpeed); delay(300); }
      currentHeading = chosenAbs; 
    }
  }
}

void simplifyPathAbsolute() {
  if (pathLen < 3) return;
  if (((currentPath[pathLen - 3] + 2) % 4) == currentPath[pathLen - 2]) {
    currentPath[pathLen - 3] = currentPath[pathLen - 1]; 
    pathLen -= 2;
  }
}

void turn90(bool right) {
  if (right) motorControl(turnSpeed, -turnSpeed); else motorControl(-turnSpeed, turnSpeed);
  delay(200); unsigned long spinStart = millis();
  while (digitalRead(pM) == 0 && (millis() - spinStart < 400)); 
  while (digitalRead(pM) == 1); delay(50); 
  if (right) motorControl(-turnSpeed, turnSpeed); else motorControl(turnSpeed, -turnSpeed);
  delay(brakeDur); 
  if (runMode == 1) { motorControl(mapBaseSpeed, mapBaseSpeed); delay(150); motorControl(0, 0); delay(100); }
  else { motorControl(speedRunBaseSpeed, speedRunBaseSpeed); delay(180); }
}

void turn180() {
  motorControl(turnSpeed, -turnSpeed); delay(200); unsigned long spinStart = millis();
  while (digitalRead(pM) == 0 && (millis() - spinStart < 400)); 
  while (digitalRead(pM) == 1); delay(50); 
  motorControl(-turnSpeed, turnSpeed); delay(brakeDur); 
  if (runMode == 1) { motorControl(mapBaseSpeed, mapBaseSpeed); delay(150); motorControl(0, 0); delay(100); }
  else { motorControl(speedRunBaseSpeed, speedRunBaseSpeed); delay(180); }
}

void escapeStartBox() {
  if (!digitalRead(pFL) || !digitalRead(pFR)) {
    lcd.clear(); lcd.print("Escaping Box..."); motorControl(baseSpeed, baseSpeed);
    while (!digitalRead(pFL) || !digitalRead(pFR)) { delay(10); }
    delay(100); 
  }
}

int readButtons() {
  int a = analogRead(A0);
  if (a < 200) return 1; if (a < 400) return 2; if (a < 800) return 4; 
  return 0;
}

void startSession(int slot, String msg) {
  currentSlot = slot; pathLen = 0; currentHeading = 0;
  baseSpeed = mapBaseSpeed; turnSpeed = mapTurnSpeed;
  if (slot == 0) runTimes[1] = 0; // Clear memory 2 if starting fresh track
  lcd.clear(); lcd.print(msg); delay(2000); 
  escapeStartBox(); lcd.clear(); lcd.print("MAPPING...");
  runMode = 1; startTime = millis();
}

void startSpeedRun() {
  int best = -1; unsigned long minTime = 4294967295UL; 
  for(int i=0; i<2; i++) { if(runTimes[i] > 0 && runTimes[i] < minTime) { minTime = runTimes[i]; best = i; } }
  if (best == -1) { lcd.clear(); lcd.print("NO MAP SAVED!"); delay(2000); return; }
  currentSlot = best; pathLen = EEPROM.read(mapStarts[best]);
  for(int i=0; i<pathLen; i++) currentPath[i] = EEPROM.read(mapStarts[best] + 1 + i);
  baseSpeed = speedRunBaseSpeed; turnSpeed = speedRunTurnSpeed; currentHeading = 0;
  lcd.clear(); lcd.print("FASTEST: M"); lcd.print(best+1); delay(2000); 
  escapeStartBox(); lcd.clear(); lcd.print("SPEED RUN...");
  runMode = 2; pathIndex = 0; startTime = millis();
}

void motorControl(int l, int r) {
  if (l >= 0) { digitalWrite(M1_DIR, LOW); analogWrite(M1_PWM, l); } 
  else { digitalWrite(M1_DIR, HIGH); analogWrite(M1_PWM, 255 - abs(l)); }
  if (r >= 0) { digitalWrite(M2_DIR, LOW); analogWrite(M2_PWM, r); } 
  else { digitalWrite(M2_DIR, HIGH); analogWrite(M2_PWM, 255 - abs(r)); }
}