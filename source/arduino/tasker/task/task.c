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
The idea is that signals are processed in the receiver task when
it runs.  It's up to the user to eval all signals, or.. few
of them, or however they want.


 */
//////////////////////////////////////////////////////
#include <stdint.h>
#include <string.h>

#include "task.h"

static TaskStruct TaskTable[TASK_MAX_TASK];

///////////////////////////////////////
//Init task table with default values
void Task_Init(void)
{
	uint8_t i;
	for (i = 0 ; i < TASK_MAX_TASK ; i++)
	{
		memset(TaskTable[i].name, 0x00, TASK_NAME_LENGTH);
		TaskTable[i].taskFunction = NULL_PTR;
		TaskTable[i].timer = 0;
		TaskTable[i].flagEnable = 0;
		TaskTable[i].flagRun = 0;
		TaskTable[i].index = i;
		TaskTable[i].initialTimeTick = 0;
		TaskTable[i].taskMessageWaiting = 0;
		Task_ClearAllMessages(i);
	}
}

/////////////////////////////////////////////
//Task_AddTask
//Add a task to the task table.  needs to fit
//within the size, priority = index in the table
//*task is the task to run
int Task_AddTask(char* name, void (*taskFunction) (void), uint16_t time, uint8_t priority)
{
	//check priority and if the task function is set up
	if (priority < TASK_MAX_TASK)
	{
		if (TaskTable[priority].taskFunction == NULL_PTR)
		{
			memset(TaskTable[priority].name, 0x00, TASK_NAME_LENGTH);
			strncpy(TaskTable[priority].name, name, TASK_NAME_LENGTH);

			TaskTable[priority].flagEnable = 1;					//task enable
			TaskTable[priority].flagRun = 0;					//set on timeout, polled in main
			TaskTable[priority].index = priority;				//
			TaskTable[priority].initialTimeTick = time;			//set the timeout
			TaskTable[priority].taskFunction = taskFunction;	//function pointer
			TaskTable[priority].timer = time;					//timeout

			TaskTable[priority].taskMessageWaiting = 0;
			Task_ClearAllMessages(priority);

			return 1;
		}

		//function pointer already assigned
		else
			return -1;
	}

	return -1;
}

///////////////////////////////////////
//loop through the task table until
//we hit taskFunction and reset all the
//values for that entry
//pass the task function as arg
int Task_RemoveTask(void (*taskFunction) (void))
{
	uint8_t i;
	for (i = 0 ; i < TASK_MAX_TASK ; i++)
	{
		if (taskFunction == TaskTable[i].taskFunction)
		{
			memset(TaskTable[i].name, 0x00, TASK_NAME_LENGTH);
			TaskTable[i].taskFunction = NULL_PTR;
			TaskTable[i].timer = 0;
			TaskTable[i].flagEnable = 0;
			TaskTable[i].flagRun = 0;
			TaskTable[i].index = i;
			TaskTable[i].initialTimeTick = 0;

			TaskTable[i].taskMessageWaiting = 0;
			Task_ClearAllMessages(i);

			return i;
		}
	}

	return -1;		//invalid function
}

void Task_EnableTask(uint8_t taskIndex)
{
	if (taskIndex < TASK_MAX_TASK)
	{
		//check null function
		if (TaskTable[taskIndex].taskFunction != NULL_PTR)
		{
			TaskTable[taskIndex].flagEnable = 1;
			TaskTable[taskIndex].flagRun = 0;
			TaskTable[taskIndex].initialTimeTick = TaskTable[taskIndex].timer;
		}
	}
}

void Task_DisableTask(uint8_t taskIndex)
{
	if (taskIndex < TASK_MAX_TASK)
	{
		//check null function
		if (TaskTable[taskIndex].taskFunction != NULL_PTR)
		{
			TaskTable[taskIndex].flagEnable = 0;
			TaskTable[taskIndex].flagRun = 0;
			TaskTable[taskIndex].initialTimeTick = 0;
		}
	}
}

////////////////////////////////////////////////
//update the timeout value and the initial tick
void Task_RescheduleTask(uint8_t taskIndex, uint16_t updatedTime)
{
	if (taskIndex < TASK_MAX_TASK)
	{
		//check null function
		if (TaskTable[taskIndex].taskFunction != NULL_PTR)
		{
			TaskTable[taskIndex].timer = updatedTime;
			TaskTable[taskIndex].initialTimeTick = updatedTime;
		}
	}
}



////////////////////////////////////////////
int Task_GetIndexFromName(char* name)
{
	uint8_t i;
	for (i = 0 ; i < TASK_MAX_TASK ; i++)
	{
		//get the index from name, set the signal and value
		if (!strcmp(name, TaskTable[i].name))
		{
			return i;
		}
	}

	return -1;		//invalid name
}




/////////////////////////////////////////
//Main loop routine.  should never
//stop.  called in main.
void Task_StartScheduler(void)
{
	uint8_t i = 0;

	while (1)
	{
		//run the tasker
		for (i = 0; i < TASK_MAX_TASK ; i++)
		{
			if (TaskTable[i].taskFunction != NULL_PTR)
			{
				if (TaskTable[i].flagEnable == 1)
				{
					if (TaskTable[i].flagRun == 1)
					{
						//task to run
						TaskTable[i].flagRun = 0;

						TaskTable[i].taskFunction();
						break;
					}
				}
			}
		}
	}
}


//////////////////////////////////////
//Call this function in the timer isr
//Configure the timer to rollover
//at same speed, 2x, 3x... as the rate
//we want to run the tasker.

//Loop over the task table, decrementing
//the timeTick and set the run flag for
//those timers that have rolled over

void Task_TimerISRHandler(void)
{
	uint8_t i;
	for (i = 0 ; i < TASK_MAX_TASK ; i++)
	{
		//check task enabled, function != NULL
		if ((TaskTable[i].flagEnable == 1) && (TaskTable[i].taskFunction != NULL_PTR))
		{
			if (!TaskTable[i].initialTimeTick)
			{
				//set the run flag polled in main,
				//reset the timer tick value
				TaskTable[i].flagRun = 1;
				TaskTable[i].initialTimeTick = TaskTable[i].timer;
			}

			else
				TaskTable[i].initialTimeTick--;
		}
	}
}


////////////////////////////////////////////
//Clear all messages in the TaskTable array
//for a given element
int Task_ClearAllMessages(uint8_t element)
{
	uint8_t i;

	if (element < TASK_MAX_TASK)
	{
		TaskTable[element].taskMessageWaiting = 0;

		for (i = 0 ; i < TASK_MESSAGE_SIZE ; i++)
		{
			TaskTable[element].taskMessage[i].signal = TASK_SIG_NONE;
			TaskTable[element].taskMessage[i].value = 0x00;
		}

		return 1;
	}

	return -1;		//invalid index
}


/////////////////////////////////////////
//Send message to a task.
//puts a message into taskMessage array
//at element taskNumMessage++
//returns the number of messages in the queue
//after posting the message.  returns -1 if error
//
int Task_SendMessage(uint8_t index, TaskMessage message)
{
	if (index < TASK_MAX_TASK)
	{
		//in the range and task is enabled
		if (TaskTable[index].taskMessageWaiting < (TASK_MESSAGE_SIZE - 1) &&
				(TaskTable[index].flagEnable == 1))
		{
			TaskTable[index].taskMessage[TaskTable[index].taskMessageWaiting].signal = message.signal;
			TaskTable[index].taskMessage[TaskTable[index].taskMessageWaiting].value = message.value;
			TaskTable[index].taskMessageWaiting++;

			return TaskTable[index].taskMessageWaiting;
		}
	}

	return -1;		//invalid index
}



/////////////////////////////////////////////

int Task_GetNumMessageWaiting(uint8_t index)
{
	if (index < TASK_MAX_TASK)
	{
		return TaskTable[index].taskMessageWaiting;
	}

	return -1;		//invalid index
}


////////////////////////////////////////////////////
//Dequeue a message if taskmessagewaiting is > 0
//Decrement taskMessageWaiting, load the msg ptr,
//and return the status.  Returns the current num
//messages waiting after dequeue, -1 if error (name not found)
//
int Task_GetNextMessage(uint8_t index, TaskMessage *msg)
{
	if (index < TASK_MAX_TASK)
	{
		if (TaskTable[index].taskMessageWaiting > 0)
		{
			msg->signal = TaskTable[index].taskMessage[TaskTable[index].taskMessageWaiting - 1].signal;
			msg->value = TaskTable[index].taskMessage[TaskTable[index].taskMessageWaiting - 1].value;

			//post decrement if using a while loop on reader side
			return TaskTable[index].taskMessageWaiting--;
		}

		else
		{
			return 0;
		}
	}

	return -1;		//invalid index
}
