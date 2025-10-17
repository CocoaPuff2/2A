
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
 *
 */


#include "sthread.cpp"

void func1( void *arg ) {
    sthread_init( );
    for ( int i = 0; i < 10; i++ ) {
        cout << "func1: " << (char *)arg << " " <<  i << endl;
        sleep( 1 );
        sthread_yield( );
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

    // start the sceduler
    scheduler_start( );

    return 0;
}