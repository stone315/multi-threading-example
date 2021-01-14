/**
 * readWrite.c
 * --------------
 * The canonical consumer-producer example. This version has just one reader * and just one writer (although it could be generalized to multiple readers/ * writers) communicating information through a shared buffer. There are two * generalized semaphores used, one to track the num of empty buffers, another * to track full buffers. Each is used to count, as well as control access.
 */
#include "thread_107.h"
#include <stdio.h>
#define NUM_TOTAL_BUFFERS 5
#define DATA_LENGTH 20

Semaphore finish;


/**
 * ProcessData
 * -----------
 * This just stands in for some lengthy processing step that might be
 * required to handle the incoming data. Processing the data can be done by
 * many reader threads simultaneously since it doesn't access any global state */
//static void ProcessData(char data)
//{
    //ThreadSleep(RandomInteger(0, 500));
    // sleep random amount
//}
/**
 * PrepareData
 * -----------
 * This just stands in for some lengthy processing step that might be
 * required to create the data. Preparing the data can be done by many writer * threads simultaneously since it doesn't access any global state. The data * value is just randomly generated in our simulation.
 */
static char PrepareData(int i)
{
    //ThreadSleep(RandomInteger(0, 500));
    int chr = 'A' + i;
    return (char) chr;
    // sleep random amount
    // return random character
}



/**
 * Writer
 * ------
 * This is the routine forked by the Writer thread. It will loop until * all data is written. It prepares the data to be written, then waits * for an empty buffer to be available to write the data to, after which * it signals that a full buffer is ready.
 */
static void* Writer(void* args)
{
    int i, writePt = 0;
    char data;
    
    
    Semaphore emptyBuffers = *((Semaphore **)args)[1];
    Semaphore fullBuffers = *((Semaphore **)args)[2];
    char *buffers = ((char **) args)[3];
    
    for (i = 0; i < DATA_LENGTH; i++) {
        data = PrepareData(i);
        SemaphoreWait(emptyBuffers);
        *(buffers + writePt) = data;
        printf("%s: buffer[%d] = %c\n", ThreadName(), writePt, data); writePt = (writePt + 1) % NUM_TOTAL_BUFFERS;
        SemaphoreSignal(fullBuffers);
    }
    
    SemaphoreSignal(finish);
}

// announce full buffer ready
/**
 * Reader
 * ------
 * This is the routine forked by the Reader thread. It will loop until * all data is read. It waits until a full buffer is available and the * reads from it, signals that now an empty buffer is ready, and then * goes off and processes the data.
 */
static void *Reader(void* args)
{
    int i, readPt = 0;
    char data;
    
    
    Semaphore emptyBuffers = *((Semaphore **)args)[1];
    Semaphore fullBuffers = *((Semaphore **)args)[2];
    char *buffers  = ((char **) args)[3];
    
    for (i = 0; i < DATA_LENGTH; i++) {
        SemaphoreWait(fullBuffers); // wait til something to read
        data = *(buffers + readPt); // pull value out of buffer
        printf("\t\t%s: buffer[%d] = %c\n", ThreadName(), readPt, data);
        readPt = (readPt + 1) % NUM_TOTAL_BUFFERS;
        SemaphoreSignal(emptyBuffers); // announce empty buffer
        //ProcessData(data); // now go off & process data
    }
    
    SemaphoreSignal(finish);
    
}
// go off & get data ready
// now wait til an empty buffer avail
// put data into buffer


/**
 * Initially, all buffers are empty, so our empty buffer semaphore starts
 * with a count equal to the total number of buffers, while our full buffer * semaphore begins at zero. We create two threads: one to read and one
 * to write, and then start them off running. They will finish after all
 * data has been written & read. By running with the -v flag, it will include * the trace output from the thread library.
 */


void main(int argc, char **argv)
{
    bool verbose = (argc == 2 && (strcmp(argv[1], "-v") == 0));
    Semaphore emptyBuffers, fullBuffers; // semaphores used as counters
    char *buffers = malloc( NUM_TOTAL_BUFFERS * sizeof(char)); // the shared buffer
    InitThreadPackage(verbose);
    
    
    finish = SemaphoreNew("finish", 0);
    emptyBuffers = SemaphoreNew("Empty Buffers", NUM_TOTAL_BUFFERS);
    fullBuffers = SemaphoreNew("Full Buffers", 0);
    
    
    ListAllSemaphores();
    
    for(int i=0; i<3; i++){
        char str[12];
        sprintf(str,"Writer %d", i);
        ThreadNew(str, Writer, 3, &emptyBuffers, &fullBuffers, &buffers);
    }
    
    
    for(int i=0; i<3; i++){
        char str[12];
        sprintf(str,"Reader %d", i);
        ThreadNew(str, Reader, 3, &emptyBuffers, &fullBuffers, &buffers);
    }
    
    
    
    
    RunAllThreads();
    // This is required since thread_107.h is not a offical library
    for(int i=0; i< 6; i++){
        SemaphoreWait(finish);
    }
    
    SemaphoreFree(emptyBuffers);
    SemaphoreFree(fullBuffers);
    SemaphoreFree(finish);
    printf("All done!\n");
    
}
