#define LINE_BUFFER_LENGTH 512
#include <Arduino.h>
#include <AccelStepper.h>
#include <Keypad.h>
//Hello from laptop
const int MMPerStep = 1 / 250;

const int XmotorPUL = 1;
const int XmotorDIR = 2;
const int XmotorENA = 5;
const int YmotorPUL = 3;
const int YmotorDIR = 4;
const int YmotorENA = 5;
const int LaserCtrl = 6;

AccelStepper Xaxis(1, XmotorPUL, XmotorDIR);
AccelStepper Yaxis(1, YmotorPUL, YmotorDIR);

const byte rows = 4;  //four rows
const byte cols = 3;  //three columns
char keys[rows][cols] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '#', '0', '*' }
};
byte rowPins[rows] = { 10, 9, 8, 7 };  //connect to the row pinouts of the keypad
byte colPins[cols] = { 13, 12, 11 };   //connect to the column pinouts of the keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);

const int Xmax = 300;
const int Ymax = 300;
const int Xmin = 0;
const int Ymin = 0;

int feedrate = 0;
int amount2Step = 100;
int brightness = 10;
boolean debug = true;

char line[LINE_BUFFER_LENGTH];
char c;
int lineIndex;
bool lineIsComment, lineSemiColon;

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LaserCtrl, OUTPUT);

  Xaxis.setMaxSpeed(100.0);
  Yaxis.setMaxSpeed(100.0);
  Xaxis.setEnablePin(XmotorENA);
  Yaxis.setEnablePin(YmotorENA);
  Xaxis.enableOutputs();
  Yaxis.enableOutputs();
}

void loop() {
  lineIndex = 0;
  lineSemiColon = false;
  lineIsComment = false;
  char key = keypad.getKey();

  if (key != NO_KEY) {
    Serial.println(key);
    if (key == 1) {
      setFeedrate(10);
    }
    if (key == 3) {
      setFeedrate(50);
    }
    if (key == 7) {
      setFeedrate(100);
    }
    if (key == 9) {
      setFeedrate(1000);
    }
    if (key == 2) {
      move(-amount2Step, Yaxis.currentPosition());
    }
    if (key == 4) {
      move(amount2Step, Yaxis.currentPosition());
    }
    if (key == 6) {
      move(Xaxis.currentPosition(), -amount2Step);
    }
    if (key == 8) {
      move(Xaxis.currentPosition(), amount2Step);
    }
  }

  while (Serial.available() > 0) {
    c = Serial.read();
    if ((c == '\n') || (c == '\r')) {
      if (lineIndex > 0) {
        line[lineIndex] = '\0';
        if (debug) {
          Serial.print("Received: ");
          Serial.println(line);
        }
        processIncomingLine(line);
        lineIndex = 0;
      } else {
        // Empty or comment line. Skip block.
      }
      lineIsComment = false;
      lineSemiColon = false;
      Serial.println("ok");
    } else {
      if ((lineIsComment) || (lineSemiColon)) {  // Throw away all comment characters
        if (c == ')') lineIsComment = false;     // End of comment. Resume line.
      } else {
        if (c <= ' ') {         // Throw away whitepace and control characters
        } else if (c == '/') {  // Block delete not supported. Ignore character.
        } else if (c == '(') {  // Enable comments flag and ignore all characters until ')' or EOL.
          lineIsComment = true;
        } else if (c == ';') {
          lineSemiColon = true;
        } else if (lineIndex >= LINE_BUFFER_LENGTH - 1) {
          Serial.println("ERROR - lineBuffer overflow");
          lineIsComment = false;
          lineSemiColon = false;
        } else if (c >= 'a' && c <= 'z') {  // Upcase lowercase
          line[lineIndex++] = c - 'a' + 'A';
        } else {
          line[lineIndex++] = c;
        }
      }
    }
  }
}


void processIncomingLine(char* line) {
  char* indexX = strchr(line, 'X');
  char* indexY = strchr(line, 'Y');
  char* indexZ = strchr(line, 'Z');
  char* indexF = strchr(line, 'F');
  char* indexS = strchr(line, 'S');
  char* indexP = strchr(line, 'P');
  char* indexR = strchr(line, 'R');

  Serial.print(line[0]);

  switch (line[0]) {
    case 'G':
      switch (atoi(line + 1)) {
        case 0:
        case 1:
          if (!indexX && !indexY) {
            Serial.println("No X OR Y");
            break;
          }
          if (indexS) {
            setBrightness(atoi(indexS + 1));
          }
          if (indexF) {
            setFeedrate(atof(indexF + 1));
          }
          if (indexZ) {
            if (atoi(indexZ + 1) <= 0) {
              penDown();
            } else {
              penUp();
            }
          }
          move(atoi(indexX + 1), atoi(indexY + 1));
          break;
        case 21:
          Serial.println("Set to Millimeters (NOT REALLY)");
          break;
        case 28:
          Serial.println("HOMING");
          move(0, 0);
          break;
        default:
          Serial.println("=====ERROR ON CASE G=====");
          break;
      }
      break;
    case 'M':
      switch (atoi(line + 1)) {
        case 0:
          if (indexP) {
            delay(atoi(indexP + 1));
          } else {
            Serial.println("M0 recieved with no time");
          }
          break;
        case 3:
          if (indexS) {
            setBrightness(atoi(indexS + 1));
          }
          break;
        case 5:
          if (indexS) {
            setBrightness(0);
          }
          break;
        case 8:
          Serial.println("Air Assist On");
          break;
        case 9:
          Serial.println("Air Assist Off");
          break;
        case 114:
          Serial.print("Current Position (Steps): ");
          Serial.print(Xaxis.currentPosition());
          Serial.print(", ");
          Serial.println(Yaxis.currentPosition());
          break;
        default:
          Serial.println("=====ERROR ON CASE M=====");
          break;
      }
      break;
    default:
      Serial.println("=====ERROR DURING GCODE INDEX=====");
      break;
  }
}

void setFeedrate(float feedInMMpS) {
  feedrate = (feedInMMpS / MMPerStep);
  Serial.print("Setting speed to: ");
  Serial.println(feedrate);
}
void setBrightness(int pwrIn100) {
  brightness = (pwrIn100 / 100) * 255;
  Serial.print("Laser: ");
  Serial.println(brightness);
}
void poll_steppers(void) {
  Xaxis.run();
  Yaxis.run();
}
bool is_moving(void) {
  return (Xaxis.isRunning() || Yaxis.isRunning());
}

void move(long x, long y) {
  Serial.print("Moving to ");
  Serial.print(x);
  Serial.print(", ");
  Serial.print(y);

  if (x > Xmax) {
    x = Xmax;
    Serial.println("X larger than canvas.");
    return;
  }
  if (x < Xmin) {
    x = Xmin;
    Serial.println("X smaller than canvas.");
    return;
  }
  if (y > Ymax) {
    y = Ymax;
    Serial.println("Y larger than canvas.");
    return;
  }
  if (y < Ymin) {
    y = Ymin;
    Serial.println("Y smaller than canvas.");
    return;
  }

  int xInSteps = x / MMPerStep;
  int yInSteps = y / MMPerStep;

  Xaxis.moveTo(xInSteps);
  Yaxis.moveTo(yInSteps);

  Xaxis.setSpeed(feedrate);
  Yaxis.setSpeed(feedrate);
  do {
    poll_steppers();
  } while (is_moving());

  Serial.println("Move Successful");
}

//  Raises pen
void penUp() {
  digitalWrite(LED_BUILTIN, false);
  analogWrite(LaserCtrl, 0);
  Xaxis.setSpeed(Xaxis.maxSpeed());
  Yaxis.setSpeed(Yaxis.maxSpeed());
  if (debug) {
    Serial.println("Pen up.");
    Serial.println("Traversal Speed Set.");
  }
}

//  Lowers pen
void penDown() {
  digitalWrite(LED_BUILTIN, true);
  analogWrite(LaserCtrl, brightness);
  if (debug) {
    Serial.println("Pen down.");
  }
}