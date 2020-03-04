#include "helpers.h"
#include <stdlib.h>
#include <stdio.h>


/* Helper function definitions go here */

size_t cal_user_size(size_t size){
	size_t block = 16 + size;
	size_t helper = 0;
	if(block%16 !=0){
		helper = block/16 + 1;
	}else{
		return block;
	}
	return helper*16;
}

void* create_prologue(void * new_page){
    return new_page;
}

void delete_block(ics_free_header* BlockDelete){
	if(freelist_head->prev == NULL && freelist_head->next == NULL){//only one 
		freelist_head = NULL;
	}else if(BlockDelete->prev == NULL && BlockDelete->next != NULL){ //head but stuff in the back
		freelist_head = freelist_head->next;
		freelist_head->prev = NULL;
	}else if(BlockDelete->prev != NULL && BlockDelete->next != NULL){
		BlockDelete->prev->next = BlockDelete->next;
		BlockDelete->next->prev = BlockDelete->prev;
	}else{//tail
		BlockDelete->prev->next = NULL;
	}
	if(freelist_next == BlockDelete){
		freelist_next = freelist_head;
	}
	BlockDelete->prev = NULL;
	BlockDelete->next = NULL;
}

void add_block(ics_free_header* addBlock){
	addBlock->prev = NULL;
	addBlock->next = NULL;
	if(freelist_head == NULL){
		freelist_head = addBlock;
		freelist_next = freelist_head;
	}else{
		addBlock->next = freelist_head;
		freelist_head->prev = addBlock;
		freelist_head = addBlock;
	}
}

void* create_page(){
    page_count ++;
    if(page_count > 4)
        return NULL;
    void *page_ptr = create_prologue(ics_inc_brk());

    if(page_count == 1){
		set_prologue(page_ptr);
		set_epilogue(page_ptr);
		freelist_head = page_ptr + 8;
		freelist_head->header.requested_size = 0;
		freelist_head->header.unused = 0xaaaaaaaa;
		freelist_head->header.block_size = 4096 - 16; //prologue, epilogue
		freelist_head->next = NULL;
		freelist_head->prev = NULL;
		ics_footer *free_fter = (void*) epilogue - 8;

		make_ft(freelist_head,free_fter);
		freelist_next = freelist_head;
		return (void*) freelist_head;
    }
	
	set_epilogue(page_ptr);

	ics_free_header *new_page_header = page_ptr - 8;
	new_page_header->header.block_size = 4096;
	new_page_header->header.unused = 0xaaaaaaaa;
	new_page_header->header.requested_size = 0;
	new_page_header->next = NULL;
	new_page_header->prev = NULL;

	ics_footer *new_page_footer = (void*)new_page_header + new_page_header->header.block_size - 8;
	make_ft(new_page_header, new_page_footer);

	ics_footer *previous_ft = (void*) new_page_header - 8;
	
	if((previous_ft->block_size & 1) == 0){
		ics_free_header *previous_header = (void *) previous_ft - previous_ft->block_size + 8;
		ics_free_header* merge_block = merge(previous_header, new_page_header);
		delete_block(previous_header);
		add_block(merge_block);
		return (void*) new_page_header;
	}
	add_block(new_page_header);
    return (void*) new_page_header;
}

void set_prologue(void *head){
    prologue = head;
    prologue->unused = 0xaaaaaaaa;
    prologue->block_size = 1;
}

void set_epilogue(void *head){
    epilogue = head + 4096 - 8;
    epilogue->unused = 0xffffffffffff;
    epilogue->block_size = 1;
}

void make_ft(void *head, void *ft){
    size_t size = ((ics_free_header *)head)->header.block_size;
    ((ics_footer*)ft)->block_size = size;
    ((ics_footer*)ft)->unused = 0xffffffffffff;
}

void MallocHead(void *malloc, size_t size){
    ((ics_free_header *)malloc)->header.block_size += 1;
    ((ics_free_header *)malloc)->header.requested_size = size;
}

void allocateFreeBlockSplit(void *malloc,size_t size){
size_t user_block = cal_user_size(size);    
((ics_free_header *)malloc)->header.block_size = user_block + 1;
    ((ics_free_header *)malloc)->header.requested_size = size;
}

void notSplit(void *usedBlockHead, void *usedBlockFooter){
	uint64_t original_block = ((ics_free_header *)usedBlockHead)->header.block_size;
    MallocHead(usedBlockHead, original_block);
    make_ft(usedBlockHead, usedBlockFooter);
	delete_block(usedBlockHead);
}

//return 0 if head
//return 1 if middle
//retrun 2 if tail
int fit_block_position(void *ptr){
	ics_free_header *malloc_ptr = ((ics_free_header*)ptr);
	if(malloc_ptr == freelist_head)
		return 0;
	return 1;
//	if(malloc_ptr->prev != NULL && malloc_ptr->next != NULL)
//		return 1;
//	return 2;
}

void *searchFreeList(size_t userBlock){
    ics_free_header *cur = freelist_next;
    if(cur == NULL) {
        cur = freelist_head;
		freelist_next = freelist_head;
    }

    while(freelist_next){
        if(freelist_next->header.block_size >= userBlock){
			void * malloc_ptr = freelist_next;
			freelist_next = freelist_next->next; // set freelist_next = the next of freelist_next;
			if(freelist_next == NULL)
				freelist_next = freelist_head;
            return malloc_ptr;
        }else{
            if(freelist_next->next == NULL){
                freelist_next = freelist_head;
            }else{
                freelist_next = freelist_next->next;
            }
            if(freelist_next == cur)
                break;
        }
    }
    return NULL;
}

void *returnHeader(size_t size){
  size_t user_block = cal_user_size(size);
  void *fit_block = searchFreeList(user_block);
  
  void *setBlock;
  if (fit_block != NULL) {
      int block_pos = fit_block_position(fit_block);
      if(block_pos == 0){
          setBlock = headCondition(size, fit_block);
	}
	else{
		setBlock = notheadCondition(size, fit_block);
	}
//     }else if(block_pos == 1){
//	  setBlock = middleCondition(size, fit_block);
//      }else{
//	  setBlock = tailCondition(size, fit_block);
//      }
  }else{
      return NULL;
  }
  return setBlock;
}

void* split(void *mallocHead, void *mallocFt, size_t size){
	uint64_t original_block = ((ics_free_header *)mallocHead)->header.block_size;

    size_t user_block = cal_user_size(size);
	
	((ics_free_header*) mallocHead)->header.block_size = user_block + 1;
	((ics_free_header*) mallocHead)->header.unused = 0xaaaaaaaa;
	((ics_free_header*) mallocHead)->header.requested_size = size;
	ics_footer* new_ft = mallocHead + user_block - 8;

	make_ft(mallocHead, new_ft);
	
	mallocFt = mallocHead + original_block - 8;

	ics_free_header *freeHead = user_block + mallocHead;
	freeHead->header.block_size = original_block - user_block;
	freeHead->header.unused = 0xaaaaaaaa;
	freeHead->header.requested_size = 0;
	freeHead->next = NULL;
	freeHead->prev = NULL;
	
	make_ft(freeHead,mallocFt);
	
	return freeHead;
}

void* headCondition(size_t size, void *fit_block){
	ics_free_header *malloc_ptr = ((ics_free_header*)fit_block);
    size_t user_block = cal_user_size(size);
    ics_footer *fit_block_ft = fit_block + ((ics_free_header *)fit_block)->header.block_size - 8;
    if(malloc_ptr->header.block_size - user_block < 32){ //taking whole block
        notSplit(malloc_ptr,fit_block_ft);
        return fit_block;
    }
	
	ics_free_header* free_head = split(fit_block, fit_block_ft, size);
	
	delete_block(fit_block);
	add_block(free_head);
	
    return fit_block;
}


void *notheadCondition(size_t size, void *fit_block){

        ics_free_header *malloc_ptr = ((ics_free_header*)fit_block);
	size_t user_block = cal_user_size(size);
	ics_footer *fit_block_ft = fit_block + ((ics_free_header *)fit_block)->header.block_size - 8;

        if(malloc_ptr->header.block_size - user_block < 32){ //taking whole block
            notSplit(malloc_ptr,fit_block_ft);
        return fit_block;
        }

        ics_free_header* free_head = split(fit_block, fit_block_ft, size);
        delete_block(fit_block);
        add_block(free_head);


        return fit_block;
}
/*
void *middleCondition(size_t size, void *fit_block){
//	((ics_free_header*) fit_block)->next->prev = ((ics_free_header*) fit_block)->prev;
//	((ics_free_header*) fit_block)->prev->next = ((ics_free_header*) fit_block)->next;
	
	ics_free_header *malloc_ptr = ((ics_free_header*)fit_block);
    size_t user_block = cal_user_size(size);
    ics_footer *fit_block_ft = fit_block + ((ics_free_header *)fit_block)->header.block_size - 8;
		
	if(malloc_ptr->header.block_size - user_block < 32){ //taking whole block
	    notSplit(malloc_ptr,fit_block_ft);
        return fit_block;
	}
	
	ics_free_header* free_head = split(fit_block, fit_block_ft, size);
	delete_block(fit_block);
	add_block(free_head);


	return fit_block;
}

void *tailCondition(size_t size, void *fit_block){
//	((ics_free_header*) fit_block)->prev->next = NULL;	
	
	ics_free_header *malloc_ptr = ((ics_free_header*)fit_block);
    size_t user_block = cal_user_size(size);
    ics_footer *fit_block_ft = fit_block + ((ics_free_header *)fit_block)->header.block_size - 8;

	if(malloc_ptr->header.block_size - user_block < 32){ //taking whole block
		notSplit(malloc_ptr,fit_block_ft);
        return fit_block;
	}
	ics_free_header* free_head = split(fit_block, fit_block_ft, size);
	delete_block(fit_block);
	add_block(free_head);
	
	return fit_block;
}
*/
void* merge(void * first_header, void *second_header){

    ics_free_header *first = (ics_free_header*) first_header;
    ics_free_header *second = (ics_free_header*)second_header;

    first->header.block_size = first->header.block_size + second->header.block_size;

    ics_footer *merge_ft = (ics_footer*)(second_header + second->header.block_size - 8);
	make_ft(first,  merge_ft);

    return first_header;
}

int Valid(ics_free_header *isValid){
	int flag1 = 0, flag2 = 0,flag3 = 0, flag4 = 0, flag5 = 0, flag6 = 0;
	
	if((void*) isValid > (void*)prologue && (void*)isValid < (void*)epilogue){
		flag1 = 1;
	}
	if(isValid->header.unused == 0xaaaaaaaa){
		flag2 = 1;
	}
	ics_footer* isValidFt = (ics_footer*)((void*) isValid + isValid->header.block_size -  8 - 1);
	if(isValidFt->unused == 0xffffffffffff){
		flag3 = 1;
	}
	if(isValid->header.block_size == isValidFt->block_size){
		flag4 = 1;
	}
	if(isValid->header.requested_size < isValid->header.block_size){
		flag5 = 1;
	}
	if((isValid->header.block_size & 1) != 0 && (isValidFt->block_size & 1) != 0){
		flag6 = 1;
	}
	return (flag1==1&&flag2==1&&flag3==1&&flag4==1&&flag5==1&&flag6==1)?1:0;
}

void free_helper(ics_free_header* meFree){
	meFree->header.block_size = meFree->header.block_size - 1;
	ics_footer* meFreeFT = (ics_footer*)((void*) meFree + meFree->header.block_size - 8);
	make_ft(meFree,meFreeFT);
	
	ics_footer * prev_ft = (ics_footer*)((void*)meFree - 8);
	ics_free_header *next_header = (void*)meFree + meFree->header.block_size;
	
	if((prev_ft->block_size & 1) == 0){
			ics_free_header *prev_header = (void*)meFree - prev_ft->block_size;
		if((next_header->header.block_size & 1) == 0){ //before null
			//before and after = NULL
			ics_free_header* middle_after = merge(meFree, next_header);
			ics_free_header* take_all = merge(prev_header, middle_after);
			delete_block(next_header);
			delete_block(prev_header);
			add_block(take_all);
			return;
		}
		//before = NULL, but not after
		ics_free_header* merge_header = merge(prev_header, meFree);
		delete_block(prev_header);
		add_block(merge_header);
	}else{
		if((next_header->header.block_size & 1) == 0){
			//after = null, but not before
			ics_free_header* merge_header = merge(meFree, next_header);
			delete_block(next_header);
			add_block(merge_header);
			return;
		}
		//before and after are not null
		add_block(meFree);
	}
}

void mem_copy(void* old, void* new){
	char *src = (char*)old;
	char *des = (char*)new;
	ics_free_header* old_header = (ics_free_header*)(old - 8);
	ics_free_header* new_header = (ics_free_header*)(new - 8);
	int i = 0;
	if(old_header->header.requested_size > new_header->header.requested_size){
		for(i = 0; i < new_header->header.requested_size; i++){
			des[i] = src[i];
		}
	}else{
		for(i = 0; i < old_header->header.requested_size; i++){
			des[i] = src[i];
		}
	}
}
