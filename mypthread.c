// List all group member's name: 
// username of iLab:ilab1.cs
// iLab Server:ilab1.cs.rutgers

#include "mypthread.h"

ucontext_t schedulerContext;
ucontext_t parentContext;
int threadCounter=0;
int mutexCounter=0;
int yielded=0;
threadQueue* threadQ=NULL;
queueNode* runningThread=NULL;
multiQueue* multiQ=NULL;
mutexNode* mutexList=NULL;
my_pthread_mutex_t qLock;
int ignoreSignal=0;
//ucontext_t processFinishedJobContext;

void threadWrapper(void * arg, void *(*function)(void*), int threadId){
  void * threadReturnValue = (*function)(arg);//
  returnValues[threadId] = threadReturnValue; // saving the threads return value in an array
}

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr,
                      void *(*function)(void*), void * arg) {
  //BLOCK TIMER FROM INTERRPTING    
  ignoreSignal=1;
  *thread=++threadCounter;
  if(threadQ==NULL)
  {
    getcontext(&schedulerContext);
    //where to return after function is done?
    schedulerContext.uc_link=0;
    //Define stack
    schedulerContext.uc_stack.ss_sp=malloc(STACK_SIZE);
    //Define stack size
    schedulerContext.uc_stack.ss_size=STACK_SIZE;
    //Set no flags
    schedulerContext.uc_stack.ss_flags=0;
    //Double check memory was allocated
    if (schedulerContext.uc_stack.ss_sp == 0 )
    {
           perror("Could not allocate space for return context");
           exit( 1 );
    }
    makecontext(&schedulerContext, (void*)&schedule, 0);
  }



  //printf("thread ID is? %d",*thread);
	// Create Thread Control Block
  tcb* new_tcb=(tcb*) malloc(sizeof(tcb));
  new_tcb->priority=0;
  //new_tcb->time_quantum_counter=0;
  new_tcb->time_ran=0;

  new_tcb->threadId = *thread;
  new_tcb->thread_status=READY;
  new_tcb->join_boolean=0; //FALSE
  new_tcb->blocked_from=NULL; //FALSE
	
  getcontext(&(new_tcb->return_context));
  (new_tcb->return_context).uc_link=&schedulerContext;
  //Define stack
  (new_tcb->return_context).uc_stack.ss_sp=malloc(STACK_SIZE);
  //Define stack size
  (new_tcb->return_context).uc_stack.ss_size=STACK_SIZE;
  //Set no flags
  (new_tcb->return_context).uc_stack.ss_flags=0;
  //Double check memory was allocated
  if ((new_tcb->return_context).uc_stack.ss_sp == 0 )
  {
         perror("Could not allocate space for return context");
         exit( 1 );
  }
  makecontext(&(new_tcb->return_context), (void*)&processFinishedJob, 1, new_tcb->threadId);
  
  ucontext_t newThreadContext;
  getcontext(&newThreadContext);
  //where to return after function is done?
  newThreadContext.uc_link=&(new_tcb->return_context);
  //Define stack
  newThreadContext.uc_stack.ss_sp=malloc(STACK_SIZE);
  //Define stack size
  newThreadContext.uc_stack.ss_size=STACK_SIZE;
  //Set no flags
  newThreadContext.uc_stack.ss_flags=0;
  //Double check memory was allocated
  if (newThreadContext.uc_stack.ss_sp == 0 )
  {
         perror("Could not allocate space for thread");
         exit( 1 );
  }
  makecontext(&newThreadContext, (void*)threadWrapper, 3, arg, function, (int)new_tcb->threadId);
  new_tcb->context=newThreadContext;

  queueNode* qNode =(queueNode*) malloc(sizeof(queueNode*));
  qNode->thread_tcb=new_tcb;
  qNode->next=NULL;




  if(threadQ==NULL)
  {
    signal(SIGALRM, SIGALRM_Handler);
    start_timer(TIME_QUANTUM);
    if(SCHED == MLFQ_SCHEDULER){
      threadQ = (threadQueue*)12345; // HA!!!!
      multiQ = (multiQueue*)malloc(sizeof(multiQueue));
      multiQ->queue0 = (threadQueue*) malloc(sizeof(threadQueue));
      multiQ->queue1 = (threadQueue*) malloc(sizeof(threadQueue));
      multiQ->queue2 = (threadQueue*) malloc(sizeof(threadQueue));
      multiQ->queue3 = (threadQueue*) malloc(sizeof(threadQueue));
      //printf("Created Multi Queue\n");

      multiQ->queue0->head = qNode;
      multiQ->queue0->tail = qNode;
      multiQ->queue1->head = NULL;
      multiQ->queue1->tail = NULL;
      multiQ->queue2->head = NULL;
      multiQ->queue2->tail = NULL;
      multiQ->queue3->head = NULL;
      multiQ->queue3->tail = NULL;
    }
    else if(SCHED == STCF_SCHEDULER){
      threadQ=(threadQueue*) malloc(sizeof(threadQueue));
      //initialize the head and tail
      threadQ->head=qNode;
      threadQ->tail=NULL;
    }
    else if(SCHED == FIFO_SCHEDULER){
      threadQ=(threadQueue*) malloc(sizeof(threadQueue));
      //initialize the head and tail
      threadQ->head=qNode;
      threadQ->tail=qNode;
    }
    //printf("Made the queue\n");
    pthread_mutex_init(&qLock,NULL);
    //printf("getting main context\n");
    getcontext(&parentContext);

  }
  else{
    // threads exist
    if(SCHED == MLFQ_SCHEDULER){

      if(multiQ->queue0->head == NULL){
        multiQ->queue0->head = qNode;
        multiQ->queue0->tail= qNode;
      }
      else{
        multiQ->queue0->tail->next=qNode;
        multiQ->queue0->tail=qNode;
      }


    }
    else if(SCHED == STCF_SCHEDULER){
      if(threadQ->head == NULL){
        threadQ->head = qNode;
      }
      else{
        qNode->next = threadQ->head;
        threadQ->head=qNode;
      }
    }
    else if(SCHED == FIFO_SCHEDULER){
      if(threadQ->head == NULL){
        threadQ->head = qNode;
        threadQ->tail= qNode;

      }
      else{
        threadQ->tail->next=qNode;
        threadQ->tail=qNode;
      }
    }
  }

  ignoreSignal=0;
	return 0;
};

int mypthread_yield() {
	// YOUR CODE HERE
  yielded=1;
  SIGALRM_Handler();
	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {



  ignoreSignal=1;

  queueNode* finishedQNode = getRunningThread();//getTopOfQueue();
  tcb* finishedThread=finishedQNode->thread_tcb;
  finishedThread->thread_status=DONE;
  if(value_ptr!=NULL){
    //TODO: equals *value_ptr or value_ptr??
    returnValues[(int)finishedThread->threadId]=value_ptr;
    //printf("In pthread_exit, thread ==> (%d) has return value ==> (%d)\n", (int)finishedThread->threadId, returnValues[(int)finishedThread->threadId]);
  }

  ignoreSignal=0;
  SIGALRM_Handler();

}


/* wait for thread termination */
int mypthread_join(my_pthread_t thread, void **value_ptr) {
  ignoreSignal=1;
 

  //First, check if thread is in the queueNode
  tcb* thread_to_join=findThread(thread);
  
  if(thread_to_join==NULL){
    //printf("Thread to join (%d) on is no longer in queue\n",thread);
    if(value_ptr!=NULL){
      //USE THREAD variable SINCE thread_to_join should  be freed
      *value_ptr = returnValues[(int)thread];
      //printf("In JOIN, thread ==> (%d) has return value ==> (%d)\n", thread, returnValues[thread]);
    }

    ignoreSignal=0;
    return 0;
  }

  //Thread is in Queue
  else{
    if(thread_to_join->thread_status!=DONE){
      //printf("thread to join (%d) is still executing, yielding CPU\n",thread);
      thread_to_join->join_boolean = 1;
      //schedule();
      ignoreSignal=0;
      SIGALRM_Handler();
      if(value_ptr!=NULL){
        *value_ptr = returnValues[(int)thread];
      return 0;
      //my_pthread_yield();
    }
    //If thread is done, free and return? or setContext to Main?
    else{
      //printf("thread to join (%d) is done\n",thread);
      if(value_ptr!=NULL){
        *value_ptr = returnValues[(int)thread_to_join->threadId];
        //printf("In JOIN, thread ==> (%d) has return value ==> (%d)\n", (int)thread_to_join->threadId, returnValues[(int)thread_to_join->threadId]);
      }
      free(thread_to_join->context.uc_stack.ss_sp);
      free(thread_to_join->return_context.uc_stack.ss_sp);
      free(thread_to_join);
      //TODO: Remove thread from Queue
      //TODO: Return value_ptr if not nULL
      ignoreSignal=0;
      return 0;
  }
  printf("Shouldn't reach here (end of join)\n");
  ignoreSignal=0;
	return 0;
}
}

/* initialize the mutex lock */
int mypthread_mutex_init(my_pthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {
  // Initialize data structures for this mutex
  mutexNode* newMutexNode=(mutexNode*) malloc(sizeof(mutexNode));
  //printf("mutex before Malloc is: %d\n",mutex);
  my_pthread_mutex_t* new_mutex=(my_pthread_mutex_t*)malloc(sizeof(my_pthread_mutex_t));
  newMutexNode->mutex=new_mutex;
  newMutexNode->mutex->isLocked=0;
  newMutexNode->mutex->mutexId=++mutexCounter;
  newMutexNode->next=mutexList;
  mutexList=newMutexNode;
  *mutex=*new_mutex;

  
	return 0;
}

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	ignoreSignal=1;
  mutexNode *mutexToLock = findMutex(mutex->mutexId);

  if(mutexToLock == NULL){
    printf("Mutex %d has not been initialized, cannot lock\n",mutex->mutexId);
    ignoreSignal=0;
    return -1;
  }
  else{
    if(__sync_lock_test_and_set(&(mutexToLock->mutex->isLocked),1)==1){// we want to yield to other threads, until critical section is unlocked
        //printf("mutex %d is locked, yielding cpu\n",mutexToLock->mutex->mutexId);
        //get thread ID
        queueNode* threadThatWasRunning=getRunningThread();//getTopOfQueue();
        threadThatWasRunning->thread_tcb->blocked_from=mutexToLock->mutex;
        threadThatWasRunning->thread_tcb->thread_status=BLOCKED;
        // printf("Blocked thread %d and locked mutex %d\n",threadThatWasRunning->thread_tcb->threadId,mutex->mutexId);
        ignoreSignal=0;
        my_pthread_yield();
    }
    mutexToLock->mutex->isLocked = 1;
    ignoreSignal=0;
    return 0;
  }
	// YOUR CODE HERE
  ignoreSignal=0;
	return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(my_pthread_mutex_t *mutex) {
  ignoreSignal=1;

  mutexNode *mutexToUnlock = findMutex(mutex->mutexId);
  if(mutexToUnlock==NULL)
  {
    printf("No mutex to unlock\n");
    ignoreSignal=0;
    return -1;
  }
  if(__sync_lock_test_and_set(&(mutexToUnlock->mutex->isLocked),0)==0){
    printf("Mutex %d is already unlocked!\n",mutex->mutexId);
    exit(1);
    return -1;
  }
  ignoreSignal=0;
  return 0;
}


/* destroy the mutex */
int mypthread_mutex_destroy(my_pthread_mutex_t *mutex) {
  ignoreSignal=1;
  mutexNode *mutexToDestroy = findMutex(mutex->mutexId);

  if(mutexToDestroy==NULL){
    printf("Cannot destroy right now\n");
    ignoreSignal=0;
    return -1;
  }
  if(mutexToDestroy->mutex->isLocked==1){
    my_pthread_mutex_unlock(mutex);
  }
  mutexNode *mutexPtr = mutexList;
  mutexNode *mutexPrev = mutexPtr;
  while(mutexPtr!=NULL){
    if(mutexPtr->mutex->mutexId==mutexToDestroy->mutex->mutexId){// found
      //printf("Mutex found!!!\n");
      mutexPrev->next = mutexPtr->next;
      free(mutexPtr);
      ignoreSignal=0;
      return 0;
    }
    mutexPrev = mutexPtr;
    mutexPtr = mutexPtr->next;
  }
  // mutex not found
  // shouldn't get here
  printf("mutex to unlock has not been found\n");
  ignoreSignal=0;
	return -1;
}

struct timespec timeCheck;
int firstSchedule=1;


void SIGALRM_Handler(){
  start_timer(TIME_QUANTUM);//setting timer to fire every TIME_QUANTUM milliseconds
  if(ignoreSignal==1){
    return;
  }
  ignoreSignal=1;
  
  queueNode* finishedThread=getRunningThread();//getTopOfQueue();

   
  if(finishedThread==NULL||finishedThread->thread_tcb->thread_status==READY){
    
    int swapStatus=swapcontext(&parentContext,&schedulerContext);
    if(swapStatus!=0){
      printf("Error swapping main and scheduler: %d\n",swapStatus);
      exit(-1);
    }
     //printf("\nResuming Main\n");

     return;
  }
  //top of queue was Running or Done
  else{
    //printf("\ninterrupted from thread %d\n",(finishedThread->thread_tcb->threadId));
    int swapStatus=swapcontext(&(finishedThread->thread_tcb->context),&schedulerContext);
    if(swapStatus!=0){
      printf("Error swapping top of queue and scheduler: %d\n",swapStatus);
      exit(-1);
    }
    // printf("resuming thread %d\n",(finishedThread->thread_tcb->threadId));
    return;
  }

}


/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	
}

/* scheduler */
static void schedule() {


    struct timespec prev_time=timeCheck;
    clock_gettime(CLOCK_REALTIME, &timeCheck);
    unsigned long timeRan=0;
    if(!firstSchedule){
      timeRan=(timeCheck.tv_sec - prev_time.tv_sec) * 1000 + (timeCheck.tv_nsec - prev_time.tv_nsec) / 1000000;
      //printf("scheduling time: %lu milli-seconds\n", timeRan);
    }
    else{
      //printf("first schedule\n");
      firstSchedule=0;

    }
    //get the thread that was just running.
    // queueNode* finishedThread=threadQ->head;
    queueNode* finishedThread=getRunningThread();//getTopOfQueue();
    
    if(finishedThread==NULL || finishedThread->thread_tcb->thread_status==READY){
      if(finishedThread==NULL){
        finishedThread=getNextToRun();
        if(finishedThread==NULL){
          //printf("no jobs in queue\n");
          runningThread=NULL;
          ignoreSignal=0;
          setcontext(&parentContext);
        }
        // printf("Main was running.\n");
      }
      else{
        printf("Finished thread was ready? Error in logic. \n");
      }
      finishedThread->thread_tcb->thread_status=RUNNING;
      runningThread=finishedThread;
      
      yielded=0;
      ignoreSignal=0;
      int setStatus=setcontext(&(finishedThread->thread_tcb->context));
      if(setStatus!=0){
        printf("\nOOPSIES, Swap no work in starting top of queue, error is: %d \nI'm exiting now\n",setStatus);
        exit(0);
      }
    
    }
    //Top of queue has finished executing
    else if(finishedThread->thread_tcb->thread_status==DONE)
    {
      int join_boolean=finishedThread->thread_tcb->join_boolean;
      int finishedThreadId=finishedThread->thread_tcb->threadId;
      removeFromQueue(finishedThread);
      // queueNode* threadToRun=threadQ->head; // get next to run
      queueNode* threadToRun=getNextToRun();//getTopOfQueue(); // get next to run

      if(join_boolean==1){
       
        runningThread=NULL;
        yielded=0;
        ignoreSignal=0;
        int setStatus=setcontext(&parentContext);
        printf("Just set context to join (main) shouldn't be here, set Status is: %d\n",setStatus );
        if(setStatus!=0){
          printf("\nOOPSIES, Swap no work before returning to join, error is: %d \nI'm exiting now\n",setStatus);
          exit(0);
        }
      }
      if(threadToRun==NULL)
      {
      
        runningThread=NULL;
        yielded=0;
        ignoreSignal=0;
        int setStatus=setcontext(&parentContext); // done executing everything in Q, switching back to main
        //printf("if this prints set failed to go back to main\n");

        if(setStatus!=0){
          printf("\nOOPSIES, Swap no work when no threads to run, error is: %d \nI'm exiting now\n",setStatus);
          exit(0);
        }
        //continue;
      }
      else{
        // printf("\nthread (%d) is going to run next\n", threadToRun->thread_tcb->threadId);
        threadToRun->thread_tcb->thread_status=RUNNING;
        runningThread=threadToRun;
       
        yielded=0;
        ignoreSignal=0;
        int setStatus=setcontext(&(threadToRun->thread_tcb->context));
        //int setStatus=swapcontext(&parentContext,&(threadToRun->thread_tcb->context));
        if(setStatus!=0){
          printf("OOPSIES, Set no work after removing a done thread and starting another one, error is: %d \nI'm exiting now\n",setStatus);
          exit(0);

        }
      }

    }
    else{
    //Swapping two running or Blocked threads

    //Change the status of the finished thread to ready
    if(finishedThread->thread_tcb->thread_status==RUNNING)
    {
      finishedThread->thread_tcb->thread_status=READY;
    }
    finishedThread->thread_tcb->time_ran+=timeRan;
    //LOWEST PRIORITY is actually a high value like 4 or 8
    if(yielded==0 && finishedThread->thread_tcb->priority<LOWEST_PRIORITY){

      finishedThread->thread_tcb->priority+=1;
      //printf("High Prioirty thread did not yield, lowering priority\n");
    }
    updateThreadPosition(finishedThread);

    if(threadToRun==NULL){
      printf("ALL THREADS ARE BLOCKED, YOU ARE IN DEADLOCK, EXITING\n");
      exit(-1);
    }
    threadToRun->thread_tcb->thread_status=RUNNING;
    runningThread=threadToRun;
    yielded=0;
    ignoreSignal=0;
    int setStatus=setcontext(&(threadToRun->thread_tcb->context));
    if(setStatus!=0){
      printf("OOPSIES, Swap no work between threads, error is: %d \nI'm exiting now\n",setStatus);
      exit(0);
    }
    // printf("\nset staus should be 0 after swapping two threads:  %d\n",setStatus);
    }

    printf("end of scheduler, should not be here,\n");
    //ALLOW TIMER TO CONTINUE
    //sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
    runningThread=NULL;
    yielded=0;
    ignoreSignal=0;
    setcontext(&parentContext);


}
void printMLFQ(){
  printf("\n\n&&*********Q_0********\n");
  printQ(multiQ->queue0);
  printf("\n\n&&*********Q_1********\n");
  printQ(multiQ->queue1);
  printf("\n\n&&*********Q_2********\n");
  printQ(multiQ->queue2);
  printf("\n\n&&*********Q_3********\n\n");
  printQ(multiQ->queue3);
}
void printQ(threadQueue *queueToPrint){
  queueNode *ptr = queueToPrint->head;
  printf("&&&&&&&&&&&&&&\n");
  while(ptr!=NULL){
    printf("(Thread: %d, Runtime: %d, Status: %d)===>\n", ptr->thread_tcb->threadId, ptr->thread_tcb->time_ran, ptr->thread_tcb->thread_status);
    ptr=ptr->next;
  }
  printf("&&&&&&&&&&&&&&\n");
}

// YOUR CODE HERE
//Marks finished Threads as DONE
void processFinishedJob(int threadID){
  
  ignoreSignal=1;
  // printf("\nA job just finished!!!! with ID %d \n",threadID);
  tcb* finishedThread=findThread(threadID);
  // printf("Found thread! about to interrupt to remove this thread!\n");
  if(finishedThread->thread_status==BLOCKED){
    printf("finished a blocked thread?? thats wrong\n");
    exit(-1);
  }
  finishedThread->thread_status=DONE;

  ignoreSignal=0;
  SIGALRM_Handler();
}



tcb* searchMLFQ(int threadID){
  tcb *returnThread = NULL;
  //first search queue0
  threadQ = multiQ->queue0;
  returnThread = findThreadHelper(threadID);
  if(returnThread!=NULL){
    return returnThread;
  }
  //first search queue1
  threadQ = multiQ->queue1;
  returnThread = findThreadHelper(threadID);
  if(returnThread!=NULL){
    return returnThread;
  }
  //first search queue2
  threadQ = multiQ->queue2;
  returnThread = findThreadHelper(threadID);
  if(returnThread!=NULL){
    return returnThread;
  }
  //first search queue3
  threadQ = multiQ->queue3;
  returnThread = findThreadHelper(threadID);
  if(returnThread!=NULL){
    return returnThread;
  }
  //printf("Thread not found in multi-level Queue\n");
  return NULL;
}

tcb* findThread(int threadID){
  if(SCHED==MLFQ_SCHEDULER){
    return searchMLFQ(threadID);
  }
  return findThreadHelper(threadID);
}

/*Search for a thread by its threadID*/
tcb* findThreadHelper(int threadID){

  if(SCHED == FIFO_SCHEDULER || SCHED == STCF_SCHEDULER || SCHED == MLFQ_SCHEDULER){
    //pthread_mutex_lock(&qLock);
    if(threadQ == NULL)
    {
      //printf("Queue is Null\n");
      return NULL;
    }
    //Linear search through Queue for threadID
    queueNode* head=threadQ->head;
    //printf("about to search list for thread %d\n",threadID);
    if(head==NULL){
      //printf("head was NULL while searching for thread # (%d)\n", threadID);
      return NULL;
    }
    if((int)(head->thread_tcb->threadId)==(int)(threadID)){
      //printf("found as head\n");
      return head->thread_tcb;
    }
    while(head!=NULL && (int)(head->thread_tcb->threadId)!=(int)(threadID) ){
      //printf("ID: %d\n",head->thread_tcb->threadId);
      if((int)(head->thread_tcb->threadId)==(int)(threadID)){
        //printf("did we actually find the thread: %d??\n",head->thread_tcb->threadId);
        return head->thread_tcb;
      }
      head=head->next;
    }

    //Reached end of list
    if(head==NULL)
    {
      //printf("Thread (%d) not found.\n", threadID);
      return NULL;
    }
    return head->thread_tcb;
    //pthread_mutex_unlock(&qLock);
  }

}

void start_timer(int timeQ){// starts a timer to fire every timeQ milliseconds
  struct itimerval it_val;
  it_val.it_value.tv_sec =  timeQ/1000;
  it_val.it_value.tv_usec =  (TIME_QUANTUM*1000) % 1000000;
  it_val.it_interval = it_val.it_value;

  if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
    perror("error calling setitimer()");
  }
}

queueNode* getNextToRun(){ // returns top of queue according to current scheduling paradigm
  //pthread_mutex_lock(&qLock);
  if(SCHED == MLFQ_SCHEDULER){

    if(multiQ==NULL){ // multiQ is not initialized
      return NULL;
    }
    queueNode* indexer=multiQ->queue0->head;
    while(indexer!=NULL){
      if(indexer->thread_tcb->thread_status==BLOCKED){
        //Check if it needs to be unblocked
        if(indexer->thread_tcb->blocked_from->isLocked==0){
          indexer->thread_tcb->thread_status=READY;
          indexer->thread_tcb->blocked_from->isLocked=1;

          return indexer;
        }
        else{
          indexer=indexer->next;
        }
      }
      else{
        return indexer;
      }
    }
    //printf("Not found in queue 0\n");
    indexer=multiQ->queue1->head;
    while(indexer!=NULL){
      if(indexer->thread_tcb->thread_status==BLOCKED){
        //Check if it needs to be unblocked
        if(indexer->thread_tcb->blocked_from->isLocked==0){
          indexer->thread_tcb->thread_status=READY;
          indexer->thread_tcb->blocked_from->isLocked=1;
          return indexer;
        }
        //still blocked, go to next thread
        else{
          indexer=indexer->next;
        }
      }
      else{
        return indexer;
      }
    }


    //printf("Not found in queue 1\n");
    indexer=multiQ->queue2->head;
    while(indexer!=NULL){
      if(indexer->thread_tcb->thread_status==BLOCKED){
        //Check if it needs to be unblocked
        if(indexer->thread_tcb->blocked_from->isLocked==0){
          indexer->thread_tcb->thread_status=READY;
          indexer->thread_tcb->blocked_from->isLocked=1;
          return indexer;
        }
        //still blocked, go to next thread
        else{
          indexer=indexer->next;
        }
      }
      else{
        return indexer;
      }
    }

    //printf("Not found in queue 2\n");
    indexer=multiQ->queue3->head;
    while(indexer!=NULL){
      if(indexer->thread_tcb->thread_status==BLOCKED){
        //Check if it needs to be unblocked
        if(indexer->thread_tcb->blocked_from->isLocked==0){
          indexer->thread_tcb->thread_status=READY;
          indexer->thread_tcb->blocked_from->isLocked=1;
          return indexer;
        }
        //still blocked, go to next thread
        else{
          indexer=indexer->next;
        }
      }
      else{
        return indexer;
      }
    }
  //printf("Nothing left to run\n");
  return NULL;
  }
  else if(SCHED == FIFO_SCHEDULER || SCHED == STCF_SCHEDULER){
    if(threadQ==NULL){

      return NULL;
    }
    queueNode* topOfQueue=threadQ->head;

    if(topOfQueue==NULL){
      return NULL;
    }
    else{
      queueNode* indexer=topOfQueue;
      while(indexer!=NULL){
        if(indexer->thread_tcb->thread_status==BLOCKED){
          //Check if it needs to be unblocked
          if(indexer->thread_tcb->blocked_from->isLocked==0){
            indexer->thread_tcb->thread_status=READY;
            //indexer->thread_tcb->blocked_from->isLocked=1;
            if(__sync_lock_test_and_set(&(indexer->thread_tcb->blocked_from->isLocked),1)==0){
                // printf("Successfully unblocked thread %d and relocked mutex\n",indexer->thread_tcb->threadId);
            }
            return indexer;
          }
          //still blocked, go to next thread
          else{
            indexer=indexer->next;
          }
        }
        else{
          return indexer;
        }
      }

      // printf("No threads to schedule\n");
      return NULL;
    }
  }
  // ifndef MLFQ
  // MLFQ top of queue
}
void removeFromQueue(queueNode *finishedThread){
  if(SCHED == MLFQ_SCHEDULER){
    removeFromMLFQ(finishedThread);
  }
  else{
    removeFromQueueHelper(finishedThread);
  }
}
queueNode* removeFromQueue_NoFree(queueNode *finishedThread){
  if(SCHED == MLFQ_SCHEDULER){
    removeFromMLFQ_NoFree(finishedThread);
  }
  else{
    removeFromQueueHelper_NoFree(finishedThread);
  }
  return finishedThread;
}

void removeFromMLFQ(queueNode *finishedThread){
  threadQ = multiQ->queue0;
  if(removeFromQueueHelper(finishedThread)==1){
    return;
  }
  threadQ = multiQ->queue1;
  if(removeFromQueueHelper(finishedThread)==1){
    return;
  }
  threadQ = multiQ->queue2;
  if(removeFromQueueHelper(finishedThread)==1){
    return;
  }
  threadQ = multiQ->queue3;
  if(removeFromQueueHelper(finishedThread)==1){
    return;
  }
}

void removeFromMLFQ_NoFree(queueNode *finishedThread){
  threadQ = multiQ->queue0;
  if(removeFromQueueHelper_NoFree(finishedThread)==1){
    return;
  }
  threadQ = multiQ->queue1;
  if(removeFromQueueHelper_NoFree(finishedThread)==1){
    return;
  }
  threadQ = multiQ->queue2;
  if(removeFromQueueHelper_NoFree(finishedThread)==1){
    return;
  }
  threadQ = multiQ->queue3;
  if(removeFromQueueHelper_NoFree(finishedThread)==1){
    return;
  }
}

int removeFromQueueHelper(queueNode *finishedThread){
  if(SCHED == FIFO_SCHEDULER || SCHED == STCF_SCHEDULER || SCHED == MLFQ_SCHEDULER){
    if(finishedThread==NULL){

      return 0;
    }
    if(threadQ==NULL || threadQ->head==NULL)
    {
      return 0;
    }
    if(threadQ->head->thread_tcb->threadId==finishedThread->thread_tcb->threadId){
      queueNode* tmp=threadQ->head;
      threadQ->head=threadQ->head->next;
      freeQueueNode(tmp);
      return 1;
    }
    queueNode* prev=threadQ->head;
    queueNode* current=threadQ->head->next;
    while(current!=NULL){
      if(current->thread_tcb->threadId==finishedThread->thread_tcb->threadId){
        //printf("removing thread: %d\n",current->thread_tcb->threadId);
        if(current==threadQ->tail){
          threadQ->tail=prev;
        }
        prev->next=current->next;
        freeQueueNode(current);
        //pthread_mutex_unlock(&qLock);
        return 1;
      }
      prev=prev->next;
      current=current->next;
    }
    
    return -1;
  }

}
int removeFromQueueHelper_NoFree(queueNode *finishedThread){
  if(SCHED == FIFO_SCHEDULER || SCHED == STCF_SCHEDULER || SCHED == MLFQ_SCHEDULER){
    if(finishedThread==NULL){
      //printf("Cannot remove NULL thread\n");
      //pthread_mutex_unlock(&qLock);

      return 0;
    }
    if(threadQ==NULL || threadQ->head==NULL)
    {
      return 0;
    }
    if(threadQ->head->thread_tcb->threadId==finishedThread->thread_tcb->threadId){
      queueNode* tmp=threadQ->head;
      threadQ->head=threadQ->head->next;
  
      return 1;
    }
    queueNode* prev=threadQ->head;
    queueNode* current=threadQ->head->next;
    while(current!=NULL){
      if(current->thread_tcb->threadId==finishedThread->thread_tcb->threadId){
        if(current==threadQ->tail){
          // printf("removing old tail: %d\n",current->thread_tcb->threadId);
          threadQ->tail=prev;
        }
        prev->next=current->next;
        return 1;
      }
      prev=prev->next;
      current=current->next;
    }
    return -1;
  }

}
void updateThreadPosition(queueNode* finishedThread){
  if(SCHED == MLFQ_SCHEDULER){
    if(multiQ == NULL){
      printf("Our sanity check failed in MLFQ update thread position.\n");
      exit(1);
    }
    removeFromMLFQ_NoFree(finishedThread);
    //if priority changed
    if(finishedThread->thread_tcb->priority==0){
      if(multiQ->queue0->head==NULL){
        multiQ->queue0->head=finishedThread;
      }
      if(multiQ->queue0->tail!=NULL){
        multiQ->queue0->tail->next=finishedThread;
      }
      multiQ->queue0->tail=finishedThread;
      finishedThread->next=NULL;
    }
    else if(finishedThread->thread_tcb->priority==1){
      if(multiQ->queue1->head==NULL){
        multiQ->queue1->head=finishedThread;
      }
      if(multiQ->queue1->tail!=NULL){
        multiQ->queue1->tail->next=finishedThread;
      }
      multiQ->queue1->tail=finishedThread;
      finishedThread->next=NULL;
    }
    else if(finishedThread->thread_tcb->priority==2){
      if(multiQ->queue2->head==NULL){
        multiQ->queue2->head=finishedThread;
      }
      if(multiQ->queue2->tail!=NULL){
        multiQ->queue2->tail->next=finishedThread;
      }
      multiQ->queue2->tail=finishedThread;
      finishedThread->next=NULL;
    }
    else if(finishedThread->thread_tcb->priority==3){
      if(multiQ->queue3->head==NULL){
        multiQ->queue3->head=finishedThread;
      }
      if(multiQ->queue3->tail!=NULL){
        multiQ->queue3->tail->next=finishedThread;
      }
      multiQ->queue3->tail=finishedThread;
      finishedThread->next=NULL;
    }
    else{
      printf("Priority of queue is worse than 4: %d, exiting\n",finishedThread->thread_tcb->priority);
      exit(1);
    }
    return;
  }
  else if(SCHED == STCF_SCHEDULER){
    if(threadQ == NULL || threadQ->head == NULL){
      printf("Our sanity check failed in STCF update thread position.\n");
      exit(1);
    }
    //remove from top of queue
    removeFromQueue_NoFree(finishedThread);
    //No other elements, or if finished thread is still shortest to completion, run it again
    if(threadQ->head==NULL || threadQ->head->thread_tcb->time_ran>=finishedThread->thread_tcb->time_ran){
      finishedThread->next = threadQ->head;
      threadQ->head = finishedThread;
      //printQ(threadQ);
      return;
    }
    else{

      queueNode *prev = threadQ->head;
      queueNode *current = threadQ->head->next;

      while(prev!=NULL){
        if(current==NULL || current->thread_tcb->time_ran>finishedThread->thread_tcb->time_ran){
          prev->next = finishedThread;
          finishedThread->next = current;
          //printQ(threadQ);
          return;
        }
        prev=prev->next;
        current=current->next;
      }
    }
  }
  else if(SCHED == FIFO_SCHEDULER){
    threadQ->tail->next=finishedThread;
    threadQ->tail=finishedThread;
    threadQ->head=threadQ->head->next;
    finishedThread->next=NULL;
    return;
  }
}


queueNode* getRunningThread(){
  return runningThread;
}
mutexNode *findMutex(int mutexId){// searches and returns mutex that has a matching mutexId
  ignoreSignal=1;
  mutexNode *mutexPtr = mutexList;
  while(mutexPtr!=NULL){
    if(mutexPtr->mutex->mutexId==mutexId){
      ignoreSignal=0;
      return mutexPtr;
    }
    mutexPtr = mutexPtr->next;
  }
  ignoreSignal=0;
  return NULL;
}
void freeQueueNode(queueNode* node){
  tcb* tcbToFree=node->thread_tcb;
  freeTcb(tcbToFree);
  free(node);
  //printf("Freed queue node too\n");
  return;
}
void freeTcb(tcb* tcb){
  //printf("Freed allocated stack for thread %d\n",tcb->threadId);
  free(tcb->context.uc_stack.ss_sp);
  free(tcb->return_context.uc_stack.ss_sp);
  ignoreSignal=1;
  //free(tcb);
  return;
}
