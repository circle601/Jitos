#include "stdafx.h"
#include "Queue.h"


bool CreateQueue(Queue* queue, size_t elementSise, int Maxsise) {
	int capacity = Maxsise / elementSise;
	if (capacity <= 0) return false;
	queue->capacity = capacity;
	queue->elementSise = elementSise;
	queue->head = 0;
	queue->tail = 0;

}

static inline int GetOfset(Queue* queue,int item) {
	return  item * queue->elementSise;
}


bool QueuePush(Queue* stack, void* item) {
	int head = stack->head;
	int tail = stack->tail;

	if (head < tail && (head + 1) == tail) {
		return false;
	}
	if (head >= stack->capacity) {
		head = 0;
		if (tail == 0) {
			return false;
		}
	}
	int ofset = GetOfset(stack, stack->head);
	memcpy(&(stack->data) + ofset, item, stack->elementSise);
	stack->Count++;
	stack->head++;
	return true;
}


void QueuePop(Queue* stack) {

	int head = stack->head;
	int tail = stack->tail;
	if (tail == head) {
		return;
	}
	if (tail >= stack->capacity) {
		tail = -1; //todo check if i got this part correectr for memory
		if (head == 0) {
			return;
		}
	}
	stack->tail++;
	stack->Count--;
}


bool QueueisEmpty(Queue* stack) {
	int head = stack->head;
	int tail = stack->tail;
	if (tail == head) {
		return true;
	}
	return false;
}


void* QueuePeek(Queue* stack) {
	int head = stack->head;
	int tail = stack->tail;
	if (tail == head) {
		return NULL;
	}
	if (tail >= stack->capacity) {
		tail = 0;
		if (head == 0) {
			return NULL;
		}
	}
	int ofset = GetOfset(stack, stack->tail);
	return &(stack->data) + ofset;
}



size_t QueueGetsise(size_t elementSise, int Maxsise) {
	return sizeof(Queue) + elementSise * Maxsise;
}



