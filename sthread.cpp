#include <setjmp.h>   // setjmp(), longjmp()
#include <signal.h>   // signal()
#include <unistd.h>   // sleep(), alarm()
#include <stdio.h>    // perror()
#include <stdlib.h>   // exit(), malloc(), free()
#include <iostream>   // cout, cerr
#include <string.h>   // memcpy
#include <queue>      // queue

using namespace std;

// ------------------------------------------------
// Scheduler macros

// Initializes scheduler: save main env with setjmp()
// then jump into scheduler() function
#define scheduler_init() {                 \
    if (setjmp(main_env) == 0)             \
        scheduler();                       \
}

// Starts scheduler, saves main env,
// and jumps into scheduler saved state
#define scheduler_start() {                \
    if (setjmp(main_env) == 0)             \
        longjmp(scheduler_env, 1);         \
}

// Capture current stack into TCB
#define capture() { \
    char local_var; \
    cur_tcb->sp = (void*)&local_var; /* Save current SP */ \
    \
    /* compute stack size: assume stack grows downward */ \
    if (cur_tcb->stack_base != NULL) { \
        cur_tcb->size = (char*)cur_tcb->stack_base - (char*)cur_tcb->sp; \
        if (cur_tcb->size < 0) cur_tcb->size = -cur_tcb->size; \
    } else { \
        cur_tcb->size = 8192; /* default max stack size if stack_base not set */ \
    } \
    \
    if (cur_tcb->stack != NULL) free(cur_tcb->stack); \
    cur_tcb->stack = malloc(cur_tcb->size); \
    memcpy(cur_tcb->stack, cur_tcb->sp, cur_tcb->size); \
}

// Gives up CPU to another thread
#define sthread_yield() {    \
    if (setjmp(cur_tcb->env) == 0) { \
        capture();               \
        thr_queue.push(cur_tcb); \
        longjmp(scheduler_env, 1); \
    } \
}

// Initialize a thread's environment
#define sthread_init() {                   \
    if (setjmp(cur_tcb->env) == 0) {       \
        capture();                         \
        longjmp(main_env, 1);              \
    }                                      \
    memcpy(cur_tcb->sp, cur_tcb->stack, cur_tcb->size); \
}

// Create a new thread
#define sthread_create(function, arguments) { \
    if (setjmp(main_env) == 0) {              \
        func = &function;                     \
        args = arguments;                     \
        thread_created = true;                \
        cur_tcb = new TCB();                  \
        cur_tcb->stack_base = (void*)((char*)malloc(8192) + 8192); /* top of stack */ \
        longjmp(scheduler_env, 1);            \
    }                                         \
}

// Exit current thread
#define sthread_exit() {                     \
    if (cur_tcb->stack != NULL)              \
        free(cur_tcb->stack);                \
    if (cur_tcb->stack_base != NULL)         \
        free((char*)cur_tcb->stack_base - 8192); /* free base allocation */ \
    longjmp(scheduler_env, 1);               \
}

// ------------------------------------------------
// Globals

static jmp_buf main_env;
static jmp_buf scheduler_env;

// Thread Control Block
class TCB {
public:
    TCB() : sp(NULL), stack(NULL), stack_base(NULL), size(0) { }

    jmp_buf env;       // execution environment
    void* sp;          // current stack pointer
    void* stack;       // buffer to store stack contents
    void* stack_base;  // base/top of stack
    int size;          // size of stack contents
};

static TCB* cur_tcb = NULL;     // current thread's TCB
static queue<TCB*> thr_queue;   // queue of active threads

// Alarm for preemption
static bool alarmed = false;
static void sig_alarm(int signo) {
    alarmed = true;
}

// Thread function pointer
void (*func)(void*);
void* args = NULL;
static bool thread_created = false;

// ------------------------------------------------
// Scheduler

static void scheduler() {
    // Initialize scheduler
    if (setjmp(scheduler_env) == 0) {
        cerr << "scheduler: initialized" << endl;

        if (signal(SIGALRM, sig_alarm) == SIG_ERR) {
            perror("signal function");
            exit(-1);
        }

        longjmp(main_env, 1);
    }

    // If called from sthread_create(), run the thread
    if (thread_created) {
        thread_created = false;
        (*func)(args);
    }

    // Get next thread from queue
    if (!thr_queue.empty() && (cur_tcb = thr_queue.front()) != NULL) {
        thr_queue.pop();

        // allocate a time quantum of 5 seconds
        alarm(5);

        // resume thread execution
        longjmp(cur_tcb->env, 1);
    }

    // no threads left
    cerr << "scheduler: no more threads to schedule" << endl;
    longjmp(main_env, 2);
}
