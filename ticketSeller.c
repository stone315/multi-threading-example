/*
 * ticketSeller.c
 * ---------------
 * A very simple example of a critical section that is protected by a
 * semaphore lock. There is a global variable numTickets which tracks the
 * number of tickets remaining to sell. We will create many threads that all
 * will attempt to sell tickets until they are all gone. However, we must
 * control access to this global variable lest we sell more tickets than
 * really exist. We have a semaphore lock that will only allow one seller
 * thread to access the numTickets variable at a time. Before attempting to
 * sell a ticket, the thread must acquire the lock by waiting on the semaphore
 * and then release the lock when through by signalling the semaphore.
 */
#include "thread_107.h"
#include <stdio.h>

#define NUM_TICKETS 40
#define NUM_SELLERS 3
/**
 * The ticket counter and its associated lock will be accessed
 * all threads, so made global for easy access.
 */
static int numTickets = NUM_TICKETS;
static Semaphore ticketsLock;
static Semaphore finish;


/**
 * SellTickets
 * -----------
 * This is the routine forked by each of the ticket-selling threads.
 * It will loop selling tickets until there are no more tickets left
 * to sell. Before accessing the global numTickets variable,
 * it acquires the ticketsLock to ensure that our threads don't step
 * on one another and oversell on the number of tickets.
 */
static void* SellTickets(void* l)
{
  bool done = false;
  int numSoldByThisThread = 0; // local vars are unique to each thread
  while (!done) {
  /**
  * imagine some code here which does something independent of
  * the other threads such as working with a customer to determine
  * which tickets they want. Simulate with a small random delay
  * to get random variations in output patterns.
  */
    SemaphoreWait(ticketsLock); // ENTER CRITICAL SECTION
    if (numTickets == 0) { // here is safe to access numTickets
      done = true; // a "break" here instead of done variable
  // would be an error- why?
    } else {

      numTickets--;
      numSoldByThisThread++;
      printf("%s sold one (%d left)\n", ThreadName(), numTickets);
    }
    SemaphoreSignal(ticketsLock); // LEAVE CRITICAL SECTION
    
  }
  
  printf("%s noticed all tickets sold! (I sold %d myself) \n", ThreadName(), numSoldByThisThread);

    SemaphoreSignal(finish);
}


/**
 * Our main is creates the initial semaphore lock in an unlocked state
 * (one thread can immediately acquire it) and sets up all of
 * the ticket seller threads, and lets them run to completion. They
 * should all finish when all tickets have been sold. By running with the
 * -v flag, it will include the trace output from the thread library.
 */
int main(int argc, char **argv)
{
  int i;
  char name[32];
  bool verbose = (argc == 2 && (strcmp(argv[1], "-v") == 0));



  InitThreadPackage(verbose);
  ticketsLock = SemaphoreNew("Tickets Lock", 1);
    finish = SemaphoreNew("done", 0);
  ListAllSemaphores();

  for (i = 0; i < NUM_SELLERS; i++) {
    sprintf(name, "Seller #%d", i); // give each thread a distinct name
    ThreadNew(name, SellTickets, 1, NULL);
  }
  ListAllThreads();
    
    
  RunAllThreads(); // Let all threads loose
    
    // This thread is required becuase the library thread_107.h is not official.
    // This library is provided by github open code
    for(int i=0; i< NUM_SELLERS;i++){
        SemaphoreWait(finish);
    }
    
    
    
  SemaphoreFree(ticketsLock); // to be tidy, clean up
    SemaphoreFree(finish);
  printf("All done!\n");
  return 0;
}
