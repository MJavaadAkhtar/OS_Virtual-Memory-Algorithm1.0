#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

/* 	Linked List structure that stores all the page frames as they come in
	The LRU page will be stored at the tail and the MRU page will be stored at the head
*/
struct Linked_List{
	unsigned int frame_num;
	struct Linked_List *p_next;
};

struct Linked_List *linkedListStarting;

int size; // keeps track of the number of pages


/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
	struct Linked_List *head, *prev;
	head = linkedListStarting; prev = NULL;

	// Keep going until we reach the end of linked list
	while (head->p_next != NULL){
		prev = head;
	    head = head->p_next;
	}

	// remove the tail of the linked list (which stores the LRU page)
	prev->p_next = NULL;
	unsigned int evicted_frame = head->frame_num;
	free(head);
	size --;
	return evicted_frame;

}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {

	struct Linked_List *NewPage = malloc(sizeof(struct Linked_List));
	struct Linked_List *starting = NULL;


	// Assign values of new page, entered, will be the head eventually
	NewPage->frame_num = (p->frame >> PAGE_SHIFT);
	NewPage->p_next = NULL;



	if (size == 0){	// first frame, make it the head of the linked list
		linkedListStarting->frame_num = (p->frame >> PAGE_SHIFT);

	}else{
		starting = linkedListStarting;

		// if the p's frame is the first element in the linked list, then do nothing
		if (starting->frame_num == (p->frame >> PAGE_SHIFT))
		return;

		//Now check if page already exist in linked list
		while(starting->p_next!=NULL){

			if (starting->p_next->frame_num!=(p->frame >> PAGE_SHIFT))
				starting = starting->p_next;

			else{	// if page already in the linked list, then remove from middle and move to head
				struct Linked_List *list1, *list2;
				list1 = starting;
				list2 = starting->p_next;
				list1->p_next = list2->p_next;
				list2->p_next = linkedListStarting;
				linkedListStarting = list2; return;
			}
		}

		if (size == memsize){ // No space left, so evict a page to make room for new one
			lru_evict();
			NewPage->p_next = linkedListStarting;
			linkedListStarting = NewPage;

		}

		else { 	// new page is added to head of linked list
			
			NewPage->p_next = linkedListStarting;
			linkedListStarting = NewPage;

		}

	}
	size++;
	return;
}


/* Initialize any data structures needed for this
 * replacement algorithm
 */
void lru_init() {
	linkedListStarting = malloc(sizeof(struct Linked_List));
	linkedListStarting->p_next = NULL;
	linkedListStarting->frame_num = -1; // need to initialized frame_num
	size = 0;
}
