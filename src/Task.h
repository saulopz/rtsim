#ifndef TASK_H_
#define TASK_H_

#include <allegro.h>
#include <vector>
#include "Budget.h"
#include "TimeLine.h"
#include "Aperiodic.h"
#include "SporadicServer.h"

class Budget;
class TimeLine;
class Aperiodic;
class SporadicServer;

class Task
{
public:
	BITMAP *image;
	int type;
	int period;
	float execution;
	float time;
	int priority;
	int color;
	Budget *budget;
	float executed;
	TimeLine *timeLine;
	bool running;
	char name[10];
	std::vector<Aperiodic *> aperiodic;
	int aperiodicIndex;
	SporadicServer *SS;

	Task(TimeLine *, int, int, float, int);
	void addAperiodic(float, float, int);
	void make();
	void run();
	void runPeriodic();
	void runSampleSporadicServer();
	void runDeferrableServer();
	void runPoolingServer();
	bool isHigherRunning();
	bool isLowerRunning();
	void orderAperiodic();
	Aperiodic *getAperiodic();
	void nextAperiodic();
	bool periodPoint();
	bool lowerTaskWillRun();
	void showAperiodicResponseTime();
	float getNextBusyTime();
	virtual ~Task();
};

#endif /*TASK_H_*/
