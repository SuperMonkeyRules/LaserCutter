#ifndef cutter_h
#define cutter_h

void setFeedrate(float feedInMMpS);
void setBrightness(int pwrIn100);
void laserToggle(int Zaxis);
void processIncomingLine(char *line);
void move(int x, int y);
bool is_moving(void);
void expandArc(int dirn, int prevXaxisVal, int prevYaxisVal, int xAxisVal, int yAxisVal, float iVal, float jVal);
void laserTest();
void poll_steppers(void);
void ManualMovement(char key);

#endif