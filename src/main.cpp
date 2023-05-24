#include <Arduino.h>
#include <AccelStepper.h>
#include <Keypad.h>
#include <cutter.h>

const float defaultStep = 1.0 / 25.0; // 200 steps = 8 mm | 100 steps = 4 mm | 25 steps = 1mm
float MMPerStep = defaultStep;        // Changeable step per mm
const size_t BUFFER_SIZE = 256;

const int XmotorPUL = 15; // GPIO pin 15
const int XmotorDIR = 14; // GPIO pin 14
const int XmotorENA = 13; // GPIO pin 13
const int YmotorPUL = 16; // GPIO pin 16
const int YmotorDIR = 17; // GPIO pin 17
const int YmotorENA = 18; // GPIO pin 18
const int LaserCtrl = 22; // Not set yet

AccelStepper Xaxis(1, XmotorPUL, XmotorDIR);
AccelStepper Yaxis(1, YmotorPUL, YmotorDIR);

const byte rows = 4;
const byte cols = 3;
char keys[rows][cols] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'#', '0', '*'}};
byte rowPins[rows] = {3, 2, 1, 0}; // Not set yet
byte colPins[cols] = {6, 5, 4};    // Not set yet
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);

const int Xmax = 500; // Max X bed size
const int Ymax = 500; // Max Y bed size
const int Xmin = 0;   // Min X bed size
const int Ymin = 0;   // Min Y bed size

float feedrate = float(0);    // Speed in mm/s of motors
float TravSpeed = float(750); // Traversal speed in steps
int amount2Step = 50;         // Manual movements in mm
int brightness = 0;           // Laser power 0-100

const boolean debug = true;

void setup()
{
  Serial.begin(9600);
  delay(1000);
  Serial.println("Laser Cutter on");
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LaserCtrl, OUTPUT);

  Serial.println("Setting up motors");
  Xaxis.setMaxSpeed(float(750));
  Yaxis.setMaxSpeed(float(750));
  // ENABLING MAY CAUSE PROBLEMS
  Xaxis.setEnablePin(XmotorENA);
  Yaxis.setEnablePin(YmotorENA);
  Xaxis.enableOutputs();
  Yaxis.enableOutputs();
  Serial.println("Exiting setup");
}

void loop()
{
  ManualMovement(keypad.getKey());

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
        if (atoi(indexZ + 1) <= 0)
        {
          LaserOn();
        }
        else
        {
          LaserOff();
        }
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
        if (atoi(indexZ + 1) <= 0)
        {
          LaserOn();
        }
        else
        {
          LaserOff();
        }
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

void ManualMovement(char key)
{
  if (key != NO_KEY)
  {
    Serial.println(key);
    if (key == 1)
    {
      setFeedrate(1);
    }
    if (key == 3)
    {
      setFeedrate(10);
    }
    if (key == 7)
    {
      setFeedrate(25);
    }
    if (key == 9)
    {
      setFeedrate(40);
    }
    if (key == 2)
    {
      move(-amount2Step, (Yaxis.currentPosition() * MMPerStep));
    }
    if (key == 4)
    {
      move(amount2Step, (Yaxis.currentPosition() * MMPerStep));
    }
    if (key == 6)
    {
      move((Xaxis.currentPosition() * MMPerStep), -amount2Step);
    }
    if (key == 8)
    {
      move((Xaxis.currentPosition() * MMPerStep), amount2Step);
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
  brightness = (pwrIn100 / 100) * 255;
  Serial.print("Laser: ");
  Serial.println(brightness);
}

void expandArc(int dirn, int prevXaxisVal, int prevYaxisVal, int xAxisVal, int yAxisVal, float iVal, float jVal)
{

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
  int numSegments = int(abs(sweep / (PI * 2) * 64));

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
}

void poll_steppers(void)
{
  Xaxis.runSpeedToPosition();
  Yaxis.runSpeedToPosition();
}
bool is_moving(void)
{
  return (Xaxis.currentPosition() != Xaxis.targetPosition() || Xaxis.currentPosition() != Xaxis.targetPosition());
}

void move(int x, int y)
{
  Serial.print("Currently at ");
  Serial.print(Xaxis.currentPosition() * MMPerStep);
  Serial.print("mm, ");
  Serial.print(Yaxis.currentPosition() * MMPerStep);
  Serial.println("mm");

  Serial.print("Moving to ");
  Serial.print(x);
  Serial.print("mm, ");
  Serial.print(y);
  Serial.println("mm");

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

  long xInSteps = static_cast<long>(static_cast<float>(x) / MMPerStep);
  long yInSteps = static_cast<long>(static_cast<float>(y) / MMPerStep);

  Xaxis.moveTo(xInSteps);
  Yaxis.moveTo(yInSteps);
  Serial.print("X Moving to ");
  Serial.print(xInSteps);
  Serial.println("(steps)");
  Serial.print("Y Moving to ");
  Serial.print(yInSteps);
  Serial.println("(steps)");

  Xaxis.setSpeed(feedrate);
  Yaxis.setSpeed(feedrate);
  Serial.println("Set feedrates");

  do
  {
    poll_steppers();
  } while (is_moving());

  Serial.println("Move Successful");
}

void LaserOff()
{
  digitalWrite(LED_BUILTIN, false);
  analogWrite(LaserCtrl, 0);
  Xaxis.setSpeed(TravSpeed);
  Yaxis.setSpeed(TravSpeed);
  if (debug)
  {
    Serial.println("Pen up.");
    Serial.println("Traversal Speed Set.");
  }
}

void LaserOn()
{
  digitalWrite(LED_BUILTIN, true);
  analogWrite(LaserCtrl, brightness);
  if (debug)
  {
    Serial.println("Pen down.");
  }
}