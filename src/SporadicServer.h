#ifndef SPORADICSERVER_H_
#define SPORADICSERVER_H_

#include "config.h"

class SporadicServer
{
public:
	int state;
	int ThState;
	int TState;
	float BEGIN;
	float END;
	float tr;
	float tc;
	float tf;
	float te;
	float tb;
	float t;
	float ps;

	SporadicServer();
	virtual ~SporadicServer();
};

#endif /*SPORADICSERVER_H_*/
