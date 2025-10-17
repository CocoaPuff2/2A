
/*
 * GOAL: implements a very simple user thread library
 * and its round-robin scheduler, say sthread
 *
 *  use setjmp( ) and longjmp( ): restore the current execution environment
 *  asm( ): retrieve stack and base pointers from the CPU register set
 *
 *  minimum resources of each user thread are two-fold:
 *  (1) its on-going CPU register contents
 *           captured and restored as the jmp_env structure
 *           with setjmp( ) and logjmp,
 *  (2) its stack.
 *       identified as the current activation record
 *       that is a stack space between the current  SP
 *       based on BP
 *
 *  To manage multiple user threads, we need to allocate to each
 *  thread a thread control block (TCB) that includes its jmp_env
 *  and activation record.
 *
 * How switching works?
 * Each thread has a TCB that holds jmp_buf env,
 * pointer to its saved stack area, and size.
 *
 * To pause a thread you:
 *  1) save its registers with setjmp,
 *  2) compute size between BP and SP,
 *  3) memcpy that stack area into the TCB’s heap buffer.
 *
 *  To resume a thread you longjmp to its saved env and memcpy
 *  the stack back to its original location so execution continues correctly.
 *
 */


#include "sthread.cpp"

// prints function name, counts from 0-9
void func1( void *arg ) {
    sthread_init( ); // init thread so it can return to scheduler
    for ( int i = 0; i < 10; i++ ) {
        cout << "func1: " << (char *)arg << " " <<  i << endl;
        sleep( 1 ); //  simulates work with 1 second delay
        sthread_yield( ); // gives up CPU after each iteration
    }
    sthread_exit( );
}

void func2( void *arg ) {
    sthread_init( );
    for ( int i = 0; i < 10; i++ ) {
        cout << "func2: " << (char *)arg << " " <<  i << endl;
        sleep( 1 );
        sthread_yield( );
    }
    sthread_exit( );
}

void func3( void *arg ) {
    sthread_init( );
    for ( int i = 0; i < 10; i++ ) {
        cout << "func3: " << (char *)arg << " " <<  i << endl;
        sleep( 1 );
        sthread_yield( );
    }
    sthread_exit( );
}

int main( void ) {
    // initialize the thread scheduler
    scheduler_init( );

    // start user threads
    sthread_create( func1, (void *)"Bothell" );
    sthread_create( func2, (void *)"Seattle" );
    sthread_create( func3, (void *)"Tacoma" );

    // start the scheduler, runs in RR order
    scheduler_start( );

    return 0;
}