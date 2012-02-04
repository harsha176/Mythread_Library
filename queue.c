#include "queue.h"
#include <stdlib.h>
#include <assert.h>

queue_t *initialize_queue(nfree ufree, cmp ucmp)
{
    // create a new queue
    queue_t *new_queue;
    new_queue = (queue_t *) malloc(sizeof(queue_t));
    assert(new_queue != NULL);

    // initialize
    new_queue->first = NULL;
    new_queue->last = NULL;
    new_queue->nfree = ufree;
    new_queue->size = 0;
    new_queue->cmp = ucmp;

    return new_queue;
}

queue_t *enqueue(queue_t * queue, void *data)
{
    // create a node
    node_t *new_node = (node_t *) malloc(sizeof(node_t));
    assert(new_node != NULL);

    // initialize
    new_node->data = data;
    new_node->next = NULL;


    /*
     * insert into queue
     */
    // check if it is first element
    if (queue->size == 0) {
	assert(queue->first == NULL && queue->last == NULL);
	queue->first = queue->last = new_node;
	queue->size++;
	assert(queue->size == 1);
    } else {
	queue->last->next = new_node;
	queue->last = queue->last->next;
	queue->size++;
	assert(queue->last->next == NULL);
    }

    return queue;
}

void *dequeue(queue_t ** pqueue)
{
    node_t *temp;
    temp = (*pqueue)->first;
    // queue is empty
    if (temp == NULL) {
	return NULL;
    }
    (*pqueue)->first = temp->next;
    (*pqueue)->size = (*pqueue)->size - 1;
    if ((*pqueue)->size == 0) {
	(*pqueue)->first = (*pqueue)->last = NULL;
    } else if ((*pqueue)->size == 1) {
	(*pqueue)->first = (*pqueue)->last;
    }

    return temp->data;
}

void delete_queue(queue_t * queue)
{
    assert(queue != NULL);
    node_t *curr = queue->first;

    while (curr != NULL) {
	node_t *temp = curr;
	curr = curr->next;
	queue->nfree(temp->data);
	free(temp);
    }

    free(queue);
}

void delete_elem(queue_t ** pqueue, void *data)
{
    if (!(*pqueue)->cmp(((*pqueue)->first)->data, data)) {
	dequeue(pqueue);
	return;
    }
    queue_t *queue = *pqueue;

    node_t *curr = queue->first->next;
    node_t *prev = queue->first;

    while (curr != NULL) {
	if (!queue->cmp(curr->data, data)) {
	    //queue->nfree(data);
	    //node_t* temp = curr;
	    prev->next = curr->next;
	    //free(temp); // do not free
	    if (prev->next == NULL) {
		queue->last = prev;
	    }
	    queue->size--;
	    return;
	}
	prev = curr;
	curr = curr->next;
    }
}
