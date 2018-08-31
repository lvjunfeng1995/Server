/*************************************************************************
    > File Name: timer.c
    > Author: lvjunfeng
    > Mail: 249861170@qq.com 
    > Created Time: Thu Aug 23 15:43:33 2018
 ************************************************************************/

#include<stdio.h>
#include"server.h"


void cb_func(ND_SV_USER* user)
{
	INT32 fd = user->Fd;

	release_client(fd);
}

void addtowheel(ND_SV_TIMER* timer)
{
	pthread_mutex_lock(&Wheel_mutex);

	INT32 ts = timer->Time_Slot;

	if(!TimeWheel.SlotArrayPtr[ts])
	{
		TimeWheel.SlotArrayPtr[ts] = timer;
	}
	else
	{
		timer->NextPtr = TimeWheel.SlotArrayPtr[ts];
		TimeWheel.SlotArrayPtr[ts]->PrevPtr = timer;
		TimeWheel.SlotArrayPtr[ts] = timer;
	}

	pthread_mutex_unlock(&Wheel_mutex);
}

void delfromwheel(ND_SV_TIMER* timer)
{
	pthread_mutex_lock(&Wheel_mutex);

	INT32 ts = timer->Time_Slot;

	if(timer == TimeWheel.SlotArrayPtr[ts])
	{
		TimeWheel.SlotArrayPtr[ts] = TimeWheel.SlotArrayPtr[ts]->NextPtr;

		if(TimeWheel.SlotArrayPtr[ts])
		{
			TimeWheel.SlotArrayPtr[ts]->PrevPtr = NULL;
		}

		free(timer);
	}
	else
	{
		timer->PrevPtr->NextPtr = timer->NextPtr;

		if(timer->NextPtr)
		{
			timer->NextPtr->PrevPtr = NULL;
		}

		free(timer);
	}

	pthread_mutex_unlock(&Wheel_mutex);
}

ND_SV_TIMER* add_timer(INT32 timeout)
{
	if(timeout < 0)
	{
		return NULL;
	}

	INT32 ticks = 0;

	if(timeout < SLOT_INTERVAL)
	{
		ticks = 1;
	}
	else
	{
		ticks = timeout/SLOT_INTERVAL;
	}

	INT32 rotation = ticks/SLOT_NUM;

	INT32 ts = (TimeWheel.Cur_Slot + (ticks%SLOT_NUM))%SLOT_NUM;

	ND_SV_TIMER* timer = (ND_SV_TIMER *)malloc(sizeof(ND_SV_TIMER));
	
	timer->Rotation = rotation;
	timer->Time_Slot = ts;
	timer->NextPtr = NULL;
	timer->PrevPtr = NULL;
	timer->Cb_Func = cb_func;
	timer->NextPtr = NULL;
	timer->PrevPtr = NULL;

	addtowheel(timer);

	return timer;
}

void del_timer(ND_SV_TIMER* timer)
{
	if(!timer)
	{
		return;
	}

	delfromwheel(timer);
}

void tick()
{
	ND_SV_TIMER* tmp = TimeWheel.SlotArrayPtr[TimeWheel.Cur_Slot];

	while(tmp)
	{
		if(tmp->Rotation > 0)
		{
			tmp->Rotation --;
			tmp = tmp->NextPtr;
		}
		else
		{
			tmp->Cb_Func(tmp->User_Data);

			if(tmp == TimeWheel.SlotArrayPtr[TimeWheel.Cur_Slot])
			{
				TimeWheel.SlotArrayPtr[TimeWheel.Cur_Slot] = tmp->NextPtr;
				free(tmp);

				if(TimeWheel.SlotArrayPtr[TimeWheel.Cur_Slot])
				{
					TimeWheel.SlotArrayPtr[TimeWheel.Cur_Slot]->PrevPtr = NULL;
				}

				tmp = TimeWheel.SlotArrayPtr[TimeWheel.Cur_Slot];
			}
			else
			{
				tmp->PrevPtr->NextPtr = tmp->NextPtr;

				if(tmp->NextPtr)
				{
					tmp->NextPtr->PrevPtr = tmp->PrevPtr;
				}

				ND_SV_TIMER* tmp2 = tmp->NextPtr;
				free(tmp);

				tmp = tmp2;
			}
		}
	}

	TimeWheel.Cur_Slot = ++TimeWheel.Cur_Slot % SLOT_NUM;
}
