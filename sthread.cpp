#include <setjmp.h>   // setjmp()
#include <signal.h>   // signal()
#include <unistd.h>   // sleep(), alarm()
#include <stdio.h>    // perror()
#include <stdlib.h>   // exit()
#include <iostream>   // cout, cerr
#include <string.h>   // memcpy
#include <queue>      // queue

using namespace std;

// ------------------------------------------------
// Scheduler initialization
#define scheduler_init() {                 \
    if (setjmp(main_env) == 0)             \
        scheduler();                       \
}

#define scheduler_start() {                \
    if (setjmp(main_env) == 0)             \
        longjmp(scheduler_env, 1);         \
}

// Only changed below here ------------------------------------------------

// capture() saves the current thread's SP, BP, and stack into cur_tcb
#define capture() { \
    register void* sp asm("sp"); \
    register void* bp asm("bp"); \
    cur_tcb->sp = sp; \
    cur_tcb->size = (int)((char*)bp - (char*)sp); \
    \
    /* Allocate the stack once (no freeing, no realloc) */ \
    if (cur_tcb->stack == NULL && cur_tcb->size > 0) { \
        cur_tcb->stack = malloc(cur_tcb->size); \
    } \
    \
    if (cur_tcb->stack && cur_tcb->size > 0) { \
        memcpy(cur_tcb->stack, sp, cur_tcb->size); \
    } \
    \
    /* Avoid duplicate push: only push if not already queued */ \
    if (thr_queue.empty() || thr_queue.back() != cur_tcb) { \
        thr_queue.push(cur_tcb); \
    } \
}

// sthread_yield() voluntarily gives up CPU if alarmed
#define sthread_yield() { \
    if (alarmed) { \
        if (setjmp(cur_tcb->env) == 0) { \
            capture(); \
            alarmed = false; \
            longjmp(scheduler_env, 1); \
        } \
        /* Restore this thread's stack after returning */ \
        if (cur_tcb->stack && cur_tcb->size > 0) { \
            memcpy(cur_tcb->sp, cur_tcb->stack, cur_tcb->size); \
        } \
    } \
}


// Only changed above here ------------------------------------------------
#define sthread_init() {                   \
    if (setjmp(cur_tcb->env) == 0) {       \
        capture();                         \
        longjmp(main_env, 1);              \
    }                                      \
    memcpy(cur_tcb->sp, cur_tcb->stack, cur_tcb->size); \
}

#define sthread_create(function, arguments) { \
    if (setjmp(main_env) == 0) {              \
        func = &function;                     \
        args = arguments;                     \
        thread_created = true;                \
        cur_tcb = new TCB();                  \
        longjmp(scheduler_env, 1);            \
    }                                         \
}

#define sthread_exit() {                     \
    if (cur_tcb->stack != NULL)              \
        free(cur_tcb->stack);                \
    longjmp(scheduler_env, 1);               \
}

// ------------------------------------------------
// Global variables
static jmp_buf main_env;
static jmp_buf scheduler_env;

class TCB {
public:
    TCB() : sp(NULL), stack(NULL), size(0) { }
    jmp_buf env;   // execution environment
    void* sp;      // stack pointer
    void* stack;   // saved stack buffer
    int size;      // size of stack
};

static TCB* cur_tcb = NULL;     // current thread
static queue<TCB*> thr_queue;   // ready queue
static bool alarmed = false;

static void sig_alarm(int signo) {
    alarmed = true;
}

// Thread function pointers
void (*func)(void*);
void* args = NULL;
static bool thread_created = false;

// ------------------------------------------------
// Scheduler function
static void scheduler() {
    if (setjmp(scheduler_env) == 0) {
        cerr << "scheduler: initialized" << endl;
        if (signal(SIGALRM, sig_alarm) == SIG_ERR) {
            perror("signal function");
            exit(-1);
        }
        longjmp(main_env, 1);
    }

    if (thread_created == true) {
        thread_created = false;
        (*func)(args);
    }

    if ((cur_tcb = thr_queue.front()) != NULL) {
        thr_queue.pop();
        alarm(5);  // 5-second time quantum
        longjmp(cur_tcb->env, 1);
    }

    cerr << "scheduler: no more threads to schedule" << endl;
    longjmp(main_env, 2);
}
