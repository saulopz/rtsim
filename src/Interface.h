#ifndef INTERFACE_H_
#define INTERFACE_H_

#include <allegro.h>
#include "TimeLine.h"
#include "Menu.h"

class Interface
{
private:
    TimeLine *timeLine;
    Menu *menu;
    char fileName[50];
    bool finished;

public:
    BITMAP *buffer;
    Interface(char *);
    void run();
    void draw();
    virtual ~Interface();
};

#endif /* INTERFACE_H_ */
