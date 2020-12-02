/* Midhun Nair      Mid666
 * Counting Semaphore 
 * Producer-Consumer 
 *
 */

#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<sys/types.h>
#include<signal.h>
#include<string.h>
#include<errno.h>
#include<sys/wait.h>

void SIGINT_HANDLER(int);

void producer(void);
void consumer(void);

typedef struct shm_area 
{
	unsigned int rd_index;
	unsigned int wr_index;
	unsigned int max_buffer_size;
	char buffer[20];
	unsigned int filled_slots;
	unsigned int free_slots;

}UserSpaceSharedMemory;

UserSpaceSharedMemory *sh_mem;

union semun
{
	int val;
	unsigned short *array;   
};

key_t USkey = 101;
key_t SSkey = 102;

int ret, shmid, semid;	

static char value = '0';
char readbuffer[20];

int main()
{
	int child_no = 0;  
	pid_t retf, retw;
	int status;
	
	union semun u1;
	unsigned short arr[3] = {20,1,0};
	u1.array = arr;

	struct sembuf sops[3];
	
	shmid = shmget(USkey, sizeof(UserSpaceSharedMemory), IPC_CREAT|0666);
	if(shmid < 0 )
	{
		if(errno == EEXIST)
			printf("Already exists\n");

		perror("Err: shmget Failed\n");
		exit(1);
	}

	sh_mem = shmat(shmid, 0, 0);

	semid = semget(SSkey, 3, IPC_CREAT|0666); 
	if(semid < 0 )
	{
		perror("Err: semget Failed\n");
		exit(1);
	}

	ret = semctl(semid, 0, SETALL, u1);
	if(ret < 0)
	{
		perror("Err: semctl");
		exit(2);
	}

	sh_mem->rd_index = 0;
  	sh_mem->wr_index = 0;
  	sh_mem->max_buffer_size = 20;
	sh_mem->filled_slots = 0;
	sh_mem->free_slots = 20;

	
	memset(sh_mem->buffer, 0, 20);
	memset(readbuffer, 0, 20);

	printf("Before Starting:  Buffer = %s\nFree slots = %d\nfilled slots = %d\nwr_index = %d\nrd_index = %d\n\n",\
				sh_mem->buffer, sh_mem->free_slots, sh_mem->filled_slots, sh_mem->wr_index, sh_mem->rd_index);
	
	
	/********** producer-consumer ************/
	
	while(child_no++<2)
	{
		retf = fork();
		if(retf < 0)
		{
			perror("Err: fork Failed");
			exit(3);
		}

		if(retf == 0)
		{
			signal(SIGINT, SIGINT_HANDLER);
			
			if(child_no == 1)
			{
				while(1)
				{						
					usleep(800000);
					/*******LOCK START******/
					
					 /* decrement 1, initial value: 20
					  * To keep track of the number of free slots.
		 			  * if(semaphore_count_value i.e no of free slots > 0) THEN
		  			  * Proceed to try access critical section and write to one of the free slot*/

					sops[0].sem_num = 0;   /* operate on semaphore 0 */   
					sops[0].sem_op = -1;   
					sops[0].sem_flg = 0;
					
					/* decrement 1, initial value: 1
					 * Enter critical section if semaphore_count_value 1 */

					sops[1].sem_num = 1;   /* operate on semaphore 1 */
					sops[1].sem_op = -1;   
					sops[1].sem_flg = 0;

					semop(semid, sops, 2);  /* Do both operations ATOMICALLY */
					
					/********LOCK END*******/
									
							
					/***CRITICAL SECTION START***/
					
					producer();
					
					/***CRITICAL SECTION END***/

					
				printf("After Producer CS:  Buffer = %s\nFree slots = %d\nfilled slots = %d\nwr_index = %d\nrd_index = %d\n\n",\
					       sh_mem->buffer, sh_mem->free_slots, sh_mem->filled_slots, sh_mem->wr_index, sh_mem->rd_index);

					
					/********** UNLOCK START**********/

					/*Getting out of critical section */
					sops[1].sem_num = 1;
					sops[1].sem_op = +1;
					sops[1].sem_flg = 0;

					/* To make consumer aware of how much character left to read */
					sops[2].sem_num = 2;
					sops[2].sem_op = +1;
					sops[2].sem_flg = 0;
					
					semop(semid, (sops+1), 2);

					/********* UNLOCK END *********/

				}
				exit(0);
			}
			
			if(child_no == 2)
			{
				usleep(800000);
				while(1)
				{
					/********** LOCK START**********/

		 			/* decrement 1, initial value: Depend on filled slots
		  			* To keep track of the number of filled slots.
		  			* if(semaphore_count_value i.e no of filled > 0) THEN
		  			* Proceed to try access critical section and read the filled slot*/

					sops[2].sem_num = 2;   /* operate on semaphore 2 */   
					sops[2].sem_op = -1;   
					sops[2].sem_flg = 0;
		
					/* decrement 1, initial value: 1
		 			* Enter critical section if semaphore_count_value 1 */

  					sops[1].sem_num = 1;   /* operate on semaphore 1 */
            				sops[1].sem_op = -1;   
					sops[1].sem_flg = 0;
		
					semop(semid, (sops+1), 2);  /* Do both operations ATOMICALLY */

					/*********** LOCK END**********/
					
					/*****CRITICAL SECTION START*********/
					consumer();
					/*****CRITICAL SECTION END*********/

					
				printf("After Consumer CS:  Buffer = %s\nFree slots = %d\nfilled slots = %d\nwr_index = %d\nrd_index = %d\n\n",\
						sh_mem->buffer, sh_mem->free_slots, sh_mem->filled_slots, sh_mem->wr_index, sh_mem->rd_index);
					/*****UNLOCK START*****/
					
					sops[1].sem_num = 1;
					sops[1].sem_op = +1;
					sops[1].sem_flg = 0;

					/* To make producer aware of how much free slots are available to write */
					sops[0].sem_num = 0;
					sops[0].sem_op = +1;
					sops[0].sem_flg = 0;

					semop(semid, sops, 2);

					/*****UNLOCK END******/

				}
				exit(0);
			}
		}
	}

	if(retf > 0)
	{
		int flag_normal = 0;
		while(1)
		{
			retw = waitpid(-1, &status, 0);
			if(retw > 0)
			{
				if(WIFEXITED(status))
				{
					flag_normal = 1;	
					if(WEXITSTATUS(status)==0)
					{
						printf("Normal Successful Termination\n");
					}
					else
					{
						printf("Normal Unsuccessful Termination\n");
					}
				}
				else
				{
					flag_normal = 0;
					printf("Abnormal Termination\n");
				}
			}
			if(retw < 0)
				break;
		}
		exit(0);
	}
}

void producer()
{
	value++;
	if((sh_mem->filled_slots) < (sh_mem->max_buffer_size))
	{	
		sh_mem->buffer[sh_mem->wr_index] = value;
		printf("Producer Wrote to buffer  %s\n", sh_mem->buffer);
		(sh_mem->wr_index)++;			
		(sh_mem->filled_slots)++;
		(sh_mem->free_slots)--;

		if(sh_mem->wr_index == 20)
		{
			sh_mem->wr_index = 0;
			sh_mem->filled_slots = 0;
			sh_mem->free_slots = 20;	
		}

		if(value == 73)
			value = '0';

	}
	return;
}

void consumer()
{
	char readchar;
	if((sh_mem->filled_slots) != 0)
	{
		readchar = sh_mem->buffer[sh_mem->rd_index];
		readbuffer[sh_mem->rd_index] = readchar; 
		printf("Consumer Read from buffer  %s\n", readbuffer);
		(sh_mem->rd_index)++;			
		if(sh_mem->rd_index == 20)
			sh_mem->rd_index = 0;

	}
	return;
}

void SIGINT_HANDLER(int signum)
{
	memset(sh_mem->buffer, 0, sh_mem->max_buffer_size);
	shmctl(shmid, IPC_RMID, 0); 
	printf("Exiting...\nExited\n");
	exit(0);
}
















