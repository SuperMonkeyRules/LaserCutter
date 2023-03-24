#ifndef LaserCutter_h
#define LaserCutter_h

void setFeedrate(float feedInMMpS);
void setBrightness(int pwrIn100);
void LaserOff();
void LaserOn();
void processIncomingLine(char* line);
void move(int x, int y);
bool is_moving(void);
void poll_steppers(void);

#endif