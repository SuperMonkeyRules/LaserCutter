#include <Arduino.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <Keypad.h>
#include <cutter.h>
#define version 1.20

const float defaultStep = 1.0 / 25.0; // 200 steps = 8 mm | 100 steps = 4 mm | 25 steps = 1mm
float MMPerStep = defaultStep / 4;    // Changeable mm per step
const size_t BUFFER_SIZE = 256;       // Size in bytes of text buffer

const int XmotorPUL = 10; // GPIO pin 10
const int XmotorDIR = 12; // GPIO pin 14
const int XmotorENA = 15; // GPIO pin 13
const int YmotorPUL = 11; // GPIO pin 16
const int YmotorDIR = 13; // GPIO pin 17
const int YmotorENA = 15; // GPIO pin 18
const int LaserPWM = 21;  // GPIO pin 1
const int LaserTGL = 20;  // GPIO pin 0

AccelStepper Xaxis(1, XmotorPUL, XmotorDIR); // Xaxis motor on PUL 15, DIR 14 Enable 13
AccelStepper Yaxis(1, YmotorPUL, YmotorDIR); // Xaxis motor on PUL 16, DIR 17 Enable 18
MultiStepper XYaxis;

/*
const byte ROWS = 4; // four rows
const byte COLS = 3; // three columns
char keys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};
byte rowPins[ROWS] = {5, 4, 3, 2}; // connect to the row pinouts of the kpd
byte colPins[COLS] = {8, 7, 6};    // connect to the column pinouts of the kpd

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
*/
const int Xmax = 500; // Max X bed size
const int Ymax = 500; // Max Y bed size
const int Xmin = 0;   // Min X bed size
const int Ymin = 0;   // Min Y bed size

float feedrate = 0.0F;  // Speed in steps of motors
float TravSpeed = 3000.0F; // Traversal speed in steps
int amount2Step = 50;   // Manual movements in mm
int brightness = 0;     // Laser power 0-100

const boolean debug = true;
boolean skipLimits = true;

void setup()
{
  Serial.begin(9600);
  delay(1000);
  Serial.println("Laser Cutter on");
  Serial.print("Version: ");
  Serial.println(version);
  // pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LaserPWM, OUTPUT);
  pinMode(LaserTGL, OUTPUT);
  digitalWrite(LaserTGL, LOW);
  analogWrite(LaserPWM, 0);

  Serial.println("Setting up motors");
  // ENABLING MAY CAUSE PROBLEMS
  // Xaxis.setEnablePin(XmotorENA);
  // Yaxis.setEnablePin(YmotorENA);
  // Xaxis.enableOutputs();
  // Yaxis.enableOutputs();

  XYaxis.addStepper(Xaxis);
  XYaxis.addStepper(Yaxis);

  Serial.println("Exiting setup");
}

void loop()
{
  // ManualMovement(keypad.getKey());

  while (Serial.available() > 0)
  {
    String data = Serial.readStringUntil('\n');
    data.trim();
    if (data.length() > 0)
    {
      char line[BUFFER_SIZE];
      data.toCharArray(line, BUFFER_SIZE);
      processIncomingLine(line);
    }
    Serial.println("ok");
  }
}

void processIncomingLine(char *line)
{
  char *cmd = strchr(line, 'G');
  char *indexX = strchr(line, 'X');
  char *indexY = strchr(line, 'Y');
  char *indexZ = strchr(line, 'Z');
  char *indexI = strchr(line, 'I');
  char *indexJ = strchr(line, 'J');
  char *indexF = strchr(line, 'F');
  char *indexS = strchr(line, 'S');
  char *indexP = strchr(line, 'P');

  Serial.println(line[0]);
  int dir = 0;

  switch (line[0])
  {
  case 'G':
    switch (atoi(line + 1))
    {
    case 0:
    case 1:
      if (!indexX || !indexY)
      {
        Serial.println("No X OR Y");
        break;
      }
      if (indexS)
      {
        setBrightness(atoi(indexS + 1));
      }
      if (indexF)
      {
        setFeedrate(atof(indexF + 1));
      }
      if (indexZ)
      {
        laserToggle(atoi(indexZ + 1));
      }
      move(atoi(indexX + 1), atoi(indexY + 1));
      break;
    case 2:
    case 3:
      if (!indexX && !indexY)
      {
        Serial.println("No X OR Y");
        break;
      }
      if (indexS)
      {
        setBrightness(atoi(indexS + 1));
      }
      if (indexF)
      {
        setFeedrate(atof(indexF + 1));
      }
      if (indexZ)
      {
        laserToggle(atoi(indexZ + 1));
      }
      if (atoi(cmd + 1) == 3)
        dir = 1;
      expandArc(dir, Xaxis.currentPosition(), Yaxis.currentPosition(), atoi(indexX + 1), atoi(indexY + 1), atoi(indexI + 1), atoi(indexJ + 1));
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
    switch (atoi(line + 1))
    {
    case 0:
      if (indexP)
      {
        delay(atoi(indexP + 1));
      }
      else
      {
        Serial.println("M0 recieved with no time");
      }
      break;
    case 3:
      if (indexS)
      {
        setBrightness(atoi(indexS + 1));
      }
      break;
    case 5:
      if (indexS)
      {
        setBrightness(0);
      }
      break;
    case 8:
      Serial.println("Air Assist On");
      break;
    case 9:
      Serial.println("Air Assist Off");
      break;
    case 17:
      Xaxis.enableOutputs();
      Yaxis.enableOutputs();
      break;
    case 18:
      Xaxis.disableOutputs();
      Yaxis.disableOutputs();
      break;
    case 84:
      Xaxis.disableOutputs();
      Yaxis.disableOutputs();
      break;
    case 92:
      Serial.println("Setting microstepping");
      if (indexX)
      {
        MMPerStep = defaultStep / atoi(indexX + 1);
      }
      if (indexY)
      {
        MMPerStep = defaultStep / atoi(indexY + 1);
      }
      break;
    case 114:
      Serial.print("Current Position (MM?): ");
      Serial.print(Xaxis.currentPosition() * MMPerStep);
      Serial.print(", ");
      Serial.println(Yaxis.currentPosition() * MMPerStep);
      break;
    case 115:
      Serial.print("Current version: ");
      Serial.println(version);
      break;
    case 810:
      laserTest();
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

void ManualMovement(char key)
{
  if (key != NO_KEY)
  {
    if (key == '1')
    {
      Serial.println(key);
      setFeedrate(1);
    }
    if (key == '3')
    {
      Serial.println(key);
      setFeedrate(10);
    }
    if (key == '7')
    {
      Serial.println(key);
      setFeedrate(20);
    }
    if (key == '9')
    {
      Serial.println(key);
      setFeedrate(30);
    }
    if (key == '2')
    {
      Serial.println(key);
      move(Xaxis.currentPosition() - amount2Step, (Yaxis.currentPosition() * MMPerStep));
    }
    if (key == '4')
    {
      Serial.println(key);
      move(Xaxis.currentPosition() + amount2Step, (Yaxis.currentPosition() * MMPerStep));
    }
    if (key == '6')
    {
      Serial.println(key);
      move((Xaxis.currentPosition() * MMPerStep), Yaxis.currentPosition() - amount2Step);
    }
    if (key == '8')
    {
      Serial.println(key);
      move((Xaxis.currentPosition() * MMPerStep), Yaxis.currentPosition() + amount2Step);
    }
  }
}

void setFeedrate(float feedInMMpS)
{
  feedrate = (feedInMMpS / MMPerStep);
  Serial.print("Setting speed to: ");
  Serial.println(feedrate);
}

void setBrightness(int pwrIn100)
{
  brightness = (pwrIn100 * 255) / 100;
  Serial.print("Laser power: ");
  Serial.println(brightness);
}

void expandArc(int dirn, int prevXaxisVal, int prevYaxisVal, int xAxisVal, int yAxisVal, float iVal, float jVal)
{
  /*
  float startX = (prevXaxisVal)*MMPerStep;
  float startY = (prevYaxisVal)*MMPerStep;

  float centreX = startX + iVal;
  float centreY = startY + jVal;
  float dxStart = startX - centreX;
  float dyStart = startY - centreY;
  float startAngle = atan2(dyStart, dxStart);
  float dxEnd = xAxisVal - centreX;
  float dyEnd = yAxisVal - centreY;
  float endAngle = atan2(dyEnd, dxEnd);
  if (endAngle > startAngle)
  {
    endAngle = endAngle - (PI * 2);
  }
  float radius = sqrt((dyStart * dyStart) + (dxStart * dxStart));

  float sweep = endAngle - startAngle;
  if (dirn == 3)
  {
    sweep = (PI * 2) + sweep;
  }
  int numSegments = int(abs(sweep / (PI * 2) * (round(radius) * 4)));

  for (int x = 0; x < numSegments; x++)
  {
    float fraction = float(x) / numSegments;
    float stepAngle = startAngle + (sweep * fraction);
    float stepX = centreX + cos(stepAngle) * radius;
    float stepY = centreY + sin(stepAngle) * radius;
    if (dirn == 2)
    {
      if (sweep > 0)
      {
        float stepY = -stepY;
      }
    }
    else
    {
      if (sweep < 0)
      {
        float stepY = -stepY;
      }
    }
    move(round(stepX), round(stepY));
  }
  */
}

void laserTest()
{
  laserToggle(1);
  setBrightness(25);
  laserToggle(-1);
  delay(100);
  laserToggle(1);
}

void move(int x, int y)
{
  Serial.print("Currently at ");
  Serial.print(Xaxis.currentPosition() * MMPerStep);
  Serial.print(", ");
  Serial.println(Yaxis.currentPosition() * MMPerStep);

  Serial.print("Moving to ");
  Serial.print(x);
  Serial.print(", ");
  Serial.println(y);

  if (!skipLimits)
  {
    if (x > Xmax)
    {
      x = Xmax;
      Serial.println("X larger than canvas.");
      return;
    }
    if (x < Xmin)
    {
      x = Xmin;
      Serial.println("X smaller than canvas.");
      return;
    }
    if (y > Ymax)
    {
      y = Ymax;
      Serial.println("Y larger than canvas.");
      return;
    }
    if (y < Ymin)
    {
      y = Ymin;
      Serial.println("Y smaller than canvas.");
      return;
    }
  }

  long xInSteps = static_cast<long>(static_cast<float>(x) / MMPerStep);
  long yInSteps = static_cast<long>(static_cast<float>(y) / MMPerStep);

  Xaxis.setMaxSpeed(feedrate);
  Yaxis.setMaxSpeed(feedrate);

  long positions[2];

  positions[0] = xInSteps;
  positions[1] = yInSteps;
  XYaxis.moveTo(positions);
  XYaxis.runSpeedToPosition(); // Blocks until all are in position CHANGE LATER

  Serial.println("Move Successful");
}

void laserToggle(int Zaxis)
{
  if (Zaxis >= 0)
  {
    // digitalWrite(LED_BUILTIN, false);
    digitalWrite(LaserTGL, LOW);
    analogWrite(LaserPWM, 0);
    Xaxis.setMaxSpeed(TravSpeed);
    Yaxis.setMaxSpeed(TravSpeed);
    if (debug)
    {
      Serial.println("Laser DISABLED");
      Serial.println("Traversal Speed Set.");
    }
  }
  else
  {
    // digitalWrite(LED_BUILTIN, true);
    analogWrite(LaserPWM, brightness);
    digitalWrite(LaserTGL, HIGH);
    if (debug)
    {
      Serial.println("Laser ACTIVE");
    }
  }
}