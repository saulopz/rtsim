#ifndef APERIODIC_H_
#define APERIODIC_H_

class Aperiodic
{
public:
	double r;
	double e;
	double executed;
	double initTime;
	double endTime;
	bool running;
	int color;
	Aperiodic(double, double, int);
	double getResponseTime();
	virtual ~Aperiodic();
};

#endif /*APERIODIC_H_*/
