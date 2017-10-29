#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

//Store the size of the Linked_List
int size;
//The Linked_List Struct that we will ise for page eviction
struct Linked_List{
	int number_entry;
	// bitwise_referrence (which is 0 when not recently used and 1 when
	// it is recently used)
	int bitwise_referrence;
	// The next page in the list
	struct Linked_List *p_next;
};

//This will be used as the starting point (like a head) of
// Linked_List.
struct Linked_List *starting;

/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {
	struct Linked_List *i, *reference;
	// evicted_frames stores the latest entry of the frame evicted
	int evicted_frames;
	// the Linked_List i will be used for iterations
	i = starting;
	// the Linked_List reference will be used for reference
	reference = starting;

	//setting the pointer to the last element
	while(reference->p_next != NULL)
	reference = reference -> p_next;

	// now we have to iterate through the iterating linked_list "i" we made
	// until the btiwise_referrence is 0. The only values for bitwise_referrence
	// are 1 and 0
	while(i->bitwise_referrence){
		if (i->p_next!=NULL){
			//This is the first element (which is
			//not the last element because i->p_next exists)
			i->bitwise_referrence = 0;
			//set the starting head pointer to the next element
			starting = i->p_next;
			reference->p_next = i;
			i->p_next = NULL;
			reference = i;
			i = starting;
		}else{
			// This means there is only one element
			i->bitwise_referrence = 0;
			break;
		}
	}
	// since the loop is exited, it means that bitwise_referrence is now 0
	//decrement the size since a page has been evicted
	size--;
	// storing the evicted frame
	evicted_frames = i->number_entry;
	starting = i->p_next;

	return evicted_frames;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {

	if (size!=0){
		// If the size is not 0
		struct Linked_List *list;
		list = starting;
		//If p already exists in the list then set the
		//bitwise_referrence to 1 (means it is recently used)
		while(list!=NULL){
			if(list->number_entry == (p->frame >> PAGE_SHIFT)){
				list->bitwise_referrence = 1; return;
			}
			else{
				list = list->p_next;
			}
		}
		// if the page is not in the list, then first check if there is
		//enough space in the list to add a new page
		if(size == memsize){
			// This means that the list is full, so evict a page
			//so that we can add a new page.
			clock_evict();
		}
		//This chunk of code from 106-118 just adds a new page to
		//end of the linked list and increments the size
		struct Linked_List *list2 = starting,\
			*NewPage = malloc(sizeof(struct Linked_List));

		NewPage->number_entry = (p->frame >> PAGE_SHIFT);
		NewPage->bitwise_referrence = 0;
		NewPage->p_next = NULL;
		size++;

		while(list2->p_next!=NULL){
			list2 = list2 ->p_next;
		}
		list2->p_next = NewPage;

	}else{
		// If the size is 0 then add the page to the starting of the list.
		starting->number_entry = (p->frame >> PAGE_SHIFT);
		size++;
	}


	return;
}

/* Initialize any data structures needed for this replacement
 * algorithm.
 */
void clock_init() {
	if ((starting = malloc(sizeof(struct Linked_List)))==NULL){
		perror("Malloc problem in clock_init line number 136"); exit(1);
	}
	starting->p_next = NULL;
	size=0;

}
