#include "SporadicServer.h"

SporadicServer::SporadicServer()
{
	ThState = IDLE;
	TState = IDLE;
	state = SUSPENDED;
	BEGIN = 0;
	END = 0;
	tr = 0;
	tf = -1;
	te = 0;
	tb = 0;
}

SporadicServer::~SporadicServer()
{
}
