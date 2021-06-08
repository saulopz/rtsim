#include "config.h"
#include "TimeLine.h"
#include "Task.h"
#include "Budget.h"
#include <allegro.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

TimeLine::TimeLine(char *fName)
{
    imageSize = 1;
    image = NULL;
    time = 0.0;
    strcpy(fileName, fName);
    posX = 50;
    posY = 50;
    type = RATE_MONOTONIC;

    loadFile();
    orderTasks();
    if (computeSchaduling())
    {
        printf("Accepted scheduling...\n");
    }
    else
    {
        printf("Not accepted scheduling...\n");
    }
    computeTimeSize();
    makeTasks();
}

void TimeLine::loadFile()
{
    FILE *file;
    char n[50];
    char taskType[70];
    float period;
    float execution;
    int r, g, b;
    Task *t = NULL;

    strcpy(n, fileName);
    strcat(n, ".in");
    if ((file = fopen(n, "r")) == NULL)
    {
        fprintf(stderr, "Cannot open %s\n", n);
        exit(1);
    }
    while (!feof(file))
    {
        fscanf(file, "%s %f %f %d %d %d\n", taskType, &period, &execution, &r, &g, &b);
        printf("<%s : %.1f : %.1f : %d , %d , %d>\n", taskType, period, execution, r, g, b);
        if (strcmp("periodic", taskType) == 0)
        {
            t = addTask(PERIODIC, (int)period, execution, makecol(r, g, b));
        }
        else if (strcmp("simple_sporadic_server", taskType) == 0)
        {
            t = addTask(SIMPLE_SPORADIC_SERVER, (int)period, execution, makecol(r, g, b));
        }
        else if (strcmp("deferrable_server", taskType) == 0)
        {
            t = addTask(DEFERRABLE_SERVER, (int)period, execution, makecol(r, g, b));
        }
        else if (strcmp("pooling_server", taskType) == 0)
        {
            t = addTask(POOLING_SERVER, (int)period, execution, makecol(r, g, b));
        }
        else if (strcmp("aperiodic", taskType) == 0)
        {
            if (t != NULL)
            {
                t->addAperiodic(period, execution, makecol(r, g, b));
            }
        }
    }
    fclose(file);
}

Task *TimeLine::addTask(int type, int period, float execution, int color)
{
    Task *t = new Task(this, type, period, execution, color);
    task.push_back(t);
    return t;
}

Budget *TimeLine::addBudget(float size)
{
    printf("new budget %d\n", (int)budget.size());
    char *name = (char *)malloc(50 * sizeof(char));
    strcpy(name, "Budget");
    Budget *b = new Budget(this, name, size);
    budget.push_back(b);
    return b;
}

void TimeLine::makeTasks()
{
    for (unsigned int i = 0; i < task.size(); i++)
    {
        task[i]->priority = i + 1;
        task[i]->make();
    }
    for (unsigned int i = 0; i < budget.size(); i++)
    {
        budget[i]->make();
    }
}

void TimeLine::executeTasks()
{
    while (time < timeSize)
    {
        for (unsigned int i = 0; i < task.size(); i++)
        {
            task[i]->run();
        }
        for (unsigned int i = 0; i < budget.size(); i++)
        {
            budget[i]->run();
        }
        time += 0.1;
    }
    for (unsigned int i = 0; i < task.size(); i++)
    {
        task[i]->showAperiodicResponseTime();
    }
    printf("Execution finished...\n");
}

int TimeLine::computeSchaduling()
{
    float a = 0;
    for (unsigned int i = 0; i < task.size(); i++)
    {
        a += task[i]->execution / task[i]->period;
    }
    float b = task.size() * (pow(2, ((float)1 / task.size())) - 1);
    printf("Computing %.3f < %.3f\n", a, b);
    if (a < b)
        return true;
    else
        return false;
}

void TimeLine::computeTimeSize()
{
    int found = false;
    timeSize = 0;
    while (!found)
    {
        timeSize++;
        float total = 0;
        for (unsigned int i = 0; i < task.size(); i++)
        {
            total += (float)(((int)(timeSize)) % ((int)(task[i]->period)));
        }
        found = (total == 0);
    }
    printf("timeSize %d\n", timeSize);
}

void TimeLine::orderTasks()
{
    for (unsigned int a = 0; a < task.size(); a++)
    {
        int lowerValue = task[a]->period;
        unsigned int lowerIndex = a;
        for (unsigned int b = a + 1; b < task.size(); b++)
        {
            if (task[b]->period < lowerValue)
            {
                lowerValue = task[b]->period;
                lowerIndex = b;
            }
        }
        if (a != lowerIndex)
        {
            Task *aux = task[a];
            task[a] = task[lowerIndex];
            task[lowerIndex] = aux;
        }
    }
    for (unsigned int i = 0; i < task.size(); i++)
    {
        if (task[i] != NULL)
        {
            task[i]->orderAperiodic();
            printf("%s%d (%d, %.1f)\n", task[i]->name, i, task[i]->period, task[i]->execution);
        }
    }
    printf("Finish order...\n");
}

void TimeLine::run()
{
    executeTasks();
    draw();
}

void TimeLine::draw()
{
    makeTimeLine();
    int position = image->h - 30;
    for (int i = budget.size() - 1; i >= 0; i--)
    {
        position -= budget[i]->image->h;
        draw_sprite(image, budget[i]->image, 0, position);
    }
    for (int i = task.size() - 1; i >= 0; i--)
    {
        position -= task[i]->image->h;
        draw_sprite(image, task[i]->image, 0, position);
    }
}

void TimeLine::makeTimeLine()
{
    int h = 0;
    for (unsigned int i = 0; i < budget.size(); i++)
    {
        h += budget[i]->image->h;
    }
    for (unsigned int i = 0; i < task.size(); i++)
    {
        h += task[i]->image->h;
    }
    if (image == NULL)
    {
        image = create_bitmap((int)(timeSize * 10), h + 30);
    }
    clear_to_color(image, TRANSPARENT);
    line(image, 0, 0, 0, image->h - 20, COLOR_TIME_LINE);
    line(image, 0, image->h - 20, image->w, image->h - 20, COLOR_TIME_LINE);
    int aux = 0;
    for (int i = 0; i < timeSize; i++)
    {
        if (aux == 5)
        {
            line(image, i * 10, 0, i * 10, image->h - 21, makecol(200, 200, 200));
            char number[10];
            sprintf(number, "%d", i);
            textout_centre_ex(image, font, number, i * 10, image->h - 15, COLOR_TIME_LINE, TRANSPARENT);
            aux = 0;
        }
        else
        {
            if (i > 0)
            {
                line(image, i * 10, 0, i * 10, image->h - 21, makecol(240, 240, 240));
            }
        }
        aux++;
    }
}

void TimeLine::saveTimeLine()
{
    PALLETE pal;
    char n[50];
    get_palette(pal);
    BITMAP *temp = create_bitmap(image->w, image->h);
    clear_to_color(temp, makecol(255, 255, 255));
    draw_sprite(temp, image, 0, 0);
    sprintf(n, "%s.bmp", fileName);
    save_bitmap(n, temp, pal);
    printf("Writting %s...\n", n);
}

void TimeLine::saveResponseTime()
{
    FILE *file;
    char n[50];
    sprintf(n, "%s.out", fileName);
    if ((file = fopen(n, "w")) == NULL)
    {
        fprintf(stderr, "Cannot open %s\n", n);
        exit(1);
    }
    for (unsigned int i = 0; i < task.size(); i++)
    {
        fprintf(file, "%s%d (%d, %.1f)\n", task[i]->name, task[i]->priority, task[i]->period, task[i]->execution);
        for (unsigned int j = 0; j < task[i]->aperiodic.size(); j++)
        {
            fprintf(file, "\t(A: r=%.1f, e=%.1f) - RT:%.1f\n",
                    task[i]->aperiodic[j]->r,
                    task[i]->aperiodic[j]->e,
                    task[i]->aperiodic[j]->getResponseTime());
        }
    }
    fclose(file);
    printf("Writting %s...\n", n);
}

TimeLine::~TimeLine()
{
    saveTimeLine();
    saveResponseTime();
    for (unsigned int i = 0; i < task.size(); i++)
    {
        delete task[i];
    }
    for (unsigned int i = 0; i < budget.size(); i++)
    {
        delete budget[i];
    }
}
