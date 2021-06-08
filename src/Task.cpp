#include "Task.h"
#include "config.h"
#include "TimeLine.h"
#include <string.h>
#include <stdio.h>

Task::Task(TimeLine *tl, int t, int p, float e, int c)
{
	timeLine = tl;
	type = t;
	period = p;
	priority = 0;
	time = 0.0;
	executed = 0.0;
	running = false;
	execution = e;
	color = c;
	SS = NULL;
	aperiodicIndex = 0;

	if (type == PERIODIC)
		strcpy(name, "T");
	else if (type == SIMPLE_SPORADIC_SERVER)
		strcpy(name, "SS");
	else if (type == DEFERRABLE_SERVER)
		strcpy(name, "DS");
	else if (type == POOLING_SERVER)
		strcpy(name, "PS");
	else
		strcpy(name, "UNKNOW");
	if (type != PERIODIC)
	{
		SS = new SporadicServer();
		budget = timeLine->addBudget(execution);
		budget->color = color;
	}
}

void Task::make()
{
	int auxPeriod = 0;
	orderAperiodic();
	image = create_bitmap((int)(timeLine->timeSize * 10), 50);
	clear_to_color(image, TRANSPARENT);
	line(image, 0, image->h - 1, image->w, image->h - 1, COLOR_LINE_TASK);
	for (int i = 0; i < timeLine->timeSize; i++)
	{
		int sizeTrace = 3;
		if ((auxPeriod == period) && (type != SIMPLE_SPORADIC_SERVER))
		{
			sizeTrace = 20;
			auxPeriod = 0;
		}
		line(image, i * MAX_TASK, image->h - 1, i * MAX_TASK, image->h - sizeTrace, COLOR_LINE_TASK);
		auxPeriod++;
	}
	bool auxPlace = false;
	for (unsigned int i = 0; i < aperiodic.size(); i++)
	{
		int placeY = 15;
		if (auxPlace)
			placeY = 25;
		line(image, (int)aperiodic[i]->r * 10,
			 image->h - 1, (int)aperiodic[i]->r * 10, placeY + 10, aperiodic[i]->color);
		char apString[255];
		sprintf(apString, "A:%.1f,%.1f", aperiodic[i]->r, aperiodic[i]->e);
		textout_ex(image, font, apString, (int)aperiodic[i]->r * 10, placeY, aperiodic[i]->color, TRANSPARENT);
		auxPlace = !auxPlace;
	}
	char strTask[20];
	sprintf(strTask, "%s(%d, %.1f)", name, period, execution);
	textout_ex(image, font, strTask, 5, 5, COLOR_LINE_TASK, TRANSPARENT);
}

void Task::addAperiodic(float r, float e, int color)
{
	aperiodic.push_back(new Aperiodic(r, e, color));
}

void Task::nextAperiodic()
{
	aperiodicIndex++;
}

Aperiodic *Task::getAperiodic()
{
	if (aperiodicIndex < (int)aperiodic.size())
	{
		return aperiodic[aperiodicIndex];
	}
	else
		return NULL;
}

void Task::orderAperiodic()
{
	for (unsigned int a = 0; a < aperiodic.size(); a++)
	{
		float lower = aperiodic[a]->r;
		int index = a;
		for (unsigned int b = a + 1; b < aperiodic.size(); b++)
		{
			if (aperiodic[b]->r < lower)
			{
				lower = aperiodic[b]->r;
				index = b;
			}
		}
		if ((int)a != index)
		{
			Aperiodic *swp = aperiodic[a];
			aperiodic[a] = aperiodic[index];
			aperiodic[index] = swp;
		}
	}
}

void Task::run()
{
	switch (type)
	{
	case PERIODIC:
		runPeriodic();
		break;
	case SIMPLE_SPORADIC_SERVER:
		runSampleSporadicServer();
		break;
	case DEFERRABLE_SERVER:
		runDeferrableServer();
		break;
	case POOLING_SERVER:
		runPoolingServer();
		break;
	}
}

void Task::runPeriodic()
{
	// If in init of period reset executed time
	if (periodPoint())
	{
		executed = 0.0;
	}
	// If executed time is higher or equal execution time, then stop running
	if (executed >= execution)
		running = false;
	// If im running and a task with higher priority begin to run
	if (running)
	{
		if (isHigherRunning())
			running = false;
	}
	else
	{
		// If im not running, but I not finish my execution
		if (executed < execution)
		{
			// if not other task with higher priority is running
			if (!isHigherRunning())
				running = true;
		}
	}
	//printf("e: %.1f p: %d\n", executed, period);
	if (running)
	{
		line(image, (int)(timeLine->time * 10), image->h - 2, (int)(timeLine->time * 10), image->h - 10, color);
		executed += 0.1;
	}
}

/**
TERMINOLOGIA

- t : instante de tempo atual.
- T : conjunto de n tarefas periodicas, independentes e preenptaveis
- PIs : prioridade arbitraria do servidor Ts
- Ts : Servidor esperadico
- Th : subconjunto de tarefas que tem mais alta prioridade que Ts
- BEGIN e END:
	- BEGIN : instante quando Th se torna de IDLE para BUSY
	- END : instante quando Th se torna IDLE novamente ou infinito
	se Th continua BUSY
	- O intervalo (BEGIN, END] eh o intervalo BUSY
- tb : inicio de um intervalo ocupado
- tr : ultimo instante do recarregamento de Ts
- tc : proximo instante de recarregamento agendado de Ts
- te : instante efetivo de recarregamento de Ts
- tf : primeiro instante depois de tr que Ts comeca sua execucao
- es : computacao (execution) do servidor esporadico

SIMPLE SPORADIC SERVER EM UM SISTEMA DE PRIORIDADE FIXA

- REGRAS DE CONSUMO: a qualquer tempo t, depois de tr, Ts consome seu BUDGET
	em uma taxa de uma unidade por unidade de execucao ate que o BUDGET
	esteja vazio nas seguintes situacoes:

 C1 : Ts esta executando ou
 C2 : Ts foi executado desde tr e END < t. (END < t -> Th esta IDLE).

- REGRAS DE RECARREGAMENTO: tr = t sempre que o BUDGET eh recarregado com
	es pelo escalonador:

 R1 : Inicialmente, tr = te = 0 e tc = ps (sendo que o sistema comeca no tempo 0)
 R2 : No tempo tf,
	- Se END = tf, te = max(tr, BEGIN)
	- Se END < tf, te = tf
	O proximo instante de recarregamento tc = te + ps
 R3 : O proximo recarregamento ocorrera em tc exceto
	(a) Se tc < tf, quando o BUDGET eh recarregado logo que for exaurido
	(b) se T ficar IDLE antes de tc e comeca um novo intervalo ocupado em
	tb, entao o BUDGET eh recarregado com min(tc, tb)
*/

void Task::runSampleSporadicServer()
{
	bool replenished = false;
	SS->t = timeLine->time;
	SS->ps = period;
	// Compute (BEGIN, END]
	if (SS->ThState == IDLE)
	{
		if (isHigherRunning())
		{
			SS->ThState = BUSY;
			SS->BEGIN = SS->t;
		}
	}
	else if (SS->ThState == BUSY)
	{
		if (!isHigherRunning())
		{
			SS->ThState = IDLE;
			SS->END = SS->t;
		}
		else
			SS->END = (float)timeLine->timeSize;
	}
	// Verify if all system is IDLE
	if (isHigherRunning() || isLowerRunning() || running || lowerTaskWillRun() || (executed < execution))
	{
		SS->TState = BUSY;
	}
	else
		SS->TState = IDLE;

	// Role R1
	if (SS->t == 0.0)
	{
		SS->te = 0;
		SS->tr = 0;
		SS->tc = SS->ps;
		budget->replenishAll();
		replenished = true;
		printf("Sporadic Server initialized (%s%d)...\n", name, priority);
	}
	// Role R3
	if (SS->tc < SS->tf)
	{
		// Role R3 (a)
		if (budget->exhausted())
		{
			budget->replenishAll();
			SS->tr = SS->t;
			SS->te = SS->t;
			SS->tc = SS->te + SS->ps;
			replenished = true;
		}
	}
	else if ((SS->TState == IDLE) && (SS->t < SS->tc))
	{	//&& (executed >= execution)) {
		// Role R3 (b)
		float nextBusyTime = getNextBusyTime();
		if (SS->tc > nextBusyTime)
			SS->tc = nextBusyTime;
	}
	if ((long)(SS->t * 10) == (long)(SS->tc * 10))
	{
		budget->replenishAll();
		SS->tr = SS->t;
		SS->te = SS->t;
		SS->tc = SS->te + SS->ps;
		replenished = true;
	}

	// Test if have an aperiod task to run
	running = false;
	Aperiodic *ap = getAperiodic();
	if (ap != NULL)
	{
		// if time of task begin
		if ((SS->t >= ap->r) && (SS->state != EXECUTED))
		{
			if (SS->state == SUSPENDED)
			{
				SS->state = READY;
			}
			// If highless tasks is IDLE
			if ((SS->ThState == IDLE) && (SS->state == READY))
			{
				SS->state = EXECUTED;
				SS->tf = SS->t;
				printf("Start run A: r %.1f at t %.1f\n", ap->r, SS->t);
			}
		}
		// If task began and Highless tasks is IDLE
		if (SS->ThState == IDLE)
		{
			if (SS->state == EXECUTED)
			{
				if (!budget->exhausted() && (executed < ap->e))
				{
					running = true;
				}
			}
		}
		// Run task
		if (running)
		{
			if (ap->initTime < 0)
				ap->initTime = SS->t;
			line(image, (int)(SS->t * 10), image->h - 2, (int)(SS->t * 10), image->h - 10, ap->color);
			executed += 0.1;
		}
		if (executed >= ap->e)
		{
			if (ap->endTime < 0)
				ap->endTime = SS->t;
			if (replenished)
			{
				SS->state = SUSPENDED;
				executed = 0.0;
				nextAperiodic();
			}
		}
	}
	// Role C1
	if (running)
	{
		budget->consume();
	}
	else
	{
		// Role C2
		if ((SS->state == EXECUTED) && (SS->ThState == IDLE))
		{ //(SS->END < SS->t)) {
			budget->consume();
		}
	}
	// Role R2
	if ((long)(SS->t * 10) == (long)(SS->tf * 10))
	{
		if ((long)(SS->END * 10) == (long)(SS->tf * 10))
		{
			if (SS->BEGIN > SS->tr)
				SS->te = SS->BEGIN;
			else
				SS->te = SS->tr;
		}
		else if (SS->END < SS->tf)
		{
			SS->te = SS->tf;
		}
		SS->tc = SS->te + SS->ps;
	}
}

void Task::runDeferrableServer()
{
	SS->t = timeLine->time;
	SS->ps = period;
	if (SS->t == 0.0)
	{
		SS->tc = SS->ps;
	}
	if (running)
	{
		budget->consume();
	}
	if ((long)(SS->t * 10) == (long)(SS->tc * 10))
	{
		budget->replenishAll();
		SS->tc += SS->ps;
	}
	Aperiodic *ap = getAperiodic();
	if (ap != NULL)
	{
		// if time of task begin
		if ((SS->t >= ap->r) && (SS->state != EXECUTED))
		{
			if (SS->state == SUSPENDED)
			{
				SS->state = READY;
			}
			// If highless tasks is IDLE
			if ((SS->ThState == IDLE) && (SS->state == READY))
			{
				SS->state = EXECUTED;
				running = true;
				printf("Start run A: r %.1f at t %.1f\n", ap->r, SS->t);
			}
		}
		// If task began and Highless tasks is IDLE
		if (SS->ThState == IDLE)
		{
			if (SS->state == EXECUTED)
			{
				if (!budget->exhausted() && (executed < ap->e))
				{
					running = true;
				}
				else
					running = false;
			}
		}
		else
			running = false;
		if (running)
		{
			if (ap->initTime < 0)
				ap->initTime = SS->t;
			line(image, (int)(SS->t * 10), image->h - 2, (int)(SS->t * 10), image->h - 10, ap->color);
			executed += 0.1;
		}
		if (executed >= ap->e)
		{
			running = false;
			if (ap->endTime < 0)
				ap->endTime = SS->t;
			SS->state = SUSPENDED;
			executed = 0.0;
			nextAperiodic();
		}
	}
}

void Task::runPoolingServer()
{
	SS->t = timeLine->time;
	SS->ps = period;
	if (SS->t == 0.0)
	{
		SS->tc = SS->ps;
	}
	if (running)
	{
		budget->consume();
	}
	if ((long)(SS->t * 10) == (long)(SS->tc * 10))
	{
		budget->replenishAll();
		SS->tc += SS->ps;
	}
	Aperiodic *ap = getAperiodic();
	if (ap != NULL)
	{
		// if time of task begin
		if ((SS->t >= ap->r) && (SS->state != EXECUTED))
		{
			if (SS->state == SUSPENDED)
			{
				SS->state = READY;
			}
			// If highless tasks is IDLE
			if ((SS->ThState == IDLE) && (SS->state == READY))
			{
				SS->state = EXECUTED;
				running = true;
				printf("Start run A: r %.1f at t %.1f\n", ap->r, SS->t);
			}
		}
		// If task began and Highless tasks is IDLE
		if (SS->ThState == IDLE)
		{
			if (SS->state == EXECUTED)
			{
				if (!budget->exhausted() && (executed < ap->e))
				{
					running = true;
				}
				else
					running = false;
			}
		}
		else
			running = false;
		if (running)
		{
			if (ap->initTime < 0)
				ap->initTime = SS->t;
			line(image, (int)(SS->t * 10), image->h - 2, (int)(SS->t * 10), image->h - 10, ap->color);
			executed += 0.1;
		}
		if (executed >= ap->e)
		{
			running = false;
			if (ap->endTime < 0)
				ap->endTime = SS->t;
			SS->state = SUSPENDED;
			executed = 0.0;
			nextAperiodic();
		}
	}
	if (!running)
		budget->consumeAll();
}

bool Task::isHigherRunning()
{
	bool isRunning = false;
	for (int i = 0; i < (priority - 1); i++)
	{
		if (timeLine->task[i]->running)
			isRunning = true;
	}
	return isRunning;
}

bool Task::isLowerRunning()
{
	bool isRunning = false;
	for (unsigned int i = priority; i < timeLine->task.size(); i++)
	{
		if (timeLine->task[i]->running)
			isRunning = true;
	}
	return isRunning;
}

float Task::getNextBusyTime()
{
	float timeAux = timeLine->timeSize;
	if (!isHigherRunning() && !isLowerRunning())
	{
		for (unsigned int i = 0; i < timeLine->task.size(); i++)
		{
			if (this != timeLine->task[i])
			{
				int t = 0;
				while (t <= timeLine->time)
				{
					t += timeLine->task[i]->period;
				}
				if (t < (int)timeAux)
					timeAux = t;
			}
		}
	}
	return timeAux;
}

bool Task::lowerTaskWillRun()
{
	bool willRun = false;
	for (unsigned int i = priority; i < timeLine->task.size(); i++)
	{
		if (timeLine->task[i]->executed < timeLine->task[i]->execution)
		{
			willRun = true;
		}
	}
	return willRun;
}

void Task::showAperiodicResponseTime()
{
	for (unsigned int i = 0; i < aperiodic.size(); i++)
	{
		printf("(A: r=%.1f, e=%.1f) - RT:%.1f\n", aperiodic[i]->r, aperiodic[i]->e, aperiodic[i]->getResponseTime());
	}
}

bool Task::periodPoint()
{
	return ((int)(((long)(timeLine->time * 10)) % (period * 10)) == 0);
}

Task::~Task()
{
	for (int i = 0; i < (int)aperiodic.size(); i++)
	{
		delete aperiodic[i];
	}
	destroy_bitmap(image);
}
