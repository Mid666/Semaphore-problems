/* Midhun Nair
 * producer and consumer problem using threads
 * Counting Semaphore-User space semaphore objects
 * Date: 06/12/2020
 *
 */


#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<semaphore.h>
#include<errno.h>


void *producer(void *);
void *consumer(void *);

typedef struct shared_memory
{
	int free_slots;
	int filled_slots;
	char buffer[20];
	int max_buffer_size;
	int wr_index;
	int rd_index;
}shm;

shm *shma;

static int buffer_init(shm *, int);

static int semaphore_init(int filled, int free, int sync);


static sem_t freeslots;
static sem_t filledslots;
static sem_t binsync;

pthread_t thid1, thid2;

static char value = '0';
char readbuffer[20];


int main()
{
	void *(*thread[2])(void *) = {producer, consumer};
	
	int ret;
	int ret_th;
	
	shma = (shm *)malloc(sizeof(shm));

	ret = buffer_init(shma, 20);
	if(ret!=0)
	{
		perror("buff init error");
		exit(1);
	}

	ret = semaphore_init(0, 20, 1);
	if(ret!=0)
	{
		perror("sem init error");
		exit(1);
	}
	
	ret = pthread_create(&thid1, NULL, thread[0], (void *)shma);
	if(ret == EINVAL)
		perror("Invalid attribute settings");
	if(ret == EPERM)
		perror("Insufficient resource");
	
	ret = pthread_create(&thid2, NULL, thread[1], (void *)shma);
	if(ret == EINVAL)
		perror("Invalid attribute settings");
	if(ret == EPERM)
		perror("Insufficient resource");

	pthread_join(thid1, NULL);
	pthread_join(thid2, NULL);

	exit(0);
	
}


static int buffer_init(shm *shmem, int max_buf_size)
{

	if(max_buf_size == 0 || shmem == NULL)
		return 1;
	
	shmem->rd_index = 0;
	shmem->wr_index = 0;
  	shmem->max_buffer_size = max_buf_size;
	shmem->filled_slots = 0;
	shmem->free_slots = max_buf_size;
		
	return 0;
}

static int semaphore_init(int init_filled, int init_free, int sync)
{	
	int rets;
	rets = sem_init(&freeslots, 0, init_free);	
	if(rets!=0)
	{
		printf("sem init error\n");
		return errno;
	}
	
	rets = sem_init(&filledslots, 0, init_filled);	
	if(rets!=0)
	{
		printf("sem init error\n");
		return errno;
	}

	rets = sem_init(&binsync, 0, sync);	
	if(rets!=0)
	{
		printf("sem init error\n");
		return errno;
	}

	return 0;
}



void *producer(void *arg)
{
	shm *sh_mem =  (shm *)arg;

	while(1)
	{	
		usleep(600000);
		
		sem_wait(&binsync);
		sem_wait(&freeslots);
		value++;
		
		if((sh_mem->filled_slots) < (sh_mem->max_buffer_size))
		{	
			sh_mem->buffer[sh_mem->wr_index] = value;
			printf("Producer Wrote to  buffer  %s\n", sh_mem->buffer);
			(sh_mem->wr_index)++;			
			(sh_mem->filled_slots)++;
			(sh_mem->free_slots)--;

			if(value == 73)
				value = '0';
		}

		sem_post(&filledslots);
	}
}

void *consumer(void *arg)
{
	shm *sh_mem =  (shm *)arg;

	while(1)
	{
		usleep(600000);
		
		sem_wait(&filledslots);
				
		char readchar;
		if((sh_mem->filled_slots) != 0)
		{
			readchar = sh_mem->buffer[sh_mem->rd_index];
			readbuffer[sh_mem->rd_index] = readchar; 
			printf("Consumer Read from buffer  %s\n", readbuffer);
			(sh_mem->rd_index)++;			
			if(sh_mem->rd_index == 20)
				sh_mem->rd_index = 0;
				
			if(sh_mem->wr_index == 20)
			{
				sh_mem->wr_index = 0;
				sh_mem->filled_slots = 0;
				sh_mem->free_slots = 20;	
			}

		}
		
		sem_post(&binsync);
		sem_post(&freeslots);	
	}

}



