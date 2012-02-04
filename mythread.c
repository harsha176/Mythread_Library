/*
 * This file implements thread library interface defined in mythread.h
 */
#include "mythread.h"
#include <stdlib.h>
#include <assert.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sched.h>
#include <unistd.h>
#include <features.h>
#include <string.h>

#define MAX_STACK_SIZE 1024*32

tcb_t **thread_table = NULL;
int nr_threads = 0;

mythread_t *current_tid;
queue_t *ready_queue;

static bool_t is_initialized = FALSE_T;

bool_t any_deletable_thread = FALSE_T;
mythread_t *deletable_thread = NULL;

/*global key table*/
key_mgmt_table_t key_mgmt_table = NULL;
int nr_keys = 0;

/*Initializes thread library: it creates two threads*/
void initialize();

/*It returns next free thread id: it initialzies the thread and 0 if it fails*/
int get_next_thread_id();

/*creates and initializes futex to value 0*/
struct futex *create_initialize_futex();

/*this method creates a thread */
mythread_t __create_thread(func_wrapper_t * /*function wrapper */ );

					  /*function wrapper for launching thread*///TODO
int thread_base_func(void * /*arg */ );

/*This is the function executed by idle thread. It schedules new thread*/
void *schedule();

/*fetches idle thread futex*/
struct futex *get_idle_thread_futex();

/*compares thread ids*/
int cmp_tid(void *val1, void *val2);

/*frees the resources of any threads*/
void free_any_thread_resources();

/*selects first thread in ready queue*/
mythread_t *select_next_runnable_thread();

/*given thread is put into ready queue and changed to ready state*/
void make_ready(mythread_t * /*tid */ );

/*gives up the cpu to next thread*/
int __yield();

/*increase key value count for all keys in key management table*/
void increase_key_value_count();

mythread_t mythread_self(void)
{
    if (!is_initialized) {
	initialize();
	is_initialized = TRUE_T;
    }
    return *current_tid;
}


void initialize()
{
    // initialize ready queue
    ready_queue = initialize_queue(free, cmp_tid);

    // create new thread: without allocating stack
    mythread_t main_thread = __create_thread(NULL);
    if (main_thread < 0) {
	printf
	    ("Failed to initialize thread library: unable to record main thread");
	fflush(NULL);
	return;
    }
    // running inside main thread so set the current thread to main thread
    current_tid = thread_table[main_thread]->tid;

    // create idle thread
    func_wrapper_t *sched_func =
	(func_wrapper_t *) malloc(sizeof(func_wrapper_t));
    if (!sched_func) {
	perror
	    ("Failed to initialize thread library: Failed to allocate memory to schedule function wrapper");
	return;
    }

    sched_func->fn = schedule;
    sched_func->arg = NULL;
    mythread_t idle_thread = __create_thread(sched_func);
    if (idle_thread < 0) {
	printf
	    ("Failed to initialize thread library: unable to record main thread");
	fflush(NULL);
	return;
    }
    //main thread should yield control
    //mythread_yield();
}


int get_next_thread_id()
{
    tcb_t **temp_table = thread_table;
    thread_table =
	(tcb_t **) realloc(thread_table, ++nr_threads * sizeof(tcb_t *));

    if (thread_table == NULL) {
	thread_table = temp_table;
	nr_threads--;		// decrement the thread count
	perror("Failed to allocate memory for new thread in thread table");
	return 0;
    }

    return nr_threads - 1;	// return last index of the thread table
}

struct futex *create_initialize_futex()
{
    /*int shm_id = shmget(IPC_PRIVATE, sizeof (struct futex), IPC_CREAT | 0666);
       struct futex* ft = shmat(shm_id, NULL, 0); */// segfault error
    struct futex *ft = (struct futex *) malloc(sizeof(struct futex));
    assert(ft != NULL);
    futex_init(ft, 0);

    return ft;
}


mythread_t __create_thread(func_wrapper_t * fn_wrapper)
{
    mythread_t new_thread = get_next_thread_id();
    thread_table[new_thread] = (tcb_t *) malloc(sizeof(tcb_t));
    int *id = (int *) malloc(sizeof(int));
    if (id == NULL) {
	perror
	    ("Create thread failed:Failed to allocate memory for thread");
	return -1;
    }
    *id = new_thread;

    if (fn_wrapper) {
	fn_wrapper->id = id;
    }

    if (!thread_table[new_thread]) {
	perror
	    ("Failed to intialaize thread library: allocate memory for new thread tcb");
	//free(thread_table[new_thread]);
	return -1;
    }

    if (fn_wrapper == NULL) {
	thread_table[new_thread]->state = RUNNING_S;
    } else {
	thread_table[new_thread]->state = READY_S;
    }
    thread_table[new_thread]->tid = id;
    thread_table[new_thread]->context_ft = create_initialize_futex();
    thread_table[new_thread]->join_queue = initialize_queue(free, cmp_tid);

    char *stack_temp = (char *) malloc(sizeof(char) * MAX_STACK_SIZE);
    if (!stack_temp) {
	perror("Failed to allocate stack memory");
	return -1;
    }
    thread_table[new_thread]->stack = stack_temp;	// set stack pointer to top of the stack

    //clone if function_wrapper is passed
    if (fn_wrapper) {
	if (clone
	    (&thread_base_func, stack_temp + MAX_STACK_SIZE - 1,
	     CLONE_FILES | CLONE_FS | CLONE_SIGHAND | CLONE_VM,
	     (void *) fn_wrapper) < 0) {
	    perror("Failed to clone system call");
	    return -1;
	}
	ready_queue = enqueue(ready_queue, id);	// inserting in ready queue
    }

    return new_thread;
}



void *schedule()
{
    while (1) {
	assert(*current_tid == 1);
	//
	mythread_t *next_tid = select_next_runnable_thread();
	if (next_tid == NULL) {
	    // no threads are there in the systems
	    // clean up allocated memory and exit
	    //clean();
	    exit(0);
	}
	// enqueue idle thread
	ready_queue = enqueue(ready_queue, current_tid);

	current_tid = next_tid;	// update current tid to next runnable thread
	thread_table[*next_tid]->state = RUNNING_S;
	futex_up(thread_table[*next_tid]->context_ft);
	//sched_yield();
	futex_down(get_idle_thread_futex());

	//check for any deletable threads and free memory for that thread
	free_any_thread_resources();
    }
}

/* Selects next thread to be executed
 */
mythread_t *select_next_runnable_thread()
{
    return (mythread_t *) dequeue(&ready_queue);
}

void free_any_thread_resources()
{
    // check for any deletable threads
    if (any_deletable_thread == TRUE_T) {
	assert(deletable_thread);
	// free stack
	int tid = *deletable_thread;
	if (tid != 0) {
	    ;			//free(thread_table[tid]->stack);
	}
	// free futex
	//free(thread_table[tid]->context_ft);
	// free tid
	//free(thread_table[tid]->tid);
	// free thread_block  
	thread_table[tid] = NULL;
	// free(thread_table[tid]);
	any_deletable_thread = FALSE_T;
        deletable_thread = NULL;      
    }
}

struct futex *get_idle_thread_futex()
{
    assert(thread_table != NULL);
    assert(thread_table + 1 != NULL);

    return thread_table[1]->context_ft;	// assumption idle thread takes second position in thread_table
}



int thread_base_func(void *arg)
{
    func_wrapper_t *fn_w = (func_wrapper_t *) arg;
    futex_down(thread_table[*(fn_w->id)]->context_ft);
    current_tid = fn_w->id;	// update current id
    int *ret = fn_w->fn(fn_w->arg);
    mythread_exit(ret);
    return 0;
}


void mythread_exit(void *retval)
{
    assert(thread_table[*current_tid]);
    queue_t *join_queue = thread_table[*current_tid]->join_queue;
    // return value needs to be updated to joined threads;

    mythread_t *next_thread;
    while ((next_thread = (mythread_t *) dequeue(&join_queue))) {
	make_ready(next_thread);
    }

    // set key value for all keys to null
    int i = 0;
    for(; i < nr_keys; i++) {
      (key_mgmt_table[i]->values_table)[*current_tid]= NULL;
    }

    // mark thread to be delatable
    any_deletable_thread = TRUE_T;
    deletable_thread = current_tid;

    // dequeue current thread from ready queue
    // dequeue_elm(ready_queue, current_tid);  //not required

    // change current thread to idle thread: to be updated
    current_tid = thread_table[1]->tid;
    // remove current tid from ready queue
    delete_elem(&ready_queue, current_tid);
    futex_up(get_idle_thread_futex());
    //sched_yield(); 
    exit(0);
}

int cmp_tid(void *val1, void *val2)
{
    if (*((int *) val1) == *((int *) val2)) {
	return 0;
    } else if (*((int *) val1) > *((int *) val2)) {
	return 1;
    } else
	return -1;
}


void make_ready(mythread_t * tid)
{
    // change state to ready
    thread_table[*tid]->state = READY_S;
    // enqueue in ready list
    ready_queue = enqueue(ready_queue, tid);
}


int mythread_yield()
{
    //change current_tid state
    thread_table[*current_tid]->state = READY_S;
    //enqueue
    ready_queue = enqueue(ready_queue, current_tid);

    return __yield();
}


int mythread_join(mythread_t target_thread, void **status)
{

    // first check if the thread exists    
    if (thread_table[target_thread] != NULL) {
	//dequeue from ready queue
	// dequeue_elm(ready_queue, current_tid); not required current_tid will not be in ready queue
	//change state to blocked
	thread_table[*current_tid]->state = BLOCKED_S;

	// add this thread to given thread join queue
	thread_table[target_thread]->join_queue =
	    enqueue(thread_table[target_thread]->join_queue,
		    current_tid);


	// yield thread to idle thread
	__yield();
    }
    return 0;
}

/* 
 * Preconditions: move tid's to appropriate queues before calling this procedure.
 */
int __yield()
{
    mythread_t *prev_tid = current_tid;
    current_tid = thread_table[1]->tid;	// assumption about idle thread
    delete_elem(&ready_queue, current_tid);
    futex_up(get_idle_thread_futex());
    //sched_yield();
    futex_down(thread_table[*prev_tid]->context_ft);
    return 0;
}

int mythread_create(mythread_t * new_thread_id, mythread_attr_t * attr,
		    fn_t start_func, void *arg)
{

    if (!is_initialized) {
	initialize();
	is_initialized = TRUE_T;
    }

    func_wrapper_t *st_fn =
	(func_wrapper_t *) malloc(sizeof(func_wrapper_t));
    if (!st_fn) {
	perror("Failed to allocate memory to function wrapper");
	return -1;
    }
    st_fn->fn = start_func;
    st_fn->arg = arg;
    *new_thread_id = __create_thread(st_fn);
    // the actual thread should yield cpu
    //mythread_yield(); not required
    
    /*allocate value for each key in thread id*/
    increase_key_value_count();
    return *new_thread_id;
}


int mythread_key_create(mythread_key_t* key, void (*destructor) (void*)) {
    if (!is_initialized) {
	initialize();
	is_initialized = TRUE_T;
    }
    nr_keys++;
    // increase size key_mgmt_table to accomadate new key
    key_mgmt_table_t temp = key_mgmt_table;
    key_mgmt_table = (key_mgmt_table_t)realloc(key_mgmt_table, sizeof(kmb_t*)*nr_keys);

    if(!key_mgmt_table) {
       nr_keys--;
       key_mgmt_table = temp;
       perror("Failed to allocate memory in key management table");
       return -1;
    }          

    // allocate memory for new key managememt block
    key_mgmt_table[nr_keys-1] = (kmb_t*)malloc(sizeof(kmb_t));
   
    if(!key_mgmt_table[nr_keys-1]) {
       perror("Failed to allocate memory for key management block");
       return -1;
    }

    // initialzie key_management block
    key_mgmt_table[nr_keys-1]->kid = nr_keys-1;
    key_mgmt_table[nr_keys-1]->destructor = destructor;
   
    // allocate memory for key values
    key_mgmt_table[nr_keys-1]->values_table = (key_value_table_t)malloc(sizeof(key_value_t)*nr_threads);
    if(!key_mgmt_table[nr_keys-1]->values_table) {
       perror("Failed to allocate memory to values table");
       return -1;
    }

    // set all values to null
    memset(key_mgmt_table[nr_keys-1]->values_table, 0, nr_threads);

    *key = nr_keys - 1;  // update key
    return nr_keys - 1; 
}


int mythread_key_delete(mythread_key_t key) {
   
    if (!is_initialized) {
	initialize();
	is_initialized = TRUE_T;
    }
   // check if the key is valid
   if(key < 0 /*&& key >= nr_keys*/){
      return -1;
   }

   /* remove key management block and set the entry for that keys table to NULL*/
   if(!key_mgmt_table && !key_mgmt_table[key]) {
      return -1;
   }
  
   // free each value in values table
   int i = 0;
   for(; i < nr_threads; i++) {
       if((key_mgmt_table[key]->values_table)[i]) {
          key_mgmt_table[key]->destructor((key_mgmt_table[key]->values_table)[i]);
       }
   }

   // free key_mgmt_table block and set it to null
   free(key_mgmt_table[key]);
   key_mgmt_table[key] = NULL;
   
   nr_keys--;
   
   return 0;
}

void* mythread_getspecific(mythread_key_t key) {
    if (!is_initialized) {
	initialize();
	is_initialized = TRUE_T;
   }

   if(key < 0 /*&& key >= nr_keys*/){
      return NULL;
   }

   /* remove key management block and set the entry for that keys table to NULL*/
   if(!key_mgmt_table && !key_mgmt_table[key]) {
      return NULL;
   }

   // return key for that specific thread
   return (key_mgmt_table[key]->values_table)[*current_tid];
}


int mythread_setspecific(mythread_key_t key, const void* value) {
   if (!is_initialized) {
	initialize();
	is_initialized = TRUE_T;
   }
   
   if(key < 0 /*&& key >= nr_keys*/){
      return -1;
   }

   /* remove key management block and set the entry for that keys table to NULL*/
   if(!key_mgmt_table && !key_mgmt_table[key]) {
      return -1;
   }
   
   assert(key_mgmt_table[key]->values_table);
   //assign value 
   (key_mgmt_table[key]->values_table)[*current_tid] = (key_value_t*)value;
   return 0;
}


void increase_key_value_count() {
   // increase values size for each key
   int i;

   for(i = 0; i < nr_keys; i++) {
      key_value_table_t temp = key_mgmt_table[i]->values_table;
      key_mgmt_table[i]->values_table = (key_value_table_t)realloc(key_mgmt_table[i]->values_table, sizeof(key_value_t)*nr_threads);
      if(!(key_mgmt_table[i]->values_table)) {
         key_mgmt_table[i]->values_table = temp;
         perror("Failed to allocate memory for key values for new thread ");
         return ;
      }
      //set the value to NULL
      (key_mgmt_table[i]->values_table)[nr_threads - 1] = NULL;  // assumption that this is called only by mythread_create
   }
}
