#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;
char buf[10000000];



int vaddrslen;
addr_t *vaddrs;

extern char *tracefile;
FILE* trace1;
FILE* trace2;
int index_no=0;

struct Linked_List{
	unsigned int number;
	int nextIndex;
	struct Linked_List *p_next;
};
int counter;

// will be used as a head
struct Linked_List* starting;
// The size of the Linked List
int size;

/* Page to evict is chosen using the optimal (aka MIN) algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
	// Go through the list and compare next call value
	// and remove based on that
	struct Linked_List *itr, *prev;
	itr = starting; prev = NULL;
	//This is where the evicted frames will be stored, it is momentarily -1
	unsigned int evicted_frame = -1;

	if(itr->p_next != NULL){
		struct Linked_List *latest = NULL, *previousLast = NULL;
		// latest is the pointer to element that will be called last
		int lastIndex=-1;

		while (itr->p_next != NULL){
			// This means that index != -1
			if (itr->nextIndex < index_no){
				// access the index is less than the current index
				int counter1 = index_no+1;
				int current_element = itr->nextIndex;
				while(counter1<counter){
					if (vaddrs[current_element] == vaddrs[counter1]){
						itr->nextIndex = counter1;
						break;
					}counter1++;
				}
				// now it is updated, we have to check if it is not
				// -1 then we have to find the latest point to this one.
				if (itr->nextIndex !=-1){
					if (itr->nextIndex < lastIndex){
						prev = itr; itr = itr->p_next;
					}else{
						latest = itr;
						lastIndex = itr->nextIndex;
						previousLast = prev;
						prev = itr;
						itr = itr->p_next;
					}
				}else{
					// Since it is -1, we know have to remove it
					if (prev!=NULL){
						// If the element is not the first element, then
						// return the evicted frame and free itr
						evicted_frame = itr->number; size--;
						prev->p_next = itr->p_next; free(itr);
						return evicted_frame;
					}
					else{
						// If the element is the first element, then
						// return the evicted frame and free itr
						evicted_frame = itr->number; size--;
						starting = itr->p_next; free(itr);
						return evicted_frame;
					}
				}

			}else if(itr->nextIndex == -1){
				// if the index is -1, we gotta remove that.

				//frame not the first frame:
				if (prev!=NULL){
					// Store the frame, preserve the is, reduce the size by 1,
					// free itr and return the frame
					evicted_frame = itr->number; size--;
					prev->p_next = itr->p_next; free(itr);
					return evicted_frame;

				}else{
					//This means that the first element is
					// the oldest element
					evicted_frame = itr->number; size--;
					starting  = itr->p_next; free(itr);
					return evicted_frame;
				}
			}else{
				//now access the valid pte
				if (itr->nextIndex < lastIndex){
					// just move forward
					prev = itr; itr = itr -> p_next;
				}else{
					latest = itr;
					lastIndex = itr->nextIndex;
					previousLast = prev;
					prev = itr;
					itr  = itr->p_next;
				}
			}
		}
		// Since the program has reached here means that no frame has -1 as an
		// next index. so now we must remove the latest and set prev to next
		if(previousLast!= NULL)
		previousLast->p_next = latest->p_next;

		size--;
		evicted_frame = latest->number;
		free(latest);
		return evicted_frame;
	}else{
		// when memsize is 1 and 1st element is also 1,
		// plus no need to free anything since initialized it in init.
		size--;
		starting->number = -1;
	}
	return 0;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {
	if (size!=0){
		// increment the index number to the current index
		index_no++;
		// If the memory is full, since size can be equal but will never
		// exceed memory but just to be safe
		if (size>=memsize){
			struct Linked_List *itr = starting;
			//check if the element is already in the list otherwise
			// if it doesnt exist than make the pointer point to the last element.
			while(itr->p_next!=NULL){
				if (itr->number == (p->frame >> PAGE_SHIFT)){
					int counter2 = index_no+1;
					itr->nextIndex = -1;
					while(counter2<counter){
						if (vaddrs[index_no] == vaddrs[counter2]){
							itr->nextIndex = counter2; return;
						}
						counter2++;
					}
					return;
				}else{
					itr = itr->p_next;
				}
			}
			//Since the program came here, means the page wasnt found and the
			// size equals memory, we need to evict a page to make space for the
			// new page
			opt_evict();
			struct Linked_List *new_page;
			if ((new_page = malloc(sizeof(struct Linked_List)))==NULL){
				perror("mallocing problem opt_ref line 179"); exit(1);
			}
			new_page->number = (p->frame >> PAGE_SHIFT);
			new_page->p_next = NULL;
			// Making below 0 just to be safe
			new_page->nextIndex = -1;
			int counter1 = index_no+1;
			while(counter1<counter){
				if(vaddrs[index_no]==vaddrs[counter1]){
					new_page->nextIndex = counter1; break;
				}
				counter1++;
			}
			//increment the size since the page it going to be added to the
			// end of the list
			size++;
			struct Linked_List *listTemp = starting;
			while(listTemp->p_next!=NULL){
				listTemp = listTemp->p_next;
			}
			//now listTemp points to the last element after the evicition
			//happened
			listTemp->p_next = new_page;

		}else{
			// pointing a new Linked_List to starting
			struct Linked_List *itr2 = starting;
			while(itr2->p_next!=NULL){
				if (itr2->number == (p->frame >> PAGE_SHIFT)){
					// the page is already in the linked list
					int counts = index_no+1;
					itr2->nextIndex = -1;
					while(counts<counter){
						if(vaddrs[index_no] == vaddrs[counts]){
							itr2->nextIndex=counts;
							return;
						}
						counts++;
					}
					return;
				}else{
					// set the pointer to the last element
					itr2 = itr2->p_next;
				}
			}
			// the code reached here means the page is not in the
			// list which is why the program reached here
			struct Linked_List *new_page;
			if ((new_page = malloc(sizeof(struct Linked_List)))==NULL){
				perror("mallocing problem opt_ref line 216"); exit(1);
			}
			//making a new linked list for the new page in inorder to add it
			// to the original list
			new_page->number = (p->frame >> PAGE_SHIFT);
			new_page->p_next = NULL;
			new_page->nextIndex = -1;
			int counter1 = index_no+1; //change
			while(counter1<counter){
				if(vaddrs[index_no]==vaddrs[counter1]){
					itr2->nextIndex = counter1; break;
				}
				counter1++;
			}
			size++;
			itr2->p_next = new_page;
		}
	}else{
		// The code is here because the size of the linked list is 0
		// means there is no element in there
		index_no=0;
		starting->number = (p->frame >> PAGE_SHIFT); size++;
		int counts = index_no+1;
		starting->nextIndex = -1;
		while (counts<counter) {
			if (vaddrs[index_no] == vaddrs[counts]){
				starting->nextIndex = counts; break;
			}
			counts++;
		}
	}
	return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {
	if ((starting = malloc(sizeof(struct Linked_List)))==NULL){
		perror("mallocing problem opt_init line 249"); exit(1);
	}
	starting->number = -1;
	starting->nextIndex = 0;
	starting->p_next = NULL;
	size=0; counter = 0;

	//opeining the treace files for readings
	if(tracefile!=NULL){
		if ((trace1 = fopen(tracefile, "r"))==NULL){
			perror("Tracefile problem"); exit(1);
		}
		if ((trace2 = fopen(tracefile, "r"))==NULL){
			perror("Tracefile problem"); exit(1);
		}
	}else{
		perror("tracefile = NULL"); exit(1);
	}

	//getting the unmber of elements from the tracefiles for the belady's
	// algorithm
	char char1;
	addr_t vaddr;
	while(fgets(buf, 256, trace1)!=NULL){
		if (buf[0] != '='){
			sscanf(buf, "%c %lx",&char1, &vaddr); counter++;
		}
	}

	//Setting the vaddrlen to be a the total number of vaddresses
	vaddrslen=counter;
    if ((vaddrs=malloc(counter * sizeof(addr_t)))<0){
		perror("mallocing problem opt_init line 277"); exit(1);
	}
    int count2=0;

    //this loop read in every addresses and stores it.
    while(fgets(buf, 256, trace2) != NULL) {
        if(buf[0] != '=') {
            if ((sscanf(buf, "%c %lx", &char1, &vaddr))<0){
				perror("sscanf problem opt_init line 285"); exit(1);
			}
            vaddrs[count2]=vaddr;
            count2++;

        }

    }



}
