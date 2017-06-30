#pragma once
#include "stdafx.h"


struct Queue {
	int head;
	int tail;
	int elementSise;
	int capacity;
	int Count;
	char* data;
};

bool CreateQueue(Queue* queue, size_t elementSise, int Maxsise);
bool QueuePush(Queue* stack, void* item);
void QueuePop(Queue* stack);
bool QueueisEmpty(Queue* stack);
void* QueuePeek(Queue* stack);
size_t QueueGetsise(size_t elementSise, int Maxsise); 