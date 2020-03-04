	/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 * If you want to make helper functions, put them in helpers.c
 */
#include "icsmm.h"
#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

ics_free_header *freelist_head = NULL;
ics_free_header *freelist_next = NULL;

ics_footer *prologue = NULL;
ics_footer *epilogue = NULL;
int page_count = 0;

void *ics_malloc(size_t size) {


    if (size == 0) {
        errno = EINVAL; //Representing an invalid request
        return NULL;
    }

    if (size > 4096 * 4 - 16) {
        errno = ENOMEM;
        return NULL;
    }
    if(page_count > 4){
		return NULL;
    }
    if (freelist_head == NULL){
        freelist_head = create_page();
	}
    void *searching = returnHeader(size);
	
    while(searching == NULL){
		if(page_count > 4){
			return NULL;
		}
		searching = create_page();
		freelist_next = freelist_head;
		searching = returnHeader(size);
    }
    return searching + 8;
}

void *ics_realloc(void *ptr, size_t size){
	ics_free_header *Oldptr = (ics_free_header*)(ptr - 8);
	
	if(Valid(Oldptr) == 0){
		errno = EINVAL;
		return NULL;
	}
	if(size == 0){
		ics_free(ptr);
		return NULL;
	}
	ics_free_header* Newptr = ics_malloc(size);
	if(Newptr == NULL){
		return NULL;
	}	
	
	mem_copy(ptr, Newptr);
	ics_free(ptr);
	return (void*)Newptr;
}
int ics_free(void *ptr){
	ics_free_header *Found = (ics_free_header*)(ptr - 8);

	if(Valid(Found) == 0){
		errno = EINVAL;
		return -1;
	}
	free_helper(Found);

	return 0;
}
