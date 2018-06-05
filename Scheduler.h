#ifndef EX2_SCHEDULER_H
#define EX2_SCHEDULER_H

// Includes
#include "Thread.h"
#include "ErrorHandler.h"

// Data structures.
#include <queue>
#include <map>

// Macros
#define MAIN_THREAD_ID 0
#define EMPTY_CELL -1
#define NO_ACTIVE_THREAD -1
#define JUMP_RETURN_VALUE 1
#define SECOND 1000000

// The type of data structure to hold the IDs
typedef vector<int> vec;

// Iterator of vec
typedef vec::iterator vecIter;

// All possible scenarios that may occur during a round-robin cycle
enum scenario {ROUTINE, TOBLOCK, TOSLEEP, TOSELFREMOVE};


/**
 * This class is responsible for the threads management. This is done using the
 * Round-Robin algorithm as that forces the thread out of the CPU once a quantum
 * expires. The Scheduler holds all of the threads, and assigns them to different
 * data structures according to their state (which is changed by the algorithm
 * or by the user).
 */
class Scheduler {
private:

    /**
     * Maximum number of threads.
     */
    int _maxThreads;

    /**
     * Maximum number of threads.
     */
    int _stackSize;

    /**
     * An array the manages the thread IDs.
     */
    pid_t *_idManagar;

    /**
     * The current scenario
     */
    scenario _currentScenario;
    /**
     * A map that holds all of the available Threads.
     * Pairs of (ID, pointer to Thread)
     */
    map<int, Thread *> _threads;

    /**
     * A vector that holds all of the keys of the map to the ready Threads.
     */
    vec _readyThreads;

    /**
     * A vector that holds all of the keys of the map to the sleeping Threads.
     */
    vec _sleepThreads;

    /**
     * A vector that holds all of the keys of the map to the blocked Threads.
     */
    vec _blockThreads;

    /**
     * The key of the map to the running Thread.
     */
    int _runningThread;

    /**
    * The quantum counter for the whole process
    */
    int _totalQuantumCounter;

    /*
     * A pointer to a thread that will be deleted next round.
     */
    Thread* _toDelete;
//-------------

    /**
     * ID is chosen as follows: The main goal is to chose the
     * smallest non-negative integer not already taken by an existing thread.
     * Therefore,we shall iterate over the vector from the beginning
     * (In ascending order) first cell with NULL is the one we need.
     * When terminating a thread with ID i, the ith cell will be initialized
     * with EMPTY_CELL.
     * @return the new ID.
     */
    pid_t _getNewID();

    /**
     * A function that deletes an ID from idManagar.
     * @param ID the ID to delete
     * @return SUCCESS on success and FAILURE on failure
     */
    void _deleteID(pid_t ID);

    /**
     * Calls to the suitable error message according to the bad ID that is given.
     * @param ID the id
     * @return SUCCESS on success and FAILURE on failure
     */
    int _badIDChecker(int ID);

    /**
     * Gets the vector in which the Thread with ID is stored.
     * Function assumes that the ID is not in running state.
     * @param ID the Thread to remove.
     * @return a pointer to the vector. nullptr is returned if Thread doesn't
     * exists.
     */
    vec *_getVectorOfThread(int ID);

    /**
     * Gets the iterator (location) of the Thread with ID.
     * Function assumes that the ID is not in running state.
     * @param ID The Thread ID
     * @param vecOfThread The vector to search for the Thread in
     * @return an iterator the the Thread. nullptr is returned if Thread doesn't
     * exists.
     */
    vecIter _getIterOfThread(int ID, vec *vecOfThread);

    /**
     * Thread switching function between environments. if saveTo is
     * NO_ACTIVE_THREAD, no sigsetjmp will happen.
     * @param saveTo The ID of the Thread to save to
     * @param jumpTo The ID of the Thread to jump to
     * @return None
     */
    void _switchThreads(int saveTo, int jumpTo);

    /**
     * Remove a running thread. Update _runningThread, delete ID and free the
     * resources of the removed thread.
     * @param ID of the thread
     * @return SUCCESS or FAILURE accordingly.
     */
    void _removeThreadHelper(int ID, vec *stateVec);

    /**
     * Kills the main process from inside the scheduler.
     * This code will run only ONCE per run (any additional call will not
     * take any affect).
     * @return None
     */
    void _killProcess();

    /**
     * Function that is designed to facilitate the readability of the code.
	 * @param pushTO the vector to add the thread to.
	 * @param deletFrom the vector to remove the thread from.
	 * @param ID the id.
	 * @param iter the iterator of the thread with this id.
	 * @return SUCCESS on success and FAILURE on failure.
     */
    int _accessDBtoInsertAndDelete(vec *pushTO, vec *deletFrom, int ID, \
                                  vecIter iter);

    /**
     * Manages all of the sleeping threads. Decreases the time that has left
     * to sleep, and wakes up every thread that finished its sleeping time.
     * @return None
     */
    void _manageSleepingThreads(void);

public:
    /**
     * C-tor
     * @param maxThread The maximum Threads to create.
     * @param stackSize the stack size of the Thread.
     */
    Scheduler(int maxThread, int stackSize);

    /**
     * D-tor
     */
    ~Scheduler();

    /**
     * Adding a new Thread.
     * @param f The function of the Thread
     * @return The Thread ID on success and FAILURE on failure
     */
    int addThread(void (*f)(void));

    /**
     * Removing a Thread.
     * @param ID The ID of the Thread to remove.
     * @return SUCCESS on success and FAILURE on failure
     */
    int removeThread(int ID);

    /**
     * Blocking a Thread.
     * @param ID The ID of the Thread to block.
     * @return SUCCESS on success and FAILURE on failure
     */
    int blockThread(int ID);

    /**
     * Resuming a Thread.
     * @param ID The ID of the Thread to resume.
     * @return SUCCESS on success and FAILURE on failure
     */
    int resumeThread(int ID);

    /**
     * Make the runnning Thread sleep.
     * @param num_quantums The ID of the Thread that should sleep.
     * @return SUCCESS on success and FAILURE on failure
     */
    int sleepThread(int num_quantums);

    /**
     * Manages the Threads. This is where the Round-Robin decisions are being
	 * made. Every scenario will be dealt in this code, and all states and
	 * DASTs will be updated as needed.
     * @return None
     */
    void manageThreads(void);

    /**
     * Getter for the running thread ID.
     * @param dummy a dummy param that is passed in order to match the caller
	 * signature. Its value is ignored.
     * @return the ID of the running Thread.
     */
    int getRunningThreadID(int dummy);

    /**
     * This function returns the number of quantums until the thread
     * with id tid wakes up.
     * @param ID the ID of the thread.
     * @return the number of quantums until the thread wakes up.
     */
    int getTimeToWakeUp(int ID);

    /**
     * This function returns the number of quantums the threadwith ID tid was
     * in running state.
     * @param ID the ID of the thread
     * @return the number of quantums the tread was in running state.
     */
    int getNumOfQuantums(int ID);

    /**
     * Getter for the current scenario.
     * @return the current scenario.
     */
    int getScenario();

    /**
     * Getter for the total quantum counter.
     * @param dummy a dummy param that is passed in order to match the caller
	 * signature. Its value is ignored.
     * @return the total quantum counter.
     */
    int getTotalQuantumCounter(int dummy);
};


#endif //EX2_SCHEDULER_H