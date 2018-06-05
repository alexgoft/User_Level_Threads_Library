#include <iostream>
#include "Scheduler.h"

//------------------------CONSTRUCTORS DESTRUCTORS----------------------------//
/**
 * C-tor
 * @param maxThread The maximum Threads to create.
 * @param stackSize the stack size of the Thread.
 */
Scheduler::Scheduler(int maxThreads, int stackSize)
        : _maxThreads(maxThreads),
          _stackSize(stackSize),
          _idManagar(new(nothrow) int[_maxThreads]{EMPTY_CELL}),
          _currentScenario(ROUTINE),
          _totalQuantumCounter(1),
          _toDelete(nullptr)
{
    if (_idManagar == nullptr) {
        ErrorHandler::sysCallError(THREAD_SYS_CALL_ERROR_BAD_ALLOC);
    }

    // Initialize ID manager with the flag "EMPTY_CELL".
    for (int j = 0; j < maxThreads; ++j) {
        _idManagar[j] = EMPTY_CELL;
    }

    // Adding the main Thread (pid 0);
    _runningThread = addThread(nullptr);
    _threads[MAIN_THREAD_ID]->incrementQuantum();
}

/**
 * D-tor
 */
Scheduler::~Scheduler() {
    _killProcess();
};

//---------------------------ID RELATED FUNCTIONS----------------------------//

/**
 * ID is chosen as follows: The main goal is to chose the
 * smallest non-negative integer not already taken by an existing thread.
 * Therefore,we shall iterate over the vector from the beginning
 * (In ascending order) first cell with NULL is the one we need.
 * When terminating a thread with ID i, the ith cell will be initialized
 * with EMPTY_CELL.
 * @return the new ID.
 */
int Scheduler::_getNewID() {
    int last_occupied = 0;
    for (int i = 0; i < _maxThreads; ++i) {
        // The cell is empty.
        if (_idManagar[i] == EMPTY_CELL) {
            _idManagar[i] = i;
            last_occupied = i;
            break;
        }
    }

    return last_occupied;
}

/**
 * A function that deletes an ID from idManagar.
 * @param ID the ID to delete
 * @return SUCCESS on success and FAILURE on failure
 */
void Scheduler::_deleteID(int ID) {
    _idManagar[ID] = EMPTY_CELL;
}

/**
 * Calls to the suitable error message according to the bad ID that is given.
 * @param ID the id
 * @return SUCCESS on success and FAILURE on failure
 */
int Scheduler::_badIDChecker(int ID) {
    if (_idManagar[ID] == EMPTY_CELL) {
        return ErrorHandler::libError(THREAD_LIB_ERROR_NO_SUCH_ID);
    }
    return ErrorHandler::libError(THREAD_LIB_ERROR_ID_OUT_RANGE);

}
//----------------------------------UTILITIES--------------------------------//

/**
 * Function that is designed to facilitate the readability of the code.
 * @param pushTO the vector to add the thread to.
 * @param deletFrom the vector to remove the thread from.
 * @param ID the id.
 * @param iter the iterator of the thread with this id.
 * @return SUCCESS on success and FAILURE on failure.
 */
int Scheduler::_accessDBtoInsertAndDelete(vec *pushTO, vec *deletFrom, int ID,
                                          vecIter iter) {
    // Try to add and delete
    try {
        pushTO->push_back(ID);
        deletFrom->erase(iter);
    }
    catch (std::bad_alloc &ba) {
        _killProcess();
        ErrorHandler::sysCallError(THREAD_SYS_CALL_ERROR_BAD_ALLOC);
    }
    return SUCCESS;
}

/**
 * Gets the vector in which the Thread with ID is stored.
 * Function assumes that the ID is not in running state.
 * @param ID the Thread to remove.
 * @return a pointer to the vector. nullptr is returned if Thread doesn't
 * exists.
 */
vec *Scheduler::_getVectorOfThread(int ID) {
    state threadState;

    // If a thread does'nt exist or its running -> does not belong to any vev.
    if (_idManagar[ID] == EMPTY_CELL || (ID < MAIN_THREAD_ID || \
        ID > _maxThreads) || ID == MAIN_THREAD_ID) {
        return nullptr;
    }
    // Get the Thread state
    threadState = _threads[ID]->getState();

    // Return the relevant vector
    switch (threadState) {
        case READY:

            return &_readyThreads;

        case BLOCKED:
            return &_blockThreads;

        case SLEEPING:
            return &_sleepThreads;

        default:
            return nullptr;
    }
}

/**
 * Gets the iterator (location) of the Thread with ID.
 * Function assumes that the ID is not in running state.
 * @param ID The Thread ID
 * @param vecOfThread The vector to search for the Thread in
 * @return an iterator the the Thread. nullptr is returned if Thread doesn't
 * exists.
 */
vecIter Scheduler::_getIterOfThread(int ID, vec *vecOfThread) {
    vecIter retIter;
    // Check if the ID actually exists in the vector
    for (auto it = vecOfThread->begin(); it < vecOfThread->end(); ++it) {
        // If ID exists, move it
        if (*it == ID) {
            retIter = it;
        }
    }
    return retIter;
}

/**
 * Getter for the total quantum counter.
 * @param dummy a dummy param that is passed in order to match the caller
 * signature. Its value is ignored.
 * @return the total quantum counter.
 */
int Scheduler::getTotalQuantumCounter(int dummy) {
    // Reset dummy
    dummy = 0;
    return _totalQuantumCounter + dummy;
}
//-----------------------------RUNNING THREAD MANAGEMENT---------------------//

/**
 * Thread switching function between environments. if saveTo is
 * NO_ACTIVE_THREAD, no sigsetjmp will happen.
 * @param saveTo The ID of the Thread to save to
 * @param jumpTo The ID of the Thread to jump to
 * @return None
 */
void Scheduler::_switchThreads(int saveTo, int jumpTo) {
    int ret_val;

    // increase thread's quantum and total quantums
    _threads[jumpTo]->incrementQuantum();
    _totalQuantumCounter++;

    // if saveTo is illegal, don't set sig
    if (saveTo == NO_ACTIVE_THREAD) {
        siglongjmp(*_threads[jumpTo]->environment(), JUMP_RETURN_VALUE);
    }
    else {
        ret_val = sigsetjmp(*_threads[saveTo]->environment(), THREAD_SAVE_MASK);
        if (ret_val == JUMP_RETURN_VALUE) {
            // If pointer is not null, delete it and reset it to null
            if (_toDelete != nullptr) {
                delete _toDelete;
                _toDelete = nullptr;
            }
            return;
        }
        siglongjmp(*_threads[jumpTo]->environment(), JUMP_RETURN_VALUE);
    }
}

/**
 * Kills the main process from inside the scheduler.
 * This code will run only ONCE per run (any additional call will not
 * take any affect).
 * @return None
 */
void Scheduler::_killProcess() {
    // numOfKills is initialized only ONCE!
    static int numOfKills = 0;

    // kill only if this code hasn't ran before
    if (numOfKills == 0) {
        // Releasing all resources used for all of the threads.
        for (auto it = _threads.begin(); it != _threads.end(); it++) {
            delete it->second;
        }

        // If pointer is not null, delete it and reset it to null
        if (_toDelete != nullptr) {
            delete _toDelete;
            _toDelete = nullptr;
        }

        // Memory allocated for idManager is released.
        delete[] _idManagar;
        numOfKills++;
    }
}
//---------------------------------------------------------------------------//

/**
 * Adding a new Thread.
 * @param f The function of the Thread
 * @return The Thread ID on success and FAILURE on failure
 */
int Scheduler::addThread(FunctionPointer f)
{
    try {
        // don't exceed maximum threads value
        if ((signed) _threads.size() == _maxThreads) {
            return ErrorHandler::libError(THREAD_LIB_ERROR_THREADS_AMOUNT);
        }

        // get a new ID and create a new thread with that ID
        int aveliableID = _getNewID();

        _threads.insert(std::pair<int, Thread *>(aveliableID,
                                                 new Thread(aveliableID,
                                                            _stackSize, f)));

        if (f != nullptr) {
            _readyThreads.push_back(aveliableID);
        }
        return aveliableID;

    }
    catch (std::bad_alloc &ba) {
        _killProcess();
        ErrorHandler::sysCallError(THREAD_SYS_CALL_ERROR_BAD_ALLOC);
    }
    return FAILURE;
}

//-------------/

/**
 * Delete a thread from the threads DAST including his ID and resources.
 * Moreover, if its the running thread, _runningThread will be updated
 * as NO_RUNNING_THREAD. If not, the thread's id will be erased from the
 * vector of its corresponding thread. (We shall assume that in this case,
 * the only states that will be received are BLOCKED and READY (As we can
 * see, SLEEPING is dealt in the main removing method).
 */
void Scheduler::_removeThreadHelper(int ID, vec *stateVec)
{
    vecIter idPosition;

    if (ID == _runningThread) {
        _runningThread = NO_ACTIVE_THREAD;
    }
    else {
        idPosition = _getIterOfThread(ID, stateVec);
        *stateVec->erase(idPosition);
    }
    _toDelete = _threads[ID];

    _threads.erase(ID);
    _deleteID(ID);
}

/**
 * Removing a Thread.
 * @param ID The ID of the Thread to remove
 * @return SUCCESS on success and FAILURE on failure
 */
int Scheduler::removeThread(int ID) {
    // If its the main thread.
    if (ID == MAIN_THREAD_ID) {
        // Killing the process here works fine (in terms of
        // cleaning up memory).
        _killProcess();

        exit(SUCCESS);
    }

    vec *dastOfID = _getVectorOfThread(ID);
    // ID doesn't exists or its a running thread.
    if (dastOfID == nullptr) {
        // Thread's running.
        if (ID == _runningThread)
        {
            _removeThreadHelper(ID, dastOfID);
            _currentScenario = TOSELFREMOVE;
            return SUCCESS;
        }
            // Bad thread doesn't exist.
        else
        {
            //No thread with this id exists.
            return _badIDChecker(ID);
        }
    }
    else {
        // Thread's BLOCKED or READY.
        _removeThreadHelper(ID, dastOfID);
        return SUCCESS;
    }
}

//-------------/


/**
 * Blocking a Thread.
 * @param ID The ID of the Thread to block.
 * @return SUCCESS on success and FAILURE on failure
 */
int Scheduler::blockThread(int ID) {
    // If trying to block main Thread.
    if (ID == MAIN_THREAD_ID) {
        return ErrorHandler::libError(THREAD_LIB_ERROR_ILLEGAL_MAIN_OP);
    }

    if (ID != _runningThread) {
        // Check if ID is valid
        vec *vecOfThread = _getVectorOfThread(ID);

        if (vecOfThread == nullptr) {
            return _badIDChecker(ID);
        }

        // if BLOCKED or SLEEPING do nothing.
        state threadState = _threads.at(ID)->getState();
        if (threadState == BLOCKED || threadState == SLEEPING) {
            return SUCCESS;
        }
        // Thread's ready to be blocked. First the state shall be changed.
        _threads[ID]->setState(BLOCKED);

        return _accessDBtoInsertAndDelete(&_blockThreads, vecOfThread, ID,
                                          _getIterOfThread(ID, vecOfThread));
    }

    _threads[ID]->setState(BLOCKED);
    _currentScenario = TOBLOCK;

    return SUCCESS;
}

/**
 * Resuming a Thread.
 * @param ID The ID of the Thread to resume.
 * @return SUCCESS on success and FAILURE on failure
 */
int Scheduler::resumeThread(int ID) {

    // Check if ID is valid
    vec *vecOfThread = _getVectorOfThread(ID);
    if (ID != _runningThread && vecOfThread == nullptr) {
        return _badIDChecker(ID);
    }

    // if BLOCKED or SLEEPING do nothing.
    state threadState = _threads.at(ID)->getState();
    if (threadState == RUNNING ||
        threadState == SLEEPING || threadState == READY) {
        return SUCCESS;
    }

    _threads[ID]->setState(READY);

    _accessDBtoInsertAndDelete(&_readyThreads, vecOfThread, ID, \
                               _getIterOfThread(ID, vecOfThread));

    return SUCCESS;
}

//-------------/

/**
 * Make a Thread sleep.
 * @param ID The ID of the Thread that should sleep.
 * @return SUCCESS on success and FAILURE on failure
 */
int Scheduler::sleepThread(int num_quantums) {
    // If trying to block main Thread.
    if (_runningThread == MAIN_THREAD_ID) {
        return ErrorHandler::libError(THREAD_LIB_ERROR_ILLEGAL_MAIN_OP);
    }
    // Check wether the given number of quantums is positive.
    if (num_quantums < 0) {
        return ErrorHandler::libError(THREAD_LIB_ERROR_INPUT);
    }

    int threadID = _runningThread;

    // Thread is successfully set to sleep. Quantums to sleep is set.
    _threads[threadID]->setState(SLEEPING);
    _threads[threadID]->setQuantumsToSleep(num_quantums);
    _threads[threadID]->setQuantumsLeftToSleep(num_quantums);

    _currentScenario = TOSLEEP;

    return SUCCESS;
}

//---------------------------ROUND ROBIN RELATED-----------------------------//

/**
 * Manages all of the sleeping threads. Decreases the time that has left
 * to sleep, and wakes up every thread that finished its sleeping time.
 * @return None
 */
void Scheduler::_manageSleepingThreads(void) {
    for (auto it = _sleepThreads.begin(); it < _sleepThreads.end(); ++it) {
        // decrease sleeping time
        _threads[*it]->decrementQuantumsToSleep();
        // Wake up a sleeping thread.
        if (_threads[*it]->getQuantumsLeftToSleep() == 0) {
            _threads[*it]->setState(READY);
            _accessDBtoInsertAndDelete(&_readyThreads, &_sleepThreads, *it, it);
        }
    }
}

/**
 * Manages the Threads. This is where the Round-Robin decisions are being
 * made. Every scenario will be dealt in this code, and all states and
 * DASTs will be updated as needed.
 * @return None
 */
void Scheduler::manageThreads(void) {
    try
    {
        int newThread;
        int oldThread;

        oldThread = _runningThread;

        _manageSleepingThreads();

        // Deal with each scenario
        switch (_currentScenario) {
            case TOSLEEP:
                _sleepThreads.push_back(oldThread);
                _currentScenario = ROUTINE;
                break;
            case TOBLOCK:
                _blockThreads.push_back(oldThread);
                _currentScenario = ROUTINE;
                break;
            case TOSELFREMOVE:
                _currentScenario = ROUTINE;
                break;
                // Routine.
            default:
                _readyThreads.push_back(oldThread);
                _threads[oldThread]->setState(READY);
                break;
        }

        // Assign threads to DASTs
        newThread = _readyThreads.front();
        _runningThread = newThread;

        _readyThreads.erase(_readyThreads.begin());
        _threads[newThread]->setState(RUNNING);

        // Make a context switch
        _switchThreads(oldThread, newThread);
    }
    catch (std::bad_alloc &ba) {
        _killProcess();
        ErrorHandler::sysCallError(THREAD_SYS_CALL_ERROR_BAD_ALLOC);
    }
}

/**
 * Getter for the running thread ID.
 * @param dummy a dummy param that is passed in order to match the caller
 * signature. Its value is ignored.
 * @return the ID of the running Thread.
 */
int Scheduler::getRunningThreadID(int dummy) {
    dummy = 0;
    return _runningThread + dummy;
}

/**
 * This function returns the number of quantums until the thread
 * with id tid wakes up.
 * @param ID the ID of the thread.
 * @return the number of quantums until the thread wakes up.
 */
int Scheduler::getTimeToWakeUp(int ID) {
    // Check whether a thread exists.
    vec *vecOfThread = _getVectorOfThread(ID);

    // Check if ID is illegal
    if (ID != _runningThread && vecOfThread == nullptr) {
        return _badIDChecker(ID);
    }

    // If thread's sleeping 0 shall be returned.
    if (_threads[ID]->getState() == SLEEPING) {
        return _threads[ID]->getQuantumsLeftToSleep();
    }
    else {
        return 0;
    }

}

/**
 * This function returns the number of quantums the thread with ID tid was
 * in running state.
 * @param ID the ID of the thread
 * @return the number of quantums the tread was in running state.
 */
int Scheduler::getNumOfQuantums(int ID) {
    // Check whether a thread exists.
    vec *vecOfThread = _getVectorOfThread(ID);

    // Check if ID is illegal
    if (ID != _runningThread && vecOfThread == nullptr) {
        if (ID != MAIN_THREAD_ID)
            return _badIDChecker(ID);
    }

    // If thread's sleeping 0 shall be returned.
    return _threads[ID]->getQuantums();
}

/**
 * Getter for the current scenario.
 * @return the current scenario.
 */
int Scheduler::getScenario() {
    return _currentScenario;
}

