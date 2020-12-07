Prod-Cons-fork:
Semaphore Solution to Producer Consumer Problem using System V IPC(Semaphore Objects in Kernel Space): 
1. Program starts with 20 empty slots and 0 full slots in a shared circular buffer. 
2. The producer keep track of number of free slots and writes a character to the the empty slots. 
3. The consumer keep track of the number of filled slots and reads the shared buffer and save it in a readbuffer.
4. Both producer and consumer are forked from the main program and perform the job in an infinite loop.

Prod-Cons-threads:
Semaphore Solution to Producer Consumer Problem using User Space semaphore objects: 
1. Program starts with 20 empty slots and 0 full slots in a shared circular buffer. 
2. There is a buffer initialization and counting semaphore initializations.
3. The producer keep track of number of free slots and writes a character to the the empty slots. 
4. The consumer keep track of the number of filled slots and reads the shared buffer and save it in a readbuffer.
5. Both producer and consumer threads are created from the main program and perform the job in an infinite loop.


