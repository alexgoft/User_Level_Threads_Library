#ifndef EX2_THREAD_H
#define EX2_THREAD_H

#include <signal.h>
#include <setjmp.h>
#include "ErrorHandler.h"

// Typedef for 'unsigned long' , used as a type for addresses.
typedef unsigned long address_t;
// Macros for register numbers.
#define JB_SP 6
#define JB_PC 7

// Used in sigsetjmp() to save the current signal mask.
#define THREAD_SAVE_MASK 1

// Size of "environment" array is 1. (As said in README - used as wrapper).
#define JMP_BUFFER_SIZE 1
// The index of the buffer itself inside the array.
#define JMP_BUFFER_INDX 0
// Initial value of the quantums is updated inside of the constructor.
#define QUANTUMS_NOT_SET -1

// Typedef for a void function that gets
typedef void (*FunctionPointer)(void);

// All possible states the thread can be.
enum state {READY, RUNNING, BLOCKED, SLEEPING};

//---------------------------------------------------------------------------//

/*
 * We've chose to implement threads as a separate class, that holds a pointer
 * to a function (Which will be invoked every time its a given thread turn to
 * run) and a a pointer to dynamically allocated stack.
 * In addition , current information , such as state and number of quantum spent
 * by the CPU on a given thread is saved in the appropriate fields.
 * In order to save the current state of the thread for later sigsetjmp()\
 * siglongjmp() uses, we've chose it "inside" as a local member (variable
 * wrapped by an array).Beyond that,The design and implementation are pretty
 * straightforward - Getters and setters for inner member variables.
 */
class Thread
{
public:

    /**
     * C-tor.
     * @param ID the ID of the Thread.
     * @param stackSize the size of the stack (in bytes).
     * @param f a pointer to the Thread's function.
     */
    Thread(int ID, int stackSize, FunctionPointer f);

    /**
     * D-tor.
     */
    ~Thread();

    /**
     * Getter for the ID.
     * @return the ID of the Thread.
     */
    int getID() const;

    /**
     * Setter for the Thread's state.
     * @param newState the state to change to.
     * @return None.
     */
    void setState(state newState);

    /**
     * Getter for the Thread's state.
     * @return the Thread's state.
     */
    state getState() const;

    /**
     * Setter for the Thread's quantums.
     * @param newQuantums the quantums to change to.
     * @return None
     */
    void setQuantums(int newQuantums);

    /**
     * Getter for the Thread's quantums.
     * @return the Thread's quantums.
     */
    int getQuantums() const;

    /**
     * Increase thread's quantums by one.
     * @return None
     */
    void incrementQuantum(void);

    /**
     * Decrease thread's quantums by one.
     * @return None
     */
    void decrementQuantumsToSleep(void);


    /**
     * Setter for the Thread's quantums sleeping period.
     * @param newQuantums the quantums to change to.
     * @return None.
     */
    void setQuantumsToSleep(int newQuantums);

    /**
     * Getter for the Thread's quantums sleeping period.
     * @return the Thread's quantums.
     */
    int getQuantumsToSleep() const;

    /**
     * Setter for the Thread's quantums left to sleeping.
     * @param newQuantums the quantums to change to.
     * @return None.
     */
    void setQuantumsLeftToSleep(int newQuantums);

    /**
     * Getter for the Thread's quantums left to sleeping.
     * @return the Thread's quantums.
     */
    int getQuantumsLeftToSleep() const;

    /**
     * Access the Thread's environment.
     * @return a pointer to the Thread's environment.
     */
    sigjmp_buf * environment();

private:

    /**
     * The ID of the Thread
     */
    int _ID;

    /**
     * The state of the Thread
     */
    state _state;

    /**
     * The function of the Thread
     */
    FunctionPointer _function;

    /**
     * The number of quantums the Thread was in RUNNING state.
     */
    int _quantums;

    /**
     * The number of quantums the Thread will be in SLEEPING state.
     * Initialized to QUANTUMS_NOT_SET
     */
    int _quantumsToSleep;

    /**
     * The number of quantums the Thread has left to be in SLEEPING state.
     * Initialized to QUANTUMS_NOT_SET
     */
    int _quantumsLeftToSleep;



    /**
     * Pointer to the stack of the Thread
     */
    char* _stack;

    /**
     * The environments of the Thread (an array that holds the buffers)
     * Note that only 1 is used by default.
     */
    sigjmp_buf * _env;

};

#endif //EX2_THREAD_H
