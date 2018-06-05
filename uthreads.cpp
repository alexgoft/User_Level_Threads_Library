#include "uthreads.h"
#include "Scheduler.h"

#include <sys/time.h>
#include <bits/sigset.h>

// sigaction and timers
struct sigaction sa;
struct itimerval timer;
sigset_t maskSet, pendingSet;

// the Scheduler objects that manages the Threads
//static Scheduler sch(MAX_THREAD_NUM, STACK_SIZE);
static Scheduler* sch = new Scheduler(MAX_THREAD_NUM, STACK_SIZE);
// Global library counters
static int lib_quantum_usecs = 0;

// Typedef for pointers to member functions of Scheduler
typedef int (Scheduler::*SchedulerMemberFunction)(int num);

// Wish to invoke a void(*f(void))-using user function.
#define SPAWN 1
// Wish to invoke a int(*f(void))-or-int(*f(int))-using user function.
#define NOT_SPAWN 0
// Wish to invoke a int(*f(void))-using user function. (such as uthread_get_tid)
#define NO_PARAM 666
// Macro used as indicator for syscalls failures.
#define SIG_FAILED -1
// Given signal does exist in a set.
#define SIG_IN_SET 1
// Bad number of usecs
#define BAD_USEC 0
//--------------------------------------------------------------------------//

/*
 * Deleting the scheduler triggers the destructor in which all memory allocated
 * for the threads spawned (and for ID array) is released. Used specially
 * in casses we fail at system calls.
 * @param none
 * @return none
 */
static void killProcessAfterMemoryAllocs(void)
{
    delete &sch;
    ErrorHandler::sysCallError(THREAD_SYS_CALL_ERROR_TIMER_FAILED);
}

//---------------------SIGNAL AND TIMER MANAGEMENT----------------------------//


/**
* Resets the main timer with lib_quantum_usecs.
* @param None.
* @return None.
*/
static void reset_timer(void) {
    // Configure the timer to expire after quantum_usecs ms
    timer.it_value.tv_sec = lib_quantum_usecs / SECOND;
    timer.it_value.tv_usec = lib_quantum_usecs % SECOND;

    // configure the timer to expire every quantum_usecs ms after tha
    timer.it_interval.tv_sec = lib_quantum_usecs / SECOND;
    timer.it_interval.tv_usec = lib_quantum_usecs % SECOND;

    // Start a virtual timer. It counts down whenever this process is executing
    if (setitimer(ITIMER_VIRTUAL, &timer, NULL))
    {
        killProcessAfterMemoryAllocs();
    }
}

/**
* The timer handler function. Intermediate between the Scheduler and timer.
* By this function, when needed
* @return None.
*/
static void timer_handler(int sig)
{
    reset_timer();
    sch->manageThreads();
}

//----------------//

/**
* Blocks the SIG_SETMASK in maskSet.
* @return None.
*/
static void block_signal(void)
{
    if(sigprocmask(SIG_BLOCK, &maskSet, NULL) == SIG_FAILED)
    {
        killProcessAfterMemoryAllocs();
    }
}

/**
* Unblocks the SIG_SETMASK in maskSet.
* @return None.
*/
static void unblock_signal(void)
{
    if (sigprocmask(SIG_UNBLOCK, &maskSet, NULL) == SIG_FAILED)
    {
        killProcessAfterMemoryAllocs();
    }
}

//----------------//


/**
* Invokes func on scheduler with the parameter value and returns retured data.
* Note: only functions with the following sig will work: int func(int value)
* @param scheduler the Scheduler object to invoke the function on
* @param func the member function to call
* @param value the parameter to invoke func with
* return the retured value from the invoked function
*/
static int invoke_member_function(Scheduler *scheduler,
                                  SchedulerMemberFunction func,\
                                  FunctionPointer spawnFunction,int isSpawn,\
                                  int value)
{
    int result;
    int sig;
    int retVal;

    // Block signal
    block_signal();
    sigpending(&pendingSet);

    // Call function depending on what type.
    if(isSpawn == SPAWN)
    {
        retVal = scheduler->addThread(spawnFunction);
    }
    else
    {
        retVal = (scheduler->*func)(value);
    }

    if(sch->getScenario() != ROUTINE)
    {
        raise(SIGVTALRM);
    }

    if(sigemptyset(&pendingSet) == SIG_FAILED)
    {
        ErrorHandler::sysCallError(THREAD_SYS_CALL_ERROR_SIGNAL_HANDLE);
    }
    if(sigpending(&pendingSet) == SIG_FAILED)
    {
        ErrorHandler::sysCallError(THREAD_SYS_CALL_ERROR_SIGNAL_HANDLE);
    }
    // Checking if SIGVTALARM was caught when blocked.
    if (sigismember(&pendingSet, SIGVTALRM) == SIG_IN_SET)
    {
        result = sigwait(&pendingSet, &sig);
        sigpending(&pendingSet);
        // Removing SIGVTALARM from pending list was successful.
        if (result == SUCCESS)
        {
            unblock_signal();
            raise(SIGVTALRM);
            return retVal;
        }
    }
    // Unblock signal
    unblock_signal();
    return retVal;
}

//---------------------------------------------------------------------------//


/*
* Description: This function initializes the thread library.
* You may assume that this function is called before any other thread library
* function, and that it is called exactly once. The input to the function is
* the length of a quantum in micro-seconds. It is an error to call this
* function with non-positive quantum_usecs.
* @param quantum_usecs
* @return On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs) {
    if(quantum_usecs <= BAD_USEC)
    {
        ErrorHandler::sysCallError(THREAD_SYS_CALL_ERROR_TIMER);
    }

    // quantum_usecs is local, but we need it for further uses.
    lib_quantum_usecs = quantum_usecs;

    // Install timer_handler as the signal handler for SIGVTALRM.
    sa.sa_handler = &timer_handler;

    // Set the maskset - later be used to block timer signals.
    if(sigemptyset(&maskSet) == SIG_FAILED)
    {
        ErrorHandler::sysCallError(THREAD_SYS_CALL_ERROR_SIGNAL_HANDLE);
    }
    if(sigaddset(&maskSet, SIGVTALRM) == SIG_FAILED)
    {
        ErrorHandler::sysCallError(THREAD_SYS_CALL_ERROR_SIGNAL_HANDLE);
    }

    if (sigaction(SIGVTALRM, &sa, NULL) == SIG_FAILED)
    {
        ErrorHandler::sysCallError(THREAD_SYS_CALL_ERROR_SIGNAL_HANDLE);
    }

    reset_timer();
    return SUCCESS;
}

/*
* Description: This function creates a new thread, whose entry point is the
* function f with the signature void f(void). The thread is added to the end
* of the READY threads list. The uthread_spawn function should fail if it
* would cause the number of concurrent threads to exceed the limit
* (MAX_THREAD_NUM). Each thread should be allocated with a stack of size
* STACK_SIZE bytes.
* Return value: On success, return the ID of the created thread.
* On failure, return -1.
*/
int uthread_spawn(FunctionPointer f)
{
    return invoke_member_function(sch, nullptr , f, SPAWN, NO_PARAM);
}

/*
* Description: This function terminates the thread with ID tid and deletes
* it from all relevant control structures. All the resources allocated by
* the library for this thread should be released. If no thread with ID tid
* exists it is considered as an error. Terminating the main thread
* (tid == 0) will result in the termination of the entire process using
* exit(0) [after releasing the assigned library memory].
* Return value: The function returns 0 if the thread was successfully
* terminated and -1 otherwise. If a thread terminates itself or the main
* thread is terminated, the function does not return.
*/
int uthread_terminate(int tid)
{
    return invoke_member_function(sch, &Scheduler::removeThread, nullptr, \
                                  NOT_SPAWN, tid);
}


/*
* Description: This function blocks the thread with ID tid. The thread may
* be resumed later using uthread_resume. If no thread with ID tid exists it
* is considered as an error. In addition, it is an error to try blocking the
* main thread (tid == 0). If a thread blocks itself, a scheduling decision
* should be made. Blocking a thread in BLOCKED or SLEEPING states has no
* effect and is not considered as an error.
* Return value: On success, return 0. On failure, return -1.
*/
int uthread_block(int tid)
{
    return invoke_member_function(sch, &Scheduler::blockThread,nullptr, \
                                  NOT_SPAWN, tid);
}


/*
* Description: This function resumes a blocked thread with ID tid and moves
* it to the READY state. Resuming a thread in the RUNNING, READY or SLEEPING
* state has no effect and is not considered as an error. If no thread with
* ID tid exists it is considered as an error.
* Return value: On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid)
{
    return invoke_member_function(sch, &Scheduler::resumeThread, nullptr, \
                                  NOT_SPAWN, tid);
}

/*
* Description: This function puts the RUNNING thread to sleep for a period
* of num_quantums (not including the current quantum) after which it is moved
* to the READY state. num_quantums must be a positive number. It is an error
* to try to put the main thread (tid==0) to sleep. Immediately after a thread
* transitions to the SLEEPING state a scheduling decision should be made.
* Return value: On success, return 0. On failure, return -1.
*/
int uthread_sleep(int num_quantums)
{
    if(lib_quantum_usecs <= BAD_USEC)
    {
        return ErrorHandler::libError(THREAD_LIB_ERROR_NEGATIVE_QUANTUM);
    }
    return invoke_member_function(sch, &Scheduler::sleepThread,  nullptr, \
                                  NOT_SPAWN,num_quantums);
}

/*
* Description: This function returns the number of quantums until the thread
* with id tid wakes up including the current quantum. If no thread with ID
* tid exists it is considered as an error. If the thread is not sleeping,
* the function should return 0.
* Return value: Number of quantums (including current quantum) until wakeup.
*/
int uthread_get_time_until_wakeup(int tid)
{
    return invoke_member_function(sch, &Scheduler::getTimeToWakeUp, nullptr, \
                                  NOT_SPAWN, tid);
}

/*
* Description: This function returns the thread ID of the calling thread.
* Return value: The ID of the calling thread.
*/
int uthread_get_tid(void)
{
    return invoke_member_function(sch, &Scheduler::getRunningThreadID, nullptr,\
                                  NOT_SPAWN, NO_PARAM);
}

/*
* Description: This function returns the total number of quantums that were
* started since the library was initialized, including the current quantum.
* Right after the call to uthread_init, the value should be 1.
* Each time a new quantum starts, regardless of the reason, this number
* should be increased by 1.
* Return value: The total number of quantums.
*/
int uthread_get_total_quantums()
{
    return invoke_member_function(sch, &Scheduler::getTotalQuantumCounter,\
                                  nullptr, NOT_SPAWN, NO_PARAM);
}


/*
* Description: This function returns the number of quantums the thread with
* ID tid was in RUNNING state. On the first time a thread runs, the function
* should return 1. Every additional quantum that the thread starts should
* increase this value by 1 (so if the thread with ID tid is in RUNNING state
* when this function is called, include also the current quantum). If no
* thread with ID tid exists it is considered as an error.
* Return value: On success, return the number of quantums of the thread with ID
* tid. On failure, return -1.
*/
int uthread_get_quantums(int tid)
{
    return invoke_member_function(sch, &Scheduler::getNumOfQuantums,\
                                  nullptr, NOT_SPAWN, tid);
}