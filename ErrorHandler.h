#ifndef EX2_ERRORHANDLER_H
#define EX2_ERRORHANDLER_H

#include <iostream>

#define SUCCESS 0
#define FAILURE -1

#define EXIT_STATUS 1

// Error Text
#define THREAD_LIB_ERROR "thread library error: "
#define THREAD_SYS_CALL_ERROR "system error: "

// Library failures:
#define THREAD_LIB_ERROR_ILLEGAL_MAIN_OP "Invalid operation on main thread"
#define THREAD_LIB_ERROR_ID_OUT_RANGE "Thread ID must be between 0 and 99"
#define THREAD_LIB_ERROR_NO_SUCH_ID "No such ID in the thread DAST"

#define THREAD_LIB_ERROR_NEGATIVE_QUANTUM "Quantoms must be positive"
#define THREAD_LIB_ERROR_INPUT "Invalid input"
#define THREAD_LIB_ERROR_THREADS_AMOUNT "Already reached max number of threads"

// Sys Call failures:
#define THREAD_SYS_CALL_ERROR_BAD_ALLOC "Allocation failed"
#define THREAD_SYS_CALL_ERROR_TIMER_FAILED "Timer usage failure"
#define THREAD_SYS_CALL_ERROR_SIGNAL_HANDLE "Signal handling failure"
#define THREAD_SYS_CALL_ERROR_TIMER "Time initialization failed"

using namespace std;

/*
 * This simple class is responsible for serving other classes in terms
 * of error management.
 */
class ErrorHandler
{
public:
/**
 * Printing the error stderr and returning FAILURE.
 * @param text The error text
 * return FAILURE
 */
    static int libError(string text);

/**
* Printing the error stderr and exiting with FAILURE
* @param text The error text
*/
    static void sysCallError(string text);
};

#endif //EX2_ERRORHANDLER_H
