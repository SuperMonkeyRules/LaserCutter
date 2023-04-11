#include <Arduino.h>
#include <AccelStepper.h>
#include <Keypad.h>
#include <main.h>

const float MMPerStep = 1.0 / 25.0; // 200 steps = 8 mm | 100 steps = 4 mm | 25 steps = 1mm
const size_t BUFFER_SIZE = 256;

const int XmotorPUL = 15;
const int XmotorDIR = 14;
const int XmotorENA = 13;
const int YmotorPUL = 16;
const int YmotorDIR = 17;
const int YmotorENA = 18;
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

const int Xmax = 300;
const int Ymax = 300;
const int Xmin = 0;
const int Ymin = 0;

float feedrate = float(0);
float TravSpeed = float(8000);
int amount2Step = 1000;
int brightness = 10;

const boolean debug = true;

void setup()
{
  Serial.begin(9600);
  delay(1000);
  Serial.println("Laser Cutter on");
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LaserCtrl, OUTPUT);

  Serial.println("Setting up motors");
  Xaxis.setMaxSpeed(float(10000));
  Yaxis.setMaxSpeed(float(10000));
  // ENABLING MAY CAUSE PROBLEMS
  Xaxis.setEnablePin(XmotorENA);
  Yaxis.setEnablePin(YmotorENA);
  Xaxis.enableOutputs();
  Yaxis.enableOutputs();
  Serial.println("Everything should be running");
  Serial.println("Exiting setup");
}

void loop()
{
  char key = keypad.getKey();

  if (key != NO_KEY)
  {
    Serial.println(key);
    if (key == 1)
    {
      setFeedrate(10);
    }
    if (key == 3)
    {
      setFeedrate(50);
    }
    if (key == 7)
    {
      setFeedrate(100);
    }
    if (key == 9)
    {
      setFeedrate(1000);
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
  char *indexX = strchr(line, 'X');
  char *indexY = strchr(line, 'Y');
  char *indexZ = strchr(line, 'Z');
  char *indexF = strchr(line, 'F');
  char *indexS = strchr(line, 'S');
  char *indexP = strchr(line, 'P');

  Serial.println(line[0]);

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
void poll_steppers(void)
{
  Xaxis.runSpeed();
  Yaxis.runSpeed();
}
bool is_moving(void)
{
  return (Xaxis.isRunning() || Yaxis.isRunning());
}

void move(int x, int y)
{
  Serial.print("Moving to ");
  Serial.print(x);
  Serial.print(", ");
  Serial.println(y);

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

  Xaxis.setSpeed(feedrate);
  Yaxis.setSpeed(feedrate);
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