/**
 * store.c
 * --------
 * An example loosely based on some old final exam questions merged together
 * to show a combination of techniques (binary locks, generalized counters,
 * rendezvous semaphores) in a more complicated arrangement.
 *
 * This is the "ice cream store" simulation. There are customers who want
 * to buy ice cream cones, clerks who make the cones, the manager who
 * checks the work of the clerks, and the cashier who takes the customer's
 * money. There are a variety of interactions that need to be modeled:
 * the customers dispatching several clerks one for each cone they are buying,
 * the clerks who need the manager to approve their work, the cashier
 * who tries to the customer in an orderly line, and so on that require
 * use of semaphores to coordinate the activities.
 */
#include <stdio.h>
#include "thread_107.h"
#define NUM_CUSTOMERS 10
#define SECOND 1000000
static void Cashier(void);
static void Clerk(void* args);
static void Manager(void* args);
static void Customer(void* args);
static void SetupSemaphores(void);
static void FreeSemaphores(void);
static int RandomInteger(int low, int high);
static void MakeCone(void);
static bool InspectCone(void);
static void Checkout(int linePosition);
static void Browse(void);
Semaphore FinishedThread;
/* We have many variables accessed by multiple threads in this program and
 * so we have chosen to make them global. In order to keep things tidy, we
 * arrange the globals into coherent structures so that it is easy to
 * understand the relationship between them. This is much preferable to
 * having 12 independent variables declared with no clear indication of
 * their use and grouping.
 */
struct inspection { // struct of globals for Clerk->Manager rendezvous
    Semaphore available; // lock used to serialize access to the one Manager
    Semaphore requested; // signaled by clerk when cone is ready to inspect
    Semaphore finished; // signaled by manager after cone has been inspected
    bool passed; // status of the last inspection
} inspection;
struct line { // struct of globals for Customer->Cashier line
    Semaphore lock; // lock used to serialize access to counter
    int nextPlaceInLine; // counter
    Semaphore customers[NUM_CUSTOMERS]; // rendezvous for customer by position
    Semaphore customerReady;// signaled by customer when ready to check out
} line;

/*
 * The main just sets up all the semaphores and creates all the starting
 * threads. We have one thread per customer, each that is set to buy a
 * random number of cones. We keep track of the total cones needed so
 * we can pass that information to the manager.
 */
int main(int argc, char **argv)
{
    int i, numCones = 4, totalCones = 0;
    bool verbose = (argc == 2 && (strcmp(argv[1], "-v") == 0));
    InitThreadPackage(verbose);
    
    SetupSemaphores();
    FinishedThread = SemaphoreNew("Finished", 0);
    
    for (i = 0; i < NUM_CUSTOMERS; i++) {
        char name[32];
        sprintf(name, "Customer %d", i);
        ThreadNew(name, Customer, 1, &numCones);
        totalCones += numCones;
    }
    
    ThreadNew("Cashier", Cashier, 0);
    ThreadNew("Manager", Manager, 1, &totalCones);
    RunAllThreads();
    
    
    // This is required from the unoffical thread_107.c
    for(int i=0; i< NUM_CUSTOMERS; i++){
        SemaphoreWait(FinishedThread);
    }
    
    
    printf("All done!\n");
    FreeSemaphores();
    SemaphoreFree(FinishedThread);
    return 0;
}
/**
 * The manager has a pretty easy job. He just waits around until a clerk
 * has made an ice cream cone and wants the manager to check it over. The
 * manager efficiently waits for the signal from the clerk and then once
 * awakened, inspects the cone and determines its status, writing to a
 * global variable, and then signals back to the clerk that he has passed
 * judgment. Note that variables like numPerfect and numInspections can and
 * should be local to the Manager since no other thread needs access to them.
 */
static void Manager(void* args)
{
    int totalNeeded = *((Semaphore**) args)[1];
    int numPerfect = 0, numInspections = 0;
    while (numPerfect < totalNeeded) {
        SemaphoreWait(inspection.requested);
        inspection.passed = InspectCone(); // safe since clerk acquired lock
        numInspections++;
        if (inspection.passed)
            numPerfect++;
        SemaphoreSignal(inspection.finished);
    }
    printf("Inspection success rate %d%%\n", (100*numPerfect)/numInspections);
}

/*
 * A Clerk thread is dispatched by the customer for each cone they want.
 * The clerk makes the cone and then has to have the manager inspect it.
 * If it doesn't pass, they have to make another. To check with the manager,
 * the clerk has to acquire the exclusive rights to confer with the manager,
 * then signal to wake up the manager, and then wait until they have passed
 * judgment (by writing to a global). We need to be sure that we don't
 * release the inspection lock until we have read the status and are
 * totally done with our inspection. Once we have a perfect ice cream,
 * we signal back to the originating customer by means of the rendezvous
 * semaphore passed as a parameter to this thread.
 */
static void Clerk(void* args)
{
    Semaphore done = ((Semaphore **) args)[1];
    
    
    bool passed = false;
    while (!passed) {
        MakeCone();
        SemaphoreWait(inspection.available);
        SemaphoreSignal(inspection.requested);
        SemaphoreWait(inspection.finished);
        passed = inspection.passed;
        SemaphoreSignal(inspection.available);
    }
    
    SemaphoreSignal(done);
}
/*
 * The customer dispatches one thread for each cone desired,
 * then browses around while the clerks make the cones. We create
 * our own local generalized rendezvous semaphore that we pass to
 * the clerks so they can notify us as they finish. We use
 * that semaphore to "count" the number of clerks who have finished.
 * In the second loop, we wait once for each clerk, which allows
 * us to efficiently block until all clerks check back in.
 * Then we get in line for the cashier by "taking the next number"
 * that is available and signaling our presence to the cashier.
 * When they call our number (signal the semaphore at our place
 * in line), we're done.
 */
static void Customer(void* args)
{
    
    int numConesWanted = *((Semaphore **) args)[1];
    int i, myPlace;
    Semaphore clerksDone = SemaphoreNew("Count of clerks done", 0);
    
    for (i = 0; i < numConesWanted; i++)
        ThreadNew("Clerk", Clerk, 1, clerksDone);
    
    
    Browse();
    for (i = 0; i < numConesWanted; i++)
        SemaphoreWait(clerksDone);
        
    
    SemaphoreFree(clerksDone); // this semaphore is not needed anymore
    SemaphoreWait(line.lock); // binary lock to protect global
    myPlace = line.nextPlaceInLine++; // get number & update line count
    SemaphoreSignal(line.lock);
    
    SemaphoreSignal(line.customerReady); // signal to cashier we are in line
    SemaphoreWait(line.customers[myPlace]); // wait til checked through
    
    printf("%s done!\n", ThreadName());
    SemaphoreSignal(FinishedThread);
}
/*
 * The cashier just checks the customers through, one at a time,
 * as they become ready. In order to ensure that the customers get
 * processed in order, we maintain an array of semaphores that allow us
 * to selectively notify a waiting customer, rather than signaling
 * one combined semaphore without any control over which waiter will
 * get notified.
 */
static void Cashier(void)
{
    int i;
    for (i = 0; i < NUM_CUSTOMERS; i++) {
        SemaphoreWait(line.customerReady);
        Checkout(i);
        SemaphoreSignal(line.customers[i]);
    }

}
/*
 * Note carefully the initial values for all the various semaphores.
 * What is the consequence of accidentally starting a lock semaphore
 * out as 0 or 2 instead of 1? What about setting a counting or
 * rendezvous semaphore incorrectly?
 */
static void SetupSemaphores(void)
{
    int i;
    inspection.requested = SemaphoreNew("Inspection Requested", 0);
    inspection.finished = SemaphoreNew("Inspection Finished", 0);
    inspection.available = SemaphoreNew("Manager Available", 1);
    inspection.passed = false;
    line.customerReady = SemaphoreNew("Customer ready", 0);
    line.lock = SemaphoreNew("Line lock", 1);
    line.nextPlaceInLine = 0;
    for (i = 0; i < NUM_CUSTOMERS; i++)
        line.customers[i] = SemaphoreNew("Customer in line", 0);
}

static void FreeSemaphores(void)
{
    int i;
    SemaphoreFree(inspection.requested);
    SemaphoreFree(inspection.finished);
    SemaphoreFree(inspection.available);
    SemaphoreFree(line.customerReady);
    SemaphoreFree(line.lock);
    for (i = 0; i < NUM_CUSTOMERS; i++)
        SemaphoreFree(line.customers[i]);
}
/* These are just fake functions to stand in for processing steps */
static void MakeCone(void)
{
    //ThreadSleep(RandomInteger(0, 3*SECOND)); // sleep random amount
    printf("\t%s making an ice cream cone.\n", ThreadName());
}
static bool InspectCone(void)
{
    bool passed = (RandomInteger(1, 2) == 1);
    printf("\t\t%s examining cone, did it pass? %c\n", ThreadName(), (passed ? 'Y':'N'));
    //ThreadSleep(RandomInteger(0, .5*SECOND)); // sleep random amount
    return passed;
}
static void Checkout(int linePosition)
{
    printf("\t\t\t%s checking out customer in line at position #%d.\n", ThreadName(), linePosition);
    //ThreadSleep(RandomInteger(0, SECOND)); // sleep random amount
}
static void Browse(void)
{
    //ThreadSleep(RandomInteger(0, 5*SECOND)); // sleep random amount
    printf("%s browsing.\n", ThreadName());
}
/*
 * RandomInteger
 * -------------
 * Simple random integer function.
 */
static int RandomInteger(int low, int high)
{
    extern long random();
    long choice;
    int range = high - low + 1;
    PROTECT(choice = random()); // protect non-re-entrant random
    return low + choice % range;
}
