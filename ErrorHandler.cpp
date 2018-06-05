#include "ErrorHandler.h"

/**
 * Printing the error stderr and returning FAILURE.
 * @param text The error text
 * return FAILURE
 */
    int ErrorHandler::libError(string text) {
        cerr << THREAD_LIB_ERROR << text << endl;
        return FAILURE;
    }

/**
* Printing the error stderr and exiting with FAILURE
* @param text The error text
*/
    void ErrorHandler::sysCallError(string text) {
        cerr << THREAD_SYS_CALL_ERROR << text << endl;
        exit(EXIT_STATUS);
    }
