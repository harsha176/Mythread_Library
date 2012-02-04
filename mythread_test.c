#include<stdio.h>
#include "mythread.h"
#include <stdlib.h>
#include<unistd.h>
#include <assert.h>
#include <string.h>

#define MAX_NAME_LENGTH 256

mythread_key_t name_key, id_key;

void *hello(void *t)
{
    ///int *reps = (int*)t;
    int i = 0;

    // initialize thread specific data for each thread
    int* id = (int*)malloc(sizeof(int));
    assert(id);
    *id = mythread_self();
    mythread_setspecific(id_key, id);

    //set name
    char* name = (char*)malloc(sizeof(char)*MAX_NAME_LENGTH);
    assert(name);
    memset(name, '\0', MAX_NAME_LENGTH);
    sprintf(name, "worker-%d", mythread_self()); 
    mythread_setspecific(name_key,name);

    // user specific data 
    for (; i < mythread_self(); i++) {
	printf("Thread-%d:Hello thread:%d\n", mythread_self(), i);
	sleep(1);
	mythread_yield();
    }
    printf("Thread specific data for %d thread is %d and worker name is %s\n", mythread_self(), *((int *)mythread_getspecific(id_key)), (char*)mythread_getspecific(name_key));
    return NULL;
}

int main()
{
    int i = 0;
    mythread_t tid[10];
    void *status;

    // initialize key
    mythread_key_create(&name_key, free);
    mythread_key_create(&id_key, free);

    for (i = 0; i < 10; i++) {
	int *temp = (int *) (malloc) (sizeof(int));
	*temp = i;
	mythread_create(&tid[i], NULL, hello, (void *) temp);
	mythread_yield();
    }

    printf("main thread is joining with child threads\n");
    for (i = 0; i < 10; i++) {
	mythread_join(tid[i], &status);
	printf("joined on thread:%d\n", tid[i]);
    }
    
    mythread_key_delete(id_key);
    mythread_key_delete(name_key);
    printf("main thread is exiting\n");
    fflush(NULL);
    mythread_exit(NULL);
    return 0;
}
