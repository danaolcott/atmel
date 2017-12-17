/*
 * task.c
 *
 *  Created on: Nov 25, 2017
 *      Author: danao
 */
///////////////////////////////////////////////////////
/*
Simple Tasker using a timer and a while loop.  One should
be able to implement this on any processor that has a timer
with ability to produce an interrupt at a known tick rate.

How to use:

On the main program, initialize timer and other hardware
In the timer isr, call the following function: Task_TimerISRHandler()

In the main program, start the Scheduler using the following: Task_StartScheduler()

Initialize tasks using Task_AddTask() - requires name, function, period, priority
If using signals, update the TaskSignal_t values in task.h.  it would be better to
pass a pointer to a signal table to make it so tasks could have thier own signal list.

See example in main for how to implement.

 */
//////////////////////////////////////////////////////


#ifndef TASK_TASK_H_
#define TASK_TASK_H_

#include <stdint.h>			//uint32_t..etc

#define TASK_MAX_TASK		8
#define NULL_PTR			((void *)0)
#define TASK_NAME_LENGTH	8

#define TASK_MESSAGE_SIZE	8


typedef enum
{
	TASK_SIG_NONE,		//do nothing
	TASK_SIG_ON,		//led on
	TASK_SIG_OFF,		//led off
	TASK_SIG_TOGGLE,	//led toggle
    TASK_SIG_BUTTON,    //signal from a button press
	TASK_SIG_LAST,

}TaskSignal_t;


typedef struct
{
	TaskSignal_t signal;
	uint16_t value;
}TaskMessage;


//task structure
typedef struct
{
	char name[TASK_NAME_LENGTH];//task name, null terminated for ref
	uint16_t initialTimeTick;	//countdown value
	uint16_t timer;				//frequency to run task
	uint8_t flagRun;			//running or not running
	uint8_t flagEnable;			//enable task
	uint8_t index;				//index in the task table

	//task functions, signals, etc
	void (* taskFunction) (void);				//function pointer - function to run
	uint8_t taskMessageWaiting;					//how many meessages waiting
	TaskMessage taskMessage[TASK_MESSAGE_SIZE];	//task message queue - unwind when task runs

}TaskStruct;



//function prototypes
void Task_Init(void);
int Task_AddTask(char* name, void (*taskFunction) (void), uint16_t time, uint8_t priority);
int Task_RemoveTask(void (*taskFunction) (void));
void Task_EnableTask(uint8_t taskIndex);
void Task_DisableTask(uint8_t taskIndex);
void Task_RescheduleTask(uint8_t taskIndex, uint16_t updatedTime);

int Task_GetIndexFromName(char* name);

void Task_StartScheduler(void);
void Task_TimerISRHandler(void);

//messages
int Task_ClearAllMessages(uint8_t element);					//helper function on init/remove, etc
int Task_SendMessage(uint8_t index, TaskMessage message);
int Task_GetNumMessageWaiting(uint8_t index);
int Task_GetNextMessage(uint8_t index, TaskMessage *msg);




#endif /* TASK_TASK_H_ */
