#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

int cmp_int(void *data1, void *data2)
{
    int *val1 = (int *) data1;
    int *val2 = (int *) data2;

    if (*val1 == *val2)
	return 0;
    else if (*val1 > *val2)
	return 1;
    else
	return -1;
}

int main()
{
    queue_t *test_queue = initialize_queue(free, cmp_int);

    while (1) {
	while (1) {
	    int *temp = (int *) malloc(sizeof(int));
	    printf("Enter a number to enqueue(0 to exit):");
	    scanf("%d", temp);
	    if (*temp == 0) {
		break;
	    }
	    test_queue = enqueue(test_queue, temp);
	}
	int *p;

	/*printf("Select an element to delete:"); 
	   scanf("%d", p);
	   delete_elem(&test_queue, p);
	   free(p); */

	printf("dequeueing elements\n");
	while (p = (int *) dequeue(&test_queue)) {
	    printf("%2d\n", *p);
	    free(p);
	}
    }
    return 0;
}
