#include "Interface.h"

Interface::Interface(char *fName)
{
    finished = false;
    strcpy(fileName, fName);
    allegro_init();
    install_keyboard();
    set_color_depth(16);
    set_gfx_mode(VMODE, MAX_X, MAX_Y, 0, 0);
    buffer = create_bitmap(MAX_X, MAX_Y);

    timeLine = new TimeLine(fileName);
    menu = new Menu();
}

void Interface::run()
{
    timeLine->run();
    timeLine->imageSize = (float)(MAX_Y - 50) / timeLine->image->h;
    draw();
    while (!finished)
    {
        if (keypressed())
        {
            int valueMove = 5;
            if (key[KEY_LSHIFT])
                valueMove = 20;
            if (key[KEY_ESC])
                finished = true;
            if (key[KEY_UP])
                timeLine->posY -= valueMove;
            if (key[KEY_DOWN])
                timeLine->posY += valueMove;
            if (key[KEY_LEFT])
                timeLine->posX -= valueMove;
            if (key[KEY_RIGHT])
                timeLine->posX += valueMove;
            if (key[KEY_PGUP])
            {
                double size = timeLine->imageSize;
                timeLine->imageSize *= 1.02;
                timeLine->posX -= (MAX_X / 2);
                timeLine->posY -= (MAX_Y / 2);
                timeLine->posX = (int)(timeLine->imageSize * timeLine->posX / size);
                timeLine->posY = (int)(timeLine->imageSize * timeLine->posY / size);
                timeLine->posX += (MAX_X / 2);
                timeLine->posY += (MAX_Y / 2);
            }
            if (key[KEY_PGDN])
            {
                double size = timeLine->imageSize;
                timeLine->imageSize *= 0.98;
                timeLine->posX -= (MAX_X / 2);
                timeLine->posY -= (MAX_Y / 2);
                timeLine->posX = (int)(timeLine->imageSize * timeLine->posX / size);
                timeLine->posY = (int)(timeLine->imageSize * timeLine->posY / size);
                timeLine->posX += (MAX_X / 2);
                timeLine->posY += (MAX_Y / 2);
            }
            if (key[KEY_HOME])
            {
                timeLine->posX = 50;
                timeLine->posY = 50;
            }
            if (key[KEY_END])
            {
                timeLine->posX = (int)(MAX_X - (timeLine->image->w * timeLine->imageSize) - 50);
                timeLine->posY = 50;
            }
            if (key[KEY_A])
                timeLine->imageSize = 1;
            draw();
        }
    }
}

void Interface::draw()
{
    timeLine->draw();
    menu->draw();
    clear_to_color(buffer, makecol(255, 255, 255));
    stretch_sprite(buffer,
                   timeLine->image, timeLine->posX, timeLine->posY,
                   (int)(timeLine->image->w * timeLine->imageSize),
                   (int)(timeLine->image->h * timeLine->imageSize));
    draw_sprite(buffer, menu->image, 0, 0);
    blit(buffer, screen, 0, 0, 0, 0, MAX_X, MAX_Y);
}

Interface::~Interface()
{
    delete timeLine;
    delete menu;
    destroy_bitmap(buffer);
    allegro_exit();
}
