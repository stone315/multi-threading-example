
/**
 * dining.c
 * --------
 * The classic Dining Philosophers example. Allow a group of dining
 * philosophers to eat at a round sharing the forks between each person.
 * Each person needs both forks to eat.
 */
#include "thread_107.h"
#include <stdio.h>
#include<string.h>

#define NUM_DINERS 5
#define EAT_TIMES 3
/* Macros to conveniently refer to forks to left and right of each person */
#define LEFT(philNum) (philNum)
#define RIGHT(philNum) (((philNum)+1) % NUM_DINERS)
/*
 * Our main is creates a semaphore for every fork in an unlocked state
 * (one philosopher can immediately acquire each fork) and sets up the
 * numEating semaphore to only allow N-1 philosophers to try and grab
 * their forks. Each philosopher runs its own thread. They should
 * finish after getting their fill of spaghetti. By running with the
 15
 * -v flag, it will include the trace output from the thread library.
 */


static void* Philosopher(void* args);
static void Think(void);
static void Eat(Semaphore numEating, Semaphore leftFork, Semaphore rightFork);
Semaphore finish;



void main(int argc, char **argv)
{
    int i;
    char name[32];
    bool verbose = (argc == 2 && (strcmp(argv[1], "-v") == 0));
    InitThreadPackage(verbose);
    
    Semaphore fork[NUM_DINERS]; // semaphore to control access per fork
    Semaphore numEating; // to restrict contention for forks
    finish = SemaphoreNew("finish", 0);
    
    for (i = 0; i < NUM_DINERS; i++) { // Create all fork semaphores
        sprintf(name, "Fork %d", i);
        fork[i] = SemaphoreNew(name, 1); // all forks start available
    }
    numEating = SemaphoreNew("Num Eating", NUM_DINERS - 1);
    for (i = 0; i < NUM_DINERS; i++) { // Create all philosopher threads
        sprintf(name, "Philosopher %d", i);
        int index = i;
        ThreadNew(name, Philosopher, 3, &numEating, &fork, index);
    }
    RunAllThreads();
    
    // This is required since we are not using office thread_107.h
    // This thread_107.h is provided by github public code
    for(int i=0; i< NUM_DINERS; i++)
    {
        SemaphoreWait(finish);
    }
    
    
    printf("All done!\n");
    SemaphoreFree(finish);
    SemaphoreFree(numEating);
    for (i = 0; i < NUM_DINERS; i++)
        SemaphoreFree(fork[i]);
}
/**
 * Philosopher
 * -----------
 * This is the routine run in each of the philosopher threads. Each runs in
 * ean at-think loop where pondering for a while builds up a big hunger.
 */
static void* Philosopher(void* args)
{
    
    Semaphore numEating = *((Semaphore **) args)[1];
    Semaphore* fork = ((Semaphore **) args)[2];
    int index = ((int **) args)[3];
    
    Semaphore leftFork = fork[LEFT(index)];
    Semaphore rightFork = fork[RIGHT(index)];
    
    for (int i = 0; i < EAT_TIMES; i++) {
        Think();
        Eat(numEating, leftFork, rightFork);
    }
    
    SemaphoreSignal(finish);
}
static void Think(void)
{
    printf("%s thinking!\n", ThreadName());
    //RandomDelay(10000,50000); // "think" for random time
}
/**
 * We first wait on the availability of an opportunity to eat, and
 * only then do we attempt to grab our left & right forks and chow
 * some spaghetti. Notice that we let go of the locks in the reverse
 * order that we acquire them, so that we don't signal another
 * philosopher to eat until after we have put down both our forks.
 */
static void Eat(Semaphore numEating, Semaphore leftFork, Semaphore rightFork)
{
    SemaphoreWait(numEating); // wait until can try to get forks
    SemaphoreWait(leftFork); // get left
    SemaphoreWait(rightFork); // get right
    
    
    printf("%s eating!\n", ThreadName());
    //RandomDelay(10000,50000); // "eat" for random time
    SemaphoreSignal(leftFork); // let go
    SemaphoreSignal(rightFork);
    SemaphoreSignal(numEating);
}
