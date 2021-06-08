#ifndef TIMELINE_H_
#define TIMELINE_H_

#include "config.h"
#include "Task.h"
#include "Budget.h"
#include <allegro.h>
#include <vector>

class Task;
class Budget;

class TimeLine
{
public:
	BITMAP *image;
	float imageSize;
	int type;
	int posX;
	int posY;
	float time;
	std::vector<Task *> task;
	std::vector<Budget *> budget;
	char fileName[30];
	int timeSize;

	TimeLine(char *);
	Task *addTask(int, int, float, int);
	Budget *addBudget(float);
	void orderTasks();
	int computeSchaduling();
	void computeTimeSize();
	void run();
	void draw();
	void makeTasks();
	void makeTimeLine();
	void saveTimeLine();
	void saveResponseTime();
	void executeTasks();
	void loadFile();
	~TimeLine();
};

#endif /*TIMELINE_H_*/
