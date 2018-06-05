// To print errors.

#include "Thread.h"


//---------------------------------------------------------------------------//

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
static address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
            "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

//-----------------------CONSTRUCTORS DESTRUCTORS----------------------------//

/**
 * C-tor.
 * @param ID the ID of the Thread.
 * @param stackSize the size of the stack (in bytes).
 * @param f a pointer to the Thread's function.
 */
Thread::Thread(int ID, int stackSize, FunctionPointer f)
: _ID(ID),
  _state(READY),
  _function(f),
  _quantums(0),
  _quantumsToSleep(QUANTUMS_NOT_SET),
  _quantumsLeftToSleep(QUANTUMS_NOT_SET)
{
    // Construct and initialize env buffer
    _env = new(nothrow) sigjmp_buf[JMP_BUFFER_SIZE];
    if (_env == nullptr) {
        ErrorHandler::sysCallError(THREAD_SYS_CALL_ERROR_BAD_ALLOC);
    }

    // If its not the main thread.
    if (f != nullptr) {
        // Allocate stack
        _stack = new(nothrow)char[stackSize];
        if (_stack == nullptr) {
            delete[] _env;
            ErrorHandler::sysCallError(THREAD_SYS_CALL_ERROR_BAD_ALLOC);
        }

        // The address to the given function and created memory.
        address_t sp, pc;

        // translate and init stack and PC pointers
        sp = (address_t) _stack + stackSize - sizeof(address_t);
        pc = (address_t) _function;
        sigsetjmp(_env[JMP_BUFFER_INDX], THREAD_SAVE_MASK);

        (_env[JMP_BUFFER_INDX]->__jmpbuf)[JB_SP] = translate_address(sp);
        (_env[JMP_BUFFER_INDX]->__jmpbuf)[JB_PC] = translate_address(pc);

        // reset buffer mask
        if(sigemptyset(&(_env[JMP_BUFFER_INDX]->__saved_mask)) < 0)
        {
            ErrorHandler::sysCallError(THREAD_SYS_CALL_ERROR_SIGNAL_HANDLE);
        }
    }
}

Thread::~Thread()
{
    delete [] _stack;
    delete [] _env;
}

//-------------------------------GETTERS-------------------------------------//

/**
 * Getter for the ID.
 * @return the ID of the Thread.
 */
int Thread::getID(void) const
{
    return _ID;
}

/**
 * Setter for the Thread's state.
 * @param newState the state to change to.
 * @return None.
 */
void Thread::setState(state newState)
{
    _state = newState;
}

/**
 * Getter for the Thread's state.
 * @return the Thread's state.
 */
state Thread::getState(void) const
{
    return _state;
}

/**
 * Setter for the Thread's quantums.
 * @param newQuantums the quantums to change to.
 * @return None
 */
void Thread::setQuantums(int newQuantums)
{
    _quantums = newQuantums;
}

/**
 * Getter for the Thread's quantums.
 * @return the Thread's quantums.
 */
int Thread::getQuantums(void) const
{
    return _quantums;
}

/**
 * Increase thread's quantums by one.
 * @return None
 */
void Thread::incrementQuantum(void)
{
    _quantums++;
}

/**
 * Decrease thread's quantums by one.
 * @return None
 */
void Thread::decrementQuantumsToSleep(void)
{
    _quantumsLeftToSleep--;
}

/**
 * Setter for the Thread's quantums sleeping period.
 * @param newQuantums the quantums to change to.
 * @return None.
 */
void Thread::setQuantumsToSleep(int newQuantums)
{
    _quantumsToSleep = newQuantums;
}

/**
 * Getter for the Thread's quantums sleeping period.
 * @return the Thread's quantums.
 */
int Thread::getQuantumsToSleep(void) const
{
    return _quantumsToSleep;
}

/**
 * Setter for the Thread's quantums left to sleeping.
 * @param newQuantums the quantums to change to.
 * @return None.
 */
void Thread::setQuantumsLeftToSleep(int newQuantums)
{
    _quantumsLeftToSleep = newQuantums;
}

/**
 * Getter for the Thread's quantums left to sleeping.
 * @return the Thread's quantums.
 */
int Thread::getQuantumsLeftToSleep(void) const
{
    return _quantumsLeftToSleep;
}

/**
 * Access the Thread's environment.
 * @return a pointer to the Thread's environment.
 */
sigjmp_buf * Thread::environment(void)
{
    return &_env[JMP_BUFFER_INDX];
}
