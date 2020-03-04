#ifndef HELPERS_H
#define HELPERS_H

#include "icsmm.h"
/* Helper function declarations go here */

extern ics_footer *prologue;
extern ics_footer *epilogue;
extern int page_count;

size_t cal_user_size(size_t size);

void* create_prologue(void * new_page);

void delete_block(ics_free_header* BlockDelete);

void add_block(ics_free_header* addBlock);

void* create_page();

void set_prologue(void *head_page);

void set_epilogue(void *head_page);

void make_ft(void *head, void *ft);

void *searchFreeList(size_t userBlock);                                 

void MallocHead(void *malloc, size_t size);

void allocateFreeBlockSplit(void *malloc,size_t size);

void setFreeHead(ics_free_header* free_head);

void notSplit(void *usedBlockHead, void *usedBlockFooter);

int fit_block_position();

void *returnHeader(size_t size);

void* merge(void * first_block, void *second_block);

void* split(void *mallocHead, void *mallocFt, size_t size);

void* headCondition(size_t size, void *fit_block);
/*
void *middleCondition(size_t size, void *fit_block);

void *tailCondition(size_t size, void *fit_block);
*/
void RedirectionFreeListNext(void *Block);

int Valid(ics_free_header *isValid);

void free_helper(ics_free_header* meFree);

void mem_copy(void* old, void* new);
void *notheadCondition(size_t size, void *fit_block);
#endif
