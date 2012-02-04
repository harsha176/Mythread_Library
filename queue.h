/*
 * This is a queue interface used by mythread library. 
 *
 * It supports following operations. 
 *       1. Initialize queue
 *       2. enque
 *       3. deque
 *       4. delete.
 * 
 */

#ifndef _QUEUE__
#define _QUEUE__

//free function prototype.
typedef void (*nfree) (void *);

typedef int (*cmp) (void *, void *);

typedef struct __node_t_ node_t;

// node defnition
struct __node_t_ {
    void *data;
    node_t *next;
};


// queue definition
struct __queue_t_ {
    node_t *first;
    node_t *last;
    nfree nfree;
    cmp cmp;
    int size;
};

typedef struct __queue_t_ queue_t;

//queue_t queue;

/* This function initializes the queue. 
 *
 * It creates new queue data structure and initializes it. 
 *
 *
 */
queue_t *initialize_queue(nfree, cmp);

/*
 * This function retrieves first element in the queue.
 */
void *dequeue(queue_t **);

/*
 * This function inserts node at the end of the list.
 */
queue_t *enqueue(queue_t *, void *);


/*
 * This function deletes queue from memory.
 */
void delete_queue(queue_t *);

/*
 * delete node containing that data from the queue
 */
void delete_elem(queue_t **, void *data);


#endif				// end of queue function.
