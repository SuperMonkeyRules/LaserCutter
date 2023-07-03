#ifndef LaserCutter_h
#define LaserCutter_h

void setFeedrate(float feedInMMpS);
void setBrightness(int pwrIn100);
void laserToggle(int Zaxis);
void processIncomingLine(char* line);
void move(int x, int y);
void expandArc(int dirn, int prevXaxisVal, int prevYaxisVal, int xAxisVal, int yAxisVal, float iVal, float jVal);
void ManualMovement(char key);

#endif