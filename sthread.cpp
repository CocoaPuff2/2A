#include <setjmp.h>   // setjmp()
#include <signal.h>   // signal()
#include <unistd.h>   // sleep(), alarm()
#include <stdio.h>    // perror()
#include <stdlib.h>   // exit()
#include <iostream>   // cout, cerr
#include <string.h>   // memcpy
#include <queue>      // queue

using namespace std;

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

// todo
// setjmp() already stores CPU registers and and PC
// so, manually copy the stack memory portion.
#define capture() { \
   char local_var;   /* to find stack location */ \
   cur_tcb->sp = (void*)&local_var;  /* Save current sp */ \
   cur_tcb->size = (char*)cur_tcb->sp;  /* ASSUME: stack grows down */ \
                    \
   if (current_tcb_size < 0) {                    \
   cur_tcb->size = -cur_tcb->size;         \
   }                                                \
                                                     \
   if (cur_tcb->stack != NULL) {              \
   free(cur_tcb->stack);                      \
   }                \
                    \
   cur_tcb->stack = malloc(cur_tcb->size);\
   memcpy(cur_tcb->stack, cur_tcb->sp, cur_tcb->size);            \
   \
   \
}

// todo
// Gives up CPU to another thread
// 1. save current context (setjmp())
// 2. save thead's current stack (capture())
// 3. put thread back into queue (thr_queue.push(cur_tcb))
// 4. jump to scheduler to pick up new thread
#define sthread_yield() {    \
if (setjmp(cur_tcb->env) == 0) { \
    capture();               \
    thr_queue.push(cur_tcb); \
    longjmp(scheduler_env, 1);   \
                                \
}\
}

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

static jmp_buf main_env;
static jmp_buf scheduler_env;

// thread control block (TCB)
class TCB {
public:
    TCB() : sp(NULL), stack(NULL), size(0) { }

    jmp_buf env;   // execution environment captured by setjmp()
    void* sp;
    void* stack;   // buffer, stores latest stack contents
    int size;      // size of stack contents
};

static TCB* cur_tcb = NULL;     // current thread's TCB
static queue<TCB*> thr_queue;   // queue of active threads

// alarm caught to switch to next thread
static bool alarmed = false;

static void sig_alarm(int signo) {
    alarmed = true;
}

// A function to be executed by a thread
void (*func)(void*);
void* args = NULL;
static bool thread_created = false;


static void scheduler() {
    // initialize scheduler
    if (setjmp(scheduler_env) == 0) {
        cerr << "scheduler: initialized" << endl;

        if (signal(SIGALRM, sig_alarm) == SIG_ERR) {
            perror("signal function");
            exit(-1);
        }

        longjmp(main_env, 1);
    }

    // check if called from sthread_create()
    if (thread_created == true) {
        thread_created = false;
        (*func)(args);
    }

    // restore the next thread's environment
    if ((cur_tcb = thr_queue.front()) != NULL) {
        thr_queue.pop();

        // allocate a time quantum of 5 seconds
        alarm(5);

        // resume the next thread's execution
        longjmp(cur_tcb->env, 1);
    }

    // no threads to schedule, simply return
    cerr << "scheduler: no more threads to schedule" << endl;
    longjmp(main_env, 2);
}
