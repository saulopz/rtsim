#ifndef BUDGET_H_
#define BUDGET_H_

#include <allegro.h>
#include "TimeLine.h"

class TimeLine;

class Budget
{
public:
	BITMAP *image;
	TimeLine *timeLine;
	double size;
	double value;
	double lastValue;
	bool replenishedAll;
	bool consumedAll;
	int color;
	char name[20];

	Budget(TimeLine *, char *, double);
	void make();
	void run();
	void consume();
	void consumeAll();
	void replenish();
	void replenishAll();
	bool exhausted();
	virtual ~Budget();
};

#endif /*BUDGET_H_*/
