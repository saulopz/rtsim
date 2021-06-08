#include <allegro.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Budget.h"
#include "TimeLine.h"
#include "config.h"
#include "Task.h"

Budget::Budget(TimeLine *tl, char *n, double s)
{
    timeLine = tl;
    strcpy(name, n);
    size = s;
    value = size;
    lastValue = value;
    replenishedAll = false;
}

void Budget::make()
{
    image = create_bitmap((int)(timeLine->timeSize * 10), 40 + BUDGET_SIZE);
    clear_to_color(image, TRANSPARENT);
    line(image, 1, image->h - BUDGET_SIZE - 1,
         image->w, image->h - BUDGET_SIZE - 1,
         makecol(200, 200, 200));
    line(image, 1, image->h - 1, image->w, image->h - 1, makecol(200, 200, 200));
    char strTask[255];
    sprintf(strTask, "%s(%.1f)", name, size);
    textout_ex(image, font, strTask, 5, 5, COLOR_LINE_TASK, TRANSPARENT);
}

void Budget::consume()
{
    if (value > 0.0)
        value -= 0.1;
}

void Budget::consumeAll()
{
    value = 0.0;
    consumedAll = true;
}

void Budget::replenish()
{
    if (value < size)
        value += 0.1;
}

void Budget::replenishAll()
{
    value = size;
    replenishedAll = true;
}

bool Budget::exhausted()
{
    return (value <= 0.0);
}

void Budget::run()
{
    int val = (int)(BUDGET_SIZE * (lastValue / size));
    int x1 = (int)((timeLine->time * 10) - 1);
    int y1 = (int)(image->h - val) - 1;

    val = (int)(BUDGET_SIZE * (value / size));
    int x2 = (int)(timeLine->time * 10);
    int y2 = (int)(image->h - val) - 1;

    if (replenishedAll || consumedAll)
    {
        replenishedAll = false;
        consumedAll = false;
        line(image, x1, y1, x2, y1, color);
        line(image, x2, y1, x2, y2, color);
    }
    else
    {
        line(image, x1, y1, x2, y2, color);
    }
    lastValue = value;
}

Budget::~Budget()
{
    destroy_bitmap(image);
}
