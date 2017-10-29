#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int size;
int *page_array;

/* Page to evict is chosen using the fifo algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int fifo_evict() {

	int evicted_page = page_array[size%memsize];
	page_array[size%memsize] = -1; //we don't need this but just to be safe
	return evicted_page;

}

/* This function is called on each access to a page to update any information
 * needed by the fifo algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void fifo_ref(pgtbl_entry_t *p) {

	if (size >= memsize){ // page table reference is full, time to evict
		 fifo_evict();
	}

	page_array[size%memsize] = (p->frame >> PAGE_SHIFT); // adding new page
	size ++;

	return;
}

/* Initialize any data structures needed for this
 * replacement algorithm
 */
void fifo_init() {
	size = 0;
	page_array = malloc(sizeof(int)*memsize);

	for (int i = 0; i < memsize; i++){
		page_array[i] = -1;
	}
}
