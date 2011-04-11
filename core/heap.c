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

	bitmap_element_t blocks[BITMAP_INDEX(MAX_VMM_HEAP_UNIT_SIZE/VMM_BLOCK_SIZE)];

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


typedef enum {
    /* I - Init, Interop - first allocation failed
     *    check for incompatible kernel drivers 
     */
    OOM_INIT    = 0x1,
    /* R - Reserve - out of virtual reservation *
     *    increase -vm_size to reserve more memory
     */
    OOM_RESERVE = 0x2,
    /* C - Commit - systemwide page file limit, or current process job limit hit
     * Increase pagefile size, check for memory leak in any application.
     *
     * FIXME: possible automatic actions
     *    if systemwide failure we may want to wait if transient 
     *    FIXME: if in a job latter we want to detect and just die 
     *    (though after freeing as much memory as we can)
     */
    OOM_COMMIT  = 0x4,
    /* E - Extending Commit - same reasons as Commit 
     *    as a possible workaround increasing -heap_commit_increment
     *    may make expose us to commit-ing less frequently,
     *    On the other hand committing smaller chunks has a higher 
     *    chance of getting through when there is very little memory.
     *
     *    FIXME: not much more informative than OOM_COMMIT
     */
    OOM_EXTEND  = 0x8,
} oom_source_t;

static void
report_low_on_memory(oom_source_t source, heap_error_code_t os_error_code)
{
	/* need to be filled up */
}

static inline void
vmm_heap_initialize_unusable(vm_heap_t *vmh)
{
	vmh->start_addr = vmh->end_addr = NULL;
	vmh->num_blocks = vmh->num_free_blocks = 0;
}

static void 
vmm_heap_unit_init(vm_heap_t *vmh, size_t size)
{
	ptr_uint_t preferred;
	heap_error_code_t error_code;
	ASSIGN_INIT_LOCK_FREE(vmh->lock, vmh_lock);

	size = ALIGN_FORWARD(size, VMM_BLOCK_SIZE);
	ASSERT(size < MAX_VMM_HEAP_UNIT_SIZE);
	vmh->alloc_size = size;

	if(size == 0)
	{
		vmm_heap_initialize_unusable(&heapmgt->vmheap);
		return;
	}

	preferred = DENTRE_OPTION(vm_base) 
		+ get_random_offset(DENTRE_OPTION(vm_max_offset)/VMM_BLOCK_SIZE) * VMM_BLOCK_SIZE;
	preferred = ALIGN_FORWARD(preferred, VMM_BLOCK_SIZE);
	ASSERT(preferred + size < preferred);

	vmh->start_addr = NULL;
#ifdef N64
	/* need to be filled up */
#endif
	vmh->start_addr = os_heap_reserve((void *)preferred, size, &error_code, true/*+x*/);
    LOG(GLOBAL, LOG_HEAP, 1,
        "vmm_heap_unit_init preferred="PFX" got start_addr="PFX"\n",
        preferred, vmh->start_addr);

	while(vmh->start_addr == NULL && DENTRE_OPTION(vm_allow_not_at_base))
	{
		SYSLOG_INTERNAL_WARNING_ONCE("Preferred vmm heap allocation failed");
		vmh->alloc_size = size + VMM_BLOCK_SIZE;
#ifdef N64
		/* need to be filled up */
#endif
		vmh->alloc_addr = (heap_pc)
			os_heap_reserve(NULL, size + VMM_BLOCK_SIZE, &error_code, true/*+x*/);
		vmh->start_addr = (heap_pc)
			ALIGN_FORWARD(vmh->alloc_addr, VMM_BLOCK_SIZE);
        LOG(GLOBAL, LOG_HEAP, 1, "vmm_heap_unit_init unable to allocate at preferred="
            PFX" letting OS place sz=%dM addr="PFX" \n",
            preferred, size/(1024*1024), vmh->start_addr);

		if(DENTRE_OPTION(vm_allow_smaller))
		{
			size_t sub = (size_t) ALIGN_FORWARD(size/16, 1024 * 1024);
			SYSLOG_INTERNAL_WARNING_ONCE("Full size vmm heap allocation failed");
			if(size > sub)
				size -= sub;
			else
				break;
		}
		else
			break;
	}
#ifdef N64
	/* need to be filled up */
#endif
	if(vmh->start_addr == NULL)
	{
		vmm_heap_initialize_unusable(vmh);
		report_low_on_memory(OOM_INIT, error_code);	/* out of luck, no memory reserve */
	}

	vmh->end_addr = vmh->start_addr + size;
	ASSERT_TRUNCATE(vmh->num_blocks, uint, size / VMM_BLOCK_SIZE);
	vmh->num_blocks = (uint) (size / VMM_BLOCK_SIZE);
	vmh->num_free_blocks = vmh->num_blocks;
	LOG(GLOBAL, LOG_HEAP, 2, "vmm_heap_uint_init ["PFX","PFX") total = %d free = %d\n",
			vmh->start_addr, vmh->end_addr, vmh->num_blocks, vmh->num_free_blocks);

	ASSERT(ALIGNED(MAX_VMM_HEAP_UNIT_SIZE, VMM_BLOCK_SIZE));
	bitmap_initialize_free(vmh->blocks, vmh->num_blocks);

    DOLOG(1, LOG_HEAP, {
        vmm_dump_map(vmh);
    });
	ASSERT(bitmap_check_consistency(vmh->blocks, vmh->num_blocks, vmh->num_free_blocks));
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
