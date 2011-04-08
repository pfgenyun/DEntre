/************************************************************
 * Copyright (c) 2010-present Peng Fei.  All rights reserved.
 ************************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistribution and use in source and binary forms must authorized by
 * Peng Fei.
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 */

#include "globals.h"
#include "utils.h"
#include "heap.h"
#include "options.h"

//#define BLOCK_TYPES (sizeof(BLOCK_SIZES)/sizeof(uint))
#define BLOCK_TYPES 12		/* need to be filled up */

typedef byte *heap_pc;
typedef byte *vm_addr_t;

/* thread-local heap structure
 * this struct is kept at top of unit itself, not in separate allocation
 */
typedef struct _heap_unit_t
{
	heap_pc start_pc;
	heap_pc end_pc;
	heap_pc cur_pc;
	heap_pc reserved_end_pc;

	bool in_vmarea_list;

#ifdef DEBUG
	int id;
#endif

	struct _heap_unit_t *next_local;
	struct _heap_unit_t *next_global;
	struct _heap_unit_t *prev_global;

}heap_unit_t;


#ifdef HEAP_ACCOUNTING
typedef struct _heap_acct_t {
    size_t alloc_reuse[ACCT_LAST];
    size_t alloc_new[ACCT_LAST];
    size_t cur_usage[ACCT_LAST];
    size_t max_usage[ACCT_LAST];
    size_t max_single[ACCT_LAST];
    uint num_alloc[ACCT_LAST];
} heap_acct_t;
#endif

typedef struct _thread_units_t
{
	heap_unit_t *top_unit;
	heap_unit_t *cur_unit;

	heap_pc free_list[BLOCK_TYPES];

#ifdef DEBUG
	int num_units;
#endif

	dcontext_t *dcontext;	/* back pointer to owner */
	bool writable;

#ifdef HEAP_ACCOUNTING
	heap_acct_t acct;
#endif

}thread_units_t;

typedef struct _thread_heap_t
{
	thread_units_t *local_heap;
	thread_units_t *nonpersistent_heap;
}thread_heap_t;

/* global, unique thread-shared structure */
typedef struct _heap_t
{
	heap_unit_t *units;
	heap_unit_t *dead;

	uint num_dead;
}heap_t;

enum
{
	VMM_BLOCK_SIZE = 16 * 1024		/* 16K */
};

enum
{
	MAX_VMM_HEAP_UNIT_SIZE = 512 * 1024 * 1024,		/* 512M */
	MIN_VMM_HEAP_UNIT_SIZE = VMM_BLOCK_SIZE
};

typedef struct _vm_heap_t
{
	vm_addr_t start_addr;
	vm_addr_t end_addr;		/* [start_addr,end_addr)*/
	vm_addr_t alloc_addr;
	size_t alloc_size;

	uint num_blocks;		/* total number of blocks in virtual allocation */
	mutex_t lock;
	uint num_free_blocks;

	bitmap_element_t block[BITMAP_INDEX(MAX_VMM_HEAP_UNIT_SIZE/VMM_BLOCK_SIZE)];

}vm_heap_t;

/* We keep our heap management structs on the heap for selfprot (case 8074).
 * Note that we do have static structs for bootstrapping and we later move
 * the data here.
 */
typedef struct _heap_managemen_t
{
    /* high-level management */
    /* we reserve only a single vm_heap_t for guaranteed allocation, 
     * we fall back to OS when run out of reservation space */
    vm_heap_t vmheap;
	heap_t heap;

	/* thread-shared heaps */
	thread_units_t global_units;
	thread_units_t global_nonpersistent_units;
	bool global_heap_writable;
	thread_units_t global_unprotected_units;
}heap_management_t;

static heap_management_t temp_heaping;
static heap_management_t *heapmgt = &temp_heaping;


static void 
vmm_heap_unit_init(vm_heap_t *vmh, size_t size)
{
	/* need to be filled up */
}

void 
vmm_heap_init(void)
{
#ifdef N64
	/* need to be filled up */
#endif

	if(DENTRE_OPTION(vm_reserve))
	{
		vmm_heap_unit_init(&heapmgt->vmheap, DENTRE_OPTION(vm_reserve));
	}
}
