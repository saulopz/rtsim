#include "Aperiodic.h"

Aperiodic::Aperiodic(double ir, double ie, int col)
{
    r = ir;
    e = ie;
    executed = 0.0;
    running = false;
    initTime = -1;
    endTime = -1;
    color = col;
}

double Aperiodic::getResponseTime()
{
    if ((endTime > 0) && (initTime > 0))
    {
        return endTime - initTime;
    }
    else
        return 0;
}

Aperiodic::~Aperiodic()
{
}
