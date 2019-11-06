#include <stddef.h>
#include <stdlib.h>

#include "queue.h"
#include "sem.h"
#include "thread.h"

struct semaphore {
	int count;
	struct queue* wthreads; // Threads waiting to be executed/blocked threads.
};

sem_t sem_create(size_t count)
{
	struct semaphore* sem = (struct semaphore*) malloc(sizeof(struct semaphore));

	sem->count = 0;
	sem->count = count;
	sem->wthreads = queue_create();
	return sem; 
}

int sem_destroy(sem_t sem)
{
	if(sem == NULL || queue_length(sem->wthreads) != 0)
		return -1;

	free(sem);

	return 0;
}

int sem_down(sem_t sem)
{
	enter_critical_section();

	if(sem == NULL)
		return -1;

	while(sem->count == 0)
	{
		queue_enqueue(sem->wthreads,(void*)pthread_self());
		thread_block();
	}

	if(sem->count > 0)
		sem->count -= 1;

	exit_critical_section();

	return 0;
}

int sem_up(sem_t sem)
{
	enter_critical_section();

	if(sem == NULL)
		return -1;

	if(sem->count == 0)
	{
		if(queue_length(sem->wthreads) != 0)
		{
			pthread_t tid;
			queue_dequeue(sem->wthreads,(void*)&tid);
			thread_unblock(tid);
		}
	}

	sem->count += 1;

	exit_critical_section();

	return 0;
}

int sem_getvalue(sem_t sem, int *sval)
{
	if(sem == NULL || sval == NULL)
		return -1;

	if(sem->count > 0)
		*sval = sem->count;

	if(sem->count == 0)
		*sval = -(queue_length(sem->wthreads));

	return 0;
}

