#include <assert.h>
#include <sys/types.h>
#include <pthread.h>
#include "kfc.h"
#include <ucontext.h>
#include <stdlib.h>
#include <valgrind/memcheck.h>
#include <stdio.h>
#include "queue.h"

static int inited = 0;
static int nThreads = 0;
static ucontext_t contextArray[1024];


int currentId = 0;
ucontext_t Schedualer;
static queue_t readyq;
queue_t waitq[1024];
void* datarack [1024];
int hasExited[1024];
int madeMemory[1024];
__caddr_t* stacker;


/**
 * Initializes the kfc library.  Programs are required to call this function
 * before they may use anything else in the library's public interface.
 *
 * @param kthreads    Number of kernel threads (pthreads) to allocate
 * @param quantum_us  Preemption timeslice in microseconds, or 0 for cooperative
 *                    scheduling
 *
 * @return 0 if successful, nonzero on failure
 */


 void lady_and_the_trampoline(void *(*start_func)(void *), void *arg){
    kfc_exit(start_func(arg));
 }

 void schedular(){
    
    
        if(readyq.size < 1){
      
            exit(0);
        }
        int nextId  = queue_dequeue(&readyq);
        if(queue_peek(&readyq) != NULL){
    
        }
        nextId--;
       
        ucontext_t swtich = contextArray[nextId];
        currentId = nextId;
        

      
        setcontext(&swtich);
            
    
    
} 
int
kfc_init(int kthreads, int quantum_us)
{
	assert(!inited);
    queue_init(&readyq);
    
    caddr_t stack_base = malloc(KFC_DEF_STACK_SIZE);
    
    Schedualer.uc_stack.ss_sp = stack_base;
    Schedualer.uc_stack.ss_size = KFC_DEF_STACK_SIZE;

    
    VALGRIND_STACK_REGISTER(stack_base, KFC_DEF_STACK_SIZE + stack_base);
    getcontext(&Schedualer);
    makecontext(&Schedualer, (void (*)())schedular,0);
	inited = 1;

    
    
	return 0;
}





/**
 * Cleans up any resources which were allocated by kfc_init.  You may assume
 * that this function is called only from the main thread, that any other
 * threads have terminated and been joined, and that threading will not be
 * needed again.  (In other words, just clean up and don't worry about the
 * consequences.)
 *
 * I won't be testing this function specifically, but it is provided as a
 * convenience to you if you are using Valgrind to check your code, which I
 * always encourage.
 */
void
kfc_teardown(void)
{
    
    for(int i = 0; i <= nThreads; i++){
        queue_destroy(&waitq[i]);
        if(madeMemory[i] == 1){
            free(bingo[i].uc_stack.ss_sp);
        }
    }
    queue_destroy(&readyq);
	assert(inited);
    free(Schedualer.uc_stack.ss_sp);
    

	inited = 0;
}

/**
 * Creates a new user thread which executes the provided function concurrently.
 * It is left up to the implementation to decide whether the calling thread
 * continues to execute or the new thread takes over immediately.
 *
 * @param ptid[out]   Pointer to a tid_t variable in which to store the new
 *                    thread's ID
 * @param start_func  Thread main function
 * @param arg         Argument to be passed to the thread main function
 * @param stack_base  Location of the thread's stack if already allocated, or
 *                    NULL if requesting that the library allocate it
 *                    dynamically
 * @param stack_size  Size (in bytes) of the thread's stack, or 0 to use the
 *                    default thread stack size KFC_DEF_STACK_SIZE
 *
 * @return 0 if successful, nonzero on failure
 */
int kfc_create(tid_t *ptid, void *(*start_func)(void *), void *arg,
               caddr_t stack_base, size_t stack_size) {
    
    int current = currentId;  
    *ptid = ++nThreads;
    currentId = nThreads;
    
    if (stack_size == 0) {
        stack_size = KFC_DEF_STACK_SIZE;
    }
    if (stack_base == NULL) {
        stack_base = malloc(stack_size);
        madeMemory[currentId] = 1;
        
        VALGRIND_STACK_REGISTER(stack_base, stack_size + stack_base);
    }
    int next = nThreads;
    queue_init(&waitq[next]);
    bingo[next].uc_stack.ss_sp = stack_base;  
    bingo[next].uc_stack.ss_size = stack_size;
    bingo[next].uc_link = &Schedualer;
    //queueing new thread
    queue_enqueue(&readyq, currentId + 1);
  
    getcontext(&bingo[next]);

    makecontext(&bingo[next], lady_and_the_trampoline, 2, start_func, (long)arg);
        currentId = current;
    assert(inited);
    return 0;
}

/**
 * Exits the calling thread.  This should be the same thing that happens when
 * the thread's start_func returns.
 *
 * @param ret  Return value from the thread
 */
void
kfc_exit(void *ret)
{
    datarack[currentId] = ret;
    for(int i = 0; i < waitq[currentId].size; i++){
      
        queue_enqueue(&readyq, queue_dequeue(&waitq[currentId]));
    }
    
    hasExited[currentId] = 1;
    setcontext(&Schedualer);
    
	assert(inited);
}

/**
 * Waits for the thread specified by tid to terminate, retrieving that threads
 * return value.  Returns immediately if the target thread has already
 * terminated, otherwise blocks.  Attempting to join a thread which already has
 * another thread waiting to join it, or attempting to join a thread which has
 * already been joined, results in undefined behavior.
 *
 * @param pret[out]  Pointer to a void * in which the thread's return value from
 *                   kfc_exit should be stored, or NULL if the caller does not
 *                   care.
 *
 * @return 0 if successful, nonzero on failure
 */
int
kfc_join(tid_t tid, void **pret)
{
    
    if(hasExited[(int)tid] == 1){
    //getcontext(&bingo[currentId]);
    //DPRINTF("Oragne\n");
    }
    else{

        queue_enqueue(&waitq[(int)tid], (void*) currentId + 1);
  
        swapcontext(&bingo[currentId],&Schedualer);
    }
    
    *pret = datarack[(int)tid];
    assert(inited);

	return 0;
}

/**
 * Returns a small integer which identifies the calling thread.
 *
 * @return Thread ID of the currently executing thread
 */
tid_t
kfc_self(void)
{
	assert(inited);

	return currentId;
}

/**
 * Causes the calling thread to yield the processor voluntarily.  This may
 * result in another thread being scheduled, but it does not preclude the
 * possibility of the same thread continuing if re-chosen by the scheduling
 * algorithm.
 */
void
kfc_yield(void)
{

    queue_enqueue(&readyq, currentId + 1);
    swapcontext(&bingo[currentId], &Schedualer);
	assert(inited);
}

/**
 * Initializes a user-level counting semaphore with a specific value.
 *
 * @param sem    Pointer to the semaphore to be initialized
 * @param value  Initial value for the semaphore's counter
 *
 * @return 0 if successful, nonzero on failure
 */
int
kfc_sem_init(kfc_sem_t *sem, int value)
{
    sem->money = value;
    queue_init(&sem->bunger);
	assert(inited);
	return 0;
}

/**
 * Increments the value of the semaphore.  This operation is also known as
 * up, signal, release, and V (Dutch verhoog, "increase").
 *
 * @param sem  Pointer to the semaphore which the thread is releasing
 *
 * @return 0 if successful, nonzero on failure
 */
int
kfc_sem_post(kfc_sem_t *sem)
{   
    assert(inited);
    sem->money++;
    if(sem->money <= 0){
        queue_enqueue(&readyq, (queue_dequeue(&sem->bunger)-1)+1);
        
    }
	
	return 0;
}

/**
 * Attempts to decrement the value of the semaphore.  This operation is also
 * known as down, acquire, and P (Dutch probeer, "try").  This operation should
 * block when the counter is not above 0.
 *
 * @param sem  Pointer to the semaphore which the thread wishes to acquire
 *
 * @return 0 if successful, nonzero on failure
 */
int
kfc_sem_wait(kfc_sem_t *sem)
{
	sem->money--;
    if(sem->money < 0){
        queue_enqueue(&sem->bunger, (currentId + 1));
        swapcontext(&bingo[currentId], &Schedualer);
    }
	assert(inited);
	return 0;
	return 0;
}

/**
 * Frees any resources associated with a semaphore.  Destroying a semaphore on
 * which threads are waiting results in undefined behavior.
 *
 * @param sem  Pointer to the semaphore to be destroyed
 */
void
kfc_sem_destroy(kfc_sem_t *sem)
{
    queue_destroy(&sem->bunger);
	assert(inited);
}
