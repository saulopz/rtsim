/***********************************************************************
 * FEDERAL UNIVERSITY OF SANTA CATARINA - UFSC
 * POSTGRADUATE PROGRAM IN AUTOMATION AND SYSTEMS ENGINEERING - PPGEAS
 * CLASS: DAS6663 - REAL TIME SYSTEMS
 * AUTHOR: SAULO POPOV ZAMBIASI
 * PROGRAM: RTSIM
 * DATE: NOVEMBER 22, 2006
 * *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "Interface.h"

int main(int argv, char *argc[])
{
    if (argv != 2)
    {
        printf("Usage: rtsim namefile (without .in, just name)\n");
        exit(1);
    }
    Interface *interface = new Interface(argc[1]);
    interface->run();
    delete interface;
    return 0;
}
END_OF_MAIN();
