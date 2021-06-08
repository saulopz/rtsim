#ifndef CONFIG_H_
#define CONFIG_H_

#define MAX_X 800
#define MAX_Y 600
#define VMODE GFX_AUTODETECT_WINDOWED

#define PERIODIC 0
#define SIMPLE_SPORADIC_SERVER 1
#define DEFERRABLE_SERVER 2
#define POOLING_SERVER 3

#define RATE_MONOTONIC 0
#define EARLIEST_DEADLINE_FIRST 1

#define MAX_TASK 10
#define COLOR_TIME_LINE makecol(0, 0, 0)
#define COLOR_LINE_TASK makecol(0, 0, 0)
#define COLOR_LINE_BUDGET makecol(0, 0, 255)
#define TRANSPARENT makecol(255, 0, 255)

#define SUSPENDED 0
#define READY 1
#define EXECUTED 2

#define IDLE 0
#define BUSY 1

#define BUDGET_SIZE 30

#endif /*CONFIG_H_*/
