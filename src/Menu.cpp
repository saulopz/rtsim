#include "Menu.h"

Menu::Menu()
{
	str1 = "PageUp: +ZOOM - PageDown: -ZOOM - A: SIZE 1:1 - HOME: To Begin - END: To end - ESC: Exit";
	str2 = "Direcionals: move - Direcionals + Left Shift: fast move";
	image = create_bitmap(MAX_X, 40);
}

void Menu::draw()
{
	clear_to_color(image, makecol(200, 200, 240));
	textout_ex(image, font, str1, 5, 5, makecol(0, 0, 0), makecol(200, 200, 240));
	textout_ex(image, font, str2, 5, 15, makecol(0, 0, 0), makecol(200, 200, 240));
}

void Menu::run()
{
}

Menu::~Menu()
{
	destroy_bitmap(image);
}
