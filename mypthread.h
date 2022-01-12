#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H
#define _GNU_SOURCE

#define USE_MY_PTHREAD 1
#define LOWEST_PRIORITY 3
#define STACK_SIZE 1048576
#define TIME_QUANTUM 10

#ifdef MLFQ
          #define SCHED MLFQ_SCHEDULER
#elif FIFO
          #define SCHED FIFO_SCHEDULER
#else
          #define SCHED STCF_SCHEDULER
#endif

#include<unistd.h>
#include<time.h>
#include<sys/syscall.h>
#include<sys/types.h>
#include<sys/time.h>
#include<stdio.h>
#include<stdlib.h>
#include<ucontext.h>
#include<malloc.h>
typedef enum _status{
		     READY,RUNNING,DONE,BLOCKED}status;
typedef enum _scheduler{
		     MLFQ_SCHEDULER,STCF_SCHEDULER,FIFO_SCHEDULER}scheduler;
typedef uint my_pthread_t;
void *returnValues[1000000];

typedef struct threadControlBlock{
  int join_boolean;
  struct my_pthread_mutex_t* blocked_from;
  my_pthread_t *threadId;
  status thread_status;
  ucontext_t context;
  ucontext_t return_context;
  int priority;
  unsigned long int time_ran;
}tcb;

typedef struct my_pthread_mutex_t{
  int mutexId;
  int isLocked;
}my_pthread_mutex_t;

typedef struct _mutexNode{
  struct _mutexNode* next;
  my_pthread_mutex_t *mutex;
}mutexNode;

typedef struct _queueNode{
  tcb* thread_tcb;
  struct _queueNode* next;
}queueNode;

typedef struct _threadQueue{
  struct _queueNode* head;
  struct _queueNode* tail;
}threadQueue;

typedef struct _multiQueue{
  threadQueue *queue0;
  threadQueue *queue1;
  threadQueue *queue2;
  threadQueue *queue3;
}multiQueue;

int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);
int my_pthread_yield();
void my_pthread_exit(void *value_ptr);
int my_pthread_join(my_pthread_t thread, void **value_ptr);
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

void SIGALRM_Handler();
void processFinishedJob(int);
tcb* searchMLFQ(int);
tcb* findThread(int);
tcb* findThreadHelper(int);

static void schedule();
queueNode* getRunningThread();
queueNode *getNextToRun();

int removeFromQueueHelper(queueNode*);
void removeFromQueue(queueNode*);
void removeFromMLFQ(queueNode*);
int removeFromQueueHelper(queueNode*);
int removeFromQueueHelper_NoFree(queueNode*);
queueNode* removeFromQueue_NoFree(queueNode*);
void removeFromMLFQ_NoFree(queueNode*);
void updateThreadPosition(queueNode*);

void start_timer(int);
mutexNode *findMutex(int);
void freeQueueNode(queueNode*);
void freeTcb(tcb*);
void printMLFQ();
void printQ(threadQueue *queueToPrint);

#ifdef USE_MY_PTHREAD
#define pthread_t my_pthread_t
#define pthread_mutex_t my_pthread_mutex_t
#define pthread_create my_pthread_create
#define pthread_exit my_pthread_exit
#define pthread_join my_pthread_join
#define pthread_mutex_init my_pthread_mutex_init
#define pthread_mutex_lock my_pthread_mutex_lock
#define pthread_mutex_unlock my_pthread_mutex_unlock
#define pthread_mutex_destroy my_pthread_mutex_destroy
#endif

#endif
			 

  
