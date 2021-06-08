#ifndef MENU_H_
#define MENU_H_

#include <allegro.h>
#include "config.h"

class Menu
{
public:
	BITMAP *image;
	const char *str1;
	const char *str2;

	Menu();
	void draw();
	void run();
	virtual ~Menu();
};

#endif /*MENU_H_*/
