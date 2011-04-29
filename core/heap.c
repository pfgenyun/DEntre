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

#include <limits.h>		/* UINT_MAX */
#include <string.h>

#include "globals.h"
#include "utils.h"
#include "heap.h"
#include "options.h"
#include "mips/proc.h"
#include "vmareas.h"
#include "fcache.h"

/* need to be filled up */
static const uint BLOCK_SIZES[] = {
    8, /* for instr bits */
#ifndef N64
    /* for x64 future_fragment_t is 24 bytes (could be 20 if we could put flags last) */
//    sizeof(future_fragment_t), /* 12 (24 x64) */
	12,
#endif
    /* we have a lot of size 16 requests for IR but they are transient */
    24, /* fcache empties and vm_area_t are now 20, vm area extras still 24 */
//    ALIGN_FORWARD(sizeof(fragment_t) + sizeof(indirect_linkstub_t), HEAP_ALIGNMENT), /* 40 dbg / 36 rel */
	36,
#if defined(N64) || defined(PROFILE_LINKCOUNT) || defined(CUSTOM_EXIT_STUBS)
//    sizeof(instr_t), /* 64 (104 x64) */
	64,
//    sizeof(fragment_t) + sizeof(direct_linkstub_t)
//        + sizeof(cbr_fallthrough_linkstub_t), /* 68 dbg / 64 rel, 112 x64 */
	64,
    /* all other bb/trace buckets are 8 larger but in same order */
#else
//    sizeof(fragment_t) + sizeof(direct_linkstub_t)
//        + sizeof(cbr_fallthrough_linkstub_t), /* 60 dbg / 56 rel */
	56,
//    sizeof(instr_t), /* 64 */
	64,
#endif
    /* we keep this bucket even though only 10% or so of normal bbs
     * hit this.
     * FIXME: release == instr_t here so a small waste when walking buckets 
     */
//    ALIGN_FORWARD(sizeof(fragment_t) + 2*sizeof(direct_linkstub_t),
//                  HEAP_ALIGNMENT), /* 68 dbg / 64 rel (128 x64) */
	64,
//    ALIGN_FORWARD(sizeof(trace_t) + 2*sizeof(direct_linkstub_t) + sizeof(uint),
//                  HEAP_ALIGNMENT), /* 80 dbg / 76 rel (148 x64 => 152) */
	76,
    /* FIXME: measure whether should put in indirect mixes as well */
//    ALIGN_FORWARD(sizeof(trace_t) + 3*sizeof(direct_linkstub_t) + sizeof(uint),
//                  HEAP_ALIGNMENT), /* 96 dbg / 92 rel (180 x64 => 184) */
	92,
//    ALIGN_FORWARD(sizeof(trace_t) + 5*sizeof(direct_linkstub_t) + sizeof(uint),
//                  HEAP_ALIGNMENT), /* 128 dbg / 124 rel (244 x64 => 248) */
	124,
    256,
    512,
    UINT_MAX /* variable-length */
};

#define BLOCK_TYPES (sizeof(BLOCK_SIZES)/sizeof(uint))
//#define BLOCK_TYPES 12		/* need to be filled up */

#define HEADER_SIZE	sizeof(size_t)

#define GLOBAL_UNIT_MIN_SIZE	INTERNAL_OPTION(initial_global_heap_unit_size)

#define GUARD_PAGE_ADJUSTMENT	(dentre_options.guard_pages ? 2*PAGE_SIZE : 0)

#define UNIT_RESERVED_ROOM(u)		((size_t)(u->end_pc - u->start_pc))

#define UNIT_RESERVED_SIZE(u)	(UNIT_RESERVED_ROOM(u) + sizeof(heap_unit_t))

typedef byte *vm_addr_t;

/* thread-local heap structure
 * this struct is kept at top of unit itself, not in separate allocation
 */
typedef struct _heap_unit_t
{
	heap_pc start_pc;			/* not include heap_uint_t space */
	heap_pc end_pc;				/* alloc addr include heap_uint_t space + commit_size */
	heap_pc cur_pc;				
	heap_pc reserved_end_pc;	/* alloc addr include heap_uint_t space + size */

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
	bool writable;			/* remember state of heap protection */

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

static bool heap_exiting = false;

/* Lock used only for managing heap units, not for normal thread-local alloc.
 * Must be recursive due to circular dependencies between vmareas and global heap.
 * Furthermore, always grab dynamo_vm_areas_lock() before grabbing this lock,
 * to make DR areas update and heap alloc/free atomic!
 */
DECLARE_CXTSWPROT_VAR(static recursive_lock_t heap_unit_lock,
                      INIT_RECURSIVE_LOCK(heap_unit_lock));

/* N.B.: if these two locks are ever owned at the same time, the convention is
 * that global_alloc_lock MUST be grabbed first, to avoid deadlocks
 */
/* separate lock for global heap access to avoid contention between local unit
 * creation and global heap alloc
 * must be recursive so that heap_vmareas_synch_units can hold it and heap_unit_lock
 * up front to avoid deadlocks, and still allow vmareas to global_alloc --
 * BUT we do NOT want global_alloc() to be able to recurse!
 * FIXME: either find a better solution to the heap_vmareas_synch_units deadlock
 * that is as efficient, or find a way to assert that the only recursion is
 * from heap_vmareas_synch_units to global_alloc
 */
DECLARE_CXTSWPROT_VAR(static recursive_lock_t global_alloc_lock,
                      INIT_RECURSIVE_LOCK(global_alloc_lock));

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

static heap_management_t temp_heapmgt;
static heap_management_t *heapmgt = &temp_heapmgt;



static bool
safe_to_allocate_or_free_heap_units()
{
	/* need to be filled up */
	/* grab lock */
}

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


static bool
vmm_is_reserved_unit(vm_heap_t *vmh, vm_addr_t p, size_t size)
{
	size = ALIGN_FOEWARD(size, VMM_BLOCK_SIZE);

	if(p < vmh->start_addr || p> vmh->end_addr /* overflow */ ||
	   p + size > vmh->end_addr )
		return false;

    ASSERT(CHECK_TRUNCATE_TYPE_uint(size/VMM_BLOCK_SIZE));
    ASSERT(bitmap_are_reserved_blocks(vmh->blocks, vmh->num_blocks, 
                                      vmm_addr_to_block(vmh, p),
                                      (uint)size/VMM_BLOCK_SIZE));
    return true;
}

bool
is_vmm_reserved_address(byte *pc, size_t size)
{
    ASSERT(heapmgt != NULL);
    /* Case 10293: we don't call vmm_is_reserved_unit to avoid its
     * assert, which we want to maintain for callers only dealing with
     * DR-allocated addresses, while this routine is called w/ random
     * addresses
     */
    return (heapmgt != NULL && heapmgt->vmheap.start_addr != NULL &&
            pc >= heapmgt->vmheap.start_addr &&
            /* i#14: workaround for gcc 4.3.0 bug */
            (pc < ((byte *)POINTER_MAX) - size) /* no overflow */ &&
            (pc + size) <= heapmgt->vmheap.end_addr);
}

static inline vm_addr_t
vmm_block_to_addr(vm_heap_t *vmh, uint block)
{
	ASSERT(block <= 0 && block < vmh->num_blocks);
	return (vm_addr_t)(vmh->start_addr + block*VMM_BLOCK_SIZE);
}


/* Reservations here are done with VMM_BLOCK_SIZE alignment
 * (e.g. 64KB) but the caller is not forced to request at that
 * alignment.  We explicitly synchronize reservations and decommits
 * within the vm_heap_t.

 * Returns NULL if the VMMHeap is full or too fragmented to satisfy
 * the request.
 */
static vm_addr_t
vmm_heap_reserve_blocks(vm_heap_t *vmh, size_t size_in)
{
	vm_addr_t p;
	uint request;
	uint first_block;
	size_t size;

	size = ALIGNED_FORWARD(size_in, VMM_BLOCK_SIZE);
	ASSERT_TRUNCATE(request, uint, size/VMM_BLOCK_SIZE);
	request = (uint) size / VMM_BLOCK_SIZE;

    LOG(GLOBAL, LOG_HEAP, 2,
        "vmm_heap_reserve_blocks: size=%d => %d in blocks=%d free_blocks~=%d\n",
        size_in, size, request, vmh->num_free_blocks);
	
	mutex_lock(&vmh->lock);

	if(vmh->num_free_blocks < request)
	{
		mutex_unlock(&vmh->lock);
		return NULL;
	}

	first_block = bitmap_allocate_blocks(vmh->blocks, vmh->num_blocks, request);
	if(first_block != BITMAP_NOT_FOUND)
	{
		vmh->num_free_blocks -= request;
	}

	mutex_unlock(&vmh->lock);

	if(first_block != BITMAP_NOT_FOUND)
	{
		p = vmm_block_to_addr(vmh, first_block);
        STATS_ADD_PEAK(vmm_vsize_used, size);
        STATS_ADD_PEAK(vmm_vsize_blocks_used, request);
        STATS_ADD_PEAK(vmm_vsize_wasted, size - size_in);
        DOSTATS({
            if (request > 1) {
                STATS_INC(vmm_multi_block_allocs);
                STATS_ADD(vmm_multi_blocks, request);
            }
        });
	}
	else
	{
		p = NULL;
	}

    LOG(GLOBAL, LOG_HEAP, 2, "vmm_heap_reserve_blocks: size=%d blocks=%d p="PFX"\n",
        size, request, p);
    DOLOG(5, LOG_HEAP, { vmm_dump_map(vmh); });

	return p;
}


/* (1) num_free_blocks/num_blocks < 10% ||
 * (2) vmm_size < reset_at_vmm_free_limit 
 */
static bool
at_reset_at_vmm_limit()
{
	return 
		(DENTRE_OPTION(reset_at_vmm_percent_free_limit) != 0 && 
		 100 * heapmgt->vmheap.num_free_blocks <
		 DENTRE_OPTION(reset_at_vmm_percent_free_limit) * heapmgt->vmheap.num_blocks) ||
		(DENTRE_OPTION(reset_at_vmm_free_limit) != 0 &&
		 heapmgt->vmheap.num_free_blocks * VMM_BLOCK_SIZE < 
		 DENTRE_OPTION(reset_at_vmm_free_limit));
}

/* set bitmap of heapmgt->vmheap */
/* Reserve virtual address space without committing swap space for it */
static vm_addr_t
vmm_heap_reserve(size_t size, heap_error_code_t *error_code, bool executable)
{
	vm_addr_t p;

	ASSERT(size > 0 && ALIGNED(size, PAGE_SIZE));
	ASSERT(!OWN_MUTEX(&reset_pending_lock));

	if(DENTRE_OPTION(vm_reserve))
	{
		if(INTERNAL_OPTION(vm_use_last) ||
		   (DENTRE_OPTION(switch_to_os_at_vmm_reset_limit) && at_reset_at_vmm_limit()))
		{
            DO_ONCE({
                if (DENTRE_OPTION(reset_at_switch_to_os_at_vmm_limit))
                    schedule_reset(RESET_ALL);
                DODEBUG({
                    if (!INTERNAL_OPTION(vm_use_last)) {
                        ASSERT_CURIOSITY(false && "running low on vm reserve");
                    }
                });
                /* FIXME - for our testing would be nice to have some release build
                 * notification of this ... */
            });     

#ifdef N64
			/* need to be filled up */
#endif
			p = os_heap_reserve(NULL, size, error_code, executable);
			if(p != NULL)
				return p;

            LOG(GLOBAL, LOG_HEAP, 1, "vmm_heap_reserve: failed "PFX"\n", 
                *error_code);
		}

		if(at_reset_at_vmm_limit())
		{
			if(schedule_reset(RESET_ALL))		/* only notification ? */
			{
				STATS_INC(reset_low_vmm_count);
                DO_THRESHOLD_SAFE(DENTRE_OPTION(report_reset_vmm_threshold),
                                  FREQ_PROTECTED_SECTION,
                {/* < max - nothing */},
                {/* >= max */
                    /* FIXME - do we want to report more then once to give some idea of 
                     * how much thrashing there is? */
                    DO_ONCE({
                        SYSLOG_CUSTOM_NOTIFY(SYSLOG_WARNING, MSG_LOW_ON_VMM_MEMORY, 2,
                                             "Potentially thrashing on low virtual "
                                             "memory resetting.", get_application_name(),
                                             get_application_pid());
                        /* want QA to notice */
                        ASSERT_CURIOSITY(false && "vmm heap limit reset thrashing");
                    });
                });
			}
		}

		p = vmm_heap_reserve_blocks(&heapmgt->vmheap, size);
        LOG(GLOBAL, LOG_HEAP, 2, "vmm_heap_reserve: size=%d p="PFX"\n", size, p);

        if (p)
			return p;
		/*if (p == NULL) */	
        DO_ONCE({
            DODEBUG({ out_of_vmheap_once = true; });
            if (!INTERNAL_OPTION(skip_out_of_vm_reserve_curiosity)) {
                /* this maybe unsafe for early services w.r.t. case 666 */
                SYSLOG_INTERNAL_WARNING("Out of vmheap reservation - reserving %dKB."
                                        "Falling back onto OS allocation", size/1024);
                ASSERT_CURIOSITY(false && "Out of vmheap reservation");
            }
            /* This actually-out trigger is only trying to help issues like a
             * thread-private configuration being a memory hog (and thus we use up
             * our reserve). Reset needs memory, and this is asynchronous, so no
             * guarantees here anyway (the app may have already reserved all memory
             * beyond our reservation, see sqlsrvr.exe and cisvc.exe for ex.) which is
             * why we have -reset_at_vmm_threshold to make reset success more likely. */
            if (DENTRE_OPTION(reset_at_vmm_full)) {
                schedule_reset(RESET_ALL);
            }
        });
	}

#ifdef N64
	/* need to be filled up */
#endif 
	p = os_heap_reserve(NULL, size, error_code, executable);

	return p;
}


static void
heap_low_on_memory()
{
	/* need to be filled up */
}


static inline void
accout_for_memory(void *p, size_t size, uint prot, bool add_vm _IF_DEBUG(char* comment))
{
	STATS_ADD_PEAK(memory_capacity, size);

	/* areas inside the vmheap reservation are not added to the list */
	if(vmm_is_reserved_unit(&heapmgt->vmheap, p, size))
		return;
	
	if(add_vm)
	{
		add_dentre_vm_area(p, ((app_pc)p) + size, prot, false _IF_DEBUG(comment));
	}
	else
	{
		mark_dentre_vm_areas_stale();
		ASSERT(TESTALL(MEMPROT_READ | MEMPROT_WRITE, prot));
	}
}

static inline bool
vmm_heap_commit(vm_addr_t p, size_t size, uint prot, heap_error_code_t * error_code)
{
	bool res = os_heap_commit(p, size, prot, error_code);

	/* need to be filled up */
	/* reset setting */

	/* need to be filled up */
	/* retry */

	return res;
}

/* Caller is required to handle thread synchronization and to update dynamo vm areas.
 * size must be PAGE_SIZE-aligned.
 * Returns NULL if fails to allocate memory!
 */
static void *
vmm_heap_alloc(size_t size, uint prot, heap_error_code_t *error_code)
{
	vm_addr_t p =  vmm_heap_reserve(size, error_code, TEST(MEMPROT_EXEC, prot));
	if(!p)
		return NULL;

	if(!vmm_heap_commit(p, size, prot, error_code))
		return NULL;

	return p;
}


/* we indirect all os memory requests through here so we have a central place
 * to handle the out-of-memory condition.
 * add_vm MUST be false iff this is heap memory, which is updated separately.
 */
static void *
get_real_memory(size_t size, uint prot, bool add_vm _IF_DEBUG(comment))
{
	void *p;
	heap_error_code_t error_code;

	size = ALIGN_FORWARD(size, PAGE_SIZE);

	/* memory alloc/dealloc and updating DR list must be atomic */
	dentre_vm_areas_lock();

	p = vmm_heap_alloc(size, prot, &error_code);
	if(p == NULL)
	{
        SYSLOG_INTERNAL_WARNING_ONCE("Out of memory -- cannot reserve or "
                                     "commit %dKB.  Trying to recover.", size/1024);
        /* we should be ok here, shouldn't come in here holding global_alloc_lock
         * or heap_unit_lock w/o first having grabbed DR areas lock
         */
		ASSERT(safe_to_allocate_or_free_heap_units());

		heap_low_on_memory();
		fcache_low_on_memory();

        /* try again
         * FIXME: have more sophisticated strategy of freeing a little, then getting
         * more drastic with each subsequent failure
         * FIXME: can only free live fcache units for current thread w/ current
         * impl...should we wait a while and try again if out of memory, hoping
         * other threads have freed some?!?!
         */
		p = vmm_heap_alloc(size, prot, &error_code);
		if(p == NULL)
		{
			report_low_on_memory(OOM_RESERVE, error_code);
		}
        SYSLOG_INTERNAL_WARNING_ONCE("Out of memory -- but still alive after "
                                     "emergency free.");
	}

	accout_for_memory(p, size, prot, add_vm _IF_DEBUG(comment));
	dentre_vm_areas_unlock();

	return p;

}


static void
extend_commitment(vm_addr_t p, size_t size, uint prot, bool initial_commit)
{
	heap_error_code_t error_code;

	ASSERT(ALIGNED(p, PAGE_SIZE));
	size = ALIGN_FORWARD(size, PAGE_SIZE);

	if(!vmm_heap_commit(p, size, prot, &error_code))
	{
        SYSLOG_INTERNAL_WARNING_ONCE("Out of memory - cannot extend commit "
                                     "%dKB. Trying to recover.", size/1024);
        heap_low_on_memory();
        fcache_low_on_memory();
        /* see low-memory ideas in get_real_memory */
        if (!vmm_heap_commit(p, size, prot, &error_code)) {
            report_low_on_memory(initial_commit ? OOM_COMMIT : OOM_EXTEND, 
                                 error_code);
        }

        SYSLOG_INTERNAL_WARNING_ONCE("Out of memory in extend - still alive "
                                     "after emergency free.");
	}
}

/* a wrapper around get_real_memory that adds a guard page on each side of the requested unit.
 * These should consume only uncommitted virtual address and should not use any physical memory.
 * add_vm MUST be false iff this is heap memory, which is updated separately.
 */
static vm_addr_t
get_guarded_real_memory(size_t reserve_size, size_t commit_size, uint prot,
						bool add_vm, bool guarded _IF_DEBUG(char *comment))
{
	vm_addr_t p;
	uint guard_size = PAGE_SIZE;
	heap_error_code_t error_code;

	ASSERT(reserve_size > commit_size);

	if(!guarded || dentre_options.guard_pages)
	{
		if(reserve_size == commit_size)
			return get_real_memory(reserve_size, prot, add_vm _IF_DEBUG(comment));
		guard_size = 0;
	}

	reserve_size = ALIGN_FORWARD(reserve_size, PAGE_SIZE);
	commit_size = ALIGN_FORWARD(commit_size, PAGE_SIZE);

	reserve_size += 2 * guard_size;		/* add top and bottom guards. why guard ? */

	/* memory alloc/dealloc and updating DE list must be atomic */
	dentre_vm_areas_lock();

	p = vmm_heap_reserve(reserve_size, &error_code, TEST(MEMPROT_EXEC, prot));
	if(p == NULL)
	{
        /* This is very unlikely to happen - we have to reach at least 2GB reserved memory. */
        SYSLOG_INTERNAL_WARNING_ONCE("Out of memory - cannot reserve %dKB. "
                                     "Trying to recover.", reserve_size/1024);
		heap_low_on_memory();
		fcache_low_on_memory();

		p = vmm_heap_reserve(reserve_size, &error_code, TEST(MEMPROT_EXEC, prot));
        if (p == NULL) {
            report_low_on_memory(OOM_RESERVE, error_code);
        }

        SYSLOG_INTERNAL_WARNING_ONCE("Out of memory on reserve - but still "
                                     "alive after emergency free.");
	}

    /* includes guard pages if add_vm -- else, heap_vmareas_synch_units() will
     * add guard pages in by assuming one page on each side of every heap unit
     * if dynamo_options.guard_pages
     */
	accout_for_memory((void *)p, reserve_size, prot, add_vm _IF_DEBUG(comment));
	
	dentre_vm_areas_unlock();

    STATS_ADD_PEAK(reserved_memory_capacity, reserve_size);
    STATS_ADD_PEAK(guard_pages, 2);

	p += guard_size;
	extend_commitment(p, commit_size, prot, true/* initial commit */);

	return p;
}


/* size does not include guard pages (if any) and is reserved, but only
 * DENTRE_OPTION(heap_commit_increment) is committed up front
 */
static heap_unit_t *
heap_creat_unit(thread_units_t *tu, size_t size, bool must_be_new)
{
	heap_unit_t *u = NULL;
	heap_unit_t *dead = NULL;
	heap_unit_t *prev_dead = NULL;

	bool new_unit = false;

    /* we do not restrict size to unit max as we have to make larger-than-max
     * units for oversized requests
     */

    /* modifying heap list and DR areas must be atomic, and must grab
     * DR area lock before heap_unit_lock
     */
	ASSERT(safe_to_allocate_or_free_heap_units());
	dentre_vm_areas_lock();
	acquire_recursive_lock(&heap_unit_lock);	/* for heapmgt->heap.units */

	if(!must_be_new)
	{
		for(dead = heapmgt->heap.dead;
			dead != NULL && UNIT_RESERVED_SIZE(dead) < size;
			prev_dead = dead, dead = dead->next_global)
			;
	}

	if(dead != NULL)
	{
		if(prev_dead == NULL)	/* the first dead unit */
			heapmgt->heap.dead = dead->next_global;
		else
			prev_dead->next_global = dead->next_global;
		u = dead;
		heapmgt->heap.num_dead--;
		RSTATS_DEC(heap_num_free);
		release_recursive_lock(&heap_unit_lock);
        LOG(GLOBAL, LOG_HEAP, 2,
            "Re-using dead heap unit: "PFX"-"PFX" %d KB (need %d KB)\n",
            u, ((byte*)u)+size/* start_pc + size */, UNIT_RESERVED_SIZE(u)/1024, size/1024);
	}
	else
	{
		/* first time this way */
		size_t commit_size = DENTRE_OPTION(heap_commit_increment);	/* 4K */
		release_recursive_lock(&heap_unit_lock);

		/* create new unit */
		ASSERT(commit_size < size);		/* why */

		u = (heap_unit_t *)
			get_guarded_real_memory(size, commit_size, MEMPROT_READ|MEMPROT_WRITE, 
									false, true _IF_DEBUG(""));
		new_unit = true;
		ASSERT(u);
        LOG(GLOBAL, LOG_HEAP, 2, "New heap unit: "PFX"-"PFX"\n", u, ((byte*)u)+size);

		u->start_pc = (heap_pc)(((ptr_uint_t)u) + sizeof(heap_unit_t));
		u->end_pc = ((heap_pc)u) + commit_size;		/* why is commit_size */
		u->reserved_end_pc = ((heap_pc)u) + size;
		u->in_vmarea_list = false;

        STATS_ADD(heap_capacity, commit_size);
        STATS_MAX(peak_heap_capacity, heap_capacity);
        /* FIXME: heap sizes are not always page-aligned so stats will be off */
        STATS_ADD_PEAK(heap_reserved_only, (u->reserved_end_pc - u->end_pc));
	}
	RSTATS_ADD_PEAK(heap_num_live, 1);

	u->cur_pc = u->start_pc;
	u->next_local = NULL;

    DODEBUG({
        u->id = tu->num_units;
        tu->num_units++;
    });

	acquire_recursive_lock(&heap_unit_lock);
	/* insert u to the head of heapmgt->heap.units */	
	u->next_global = heapmgt->heap.units;
	if(heapmgt->heap.units != NULL)
		heapmgt->heap.units->prev_global = u;
	u->prev_global = NULL;
	heapmgt->heap.units = u;
	
	release_recursive_lock(&heap_unit_lock);
	dentre_vm_area_unlock();

#ifdef DEBUG_MEMORY
    memset(u->start_pc, HEAP_UNALLOCATED_BYTE, u->end_pc - u->start_pc);
#endif

    return u;

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


static void
threadunits_init(dcontext_t *dcontext, thread_units_t *tu, size_t size)
{
	int i;
	DODEBUG({tu->num_units = 0});

	tu->top_unit = heap_creat_unit(tu, size-GUARD_PAGE_ADJUSTMENT, false/* can reuse*/);
	tu->cur_unit = tu->top_unit;
	tu->dcontext = dcontext;
	tu->writable = true;
#ifdef HEAP_ACCOUTING
	memset(&tu->acct, 0, sizeof(tu->acct));
#endif
	for(i=0; i<BLOCK_TYPES; i++)
		tu->free_list[i] = NULL;
}

void
heap_reset_init()
{
	if(DENTRE_OPTION(enable_reset))
	{
		threadunits_init(GLOBAL_DCONTEXT, &heapmgt->global_nonpersistent_units,
						 GLOBAL_UNIT_MIN_SIZE);
	}
}

void
heap_init(void)
{
	int i;
	uint prev_sz = 0;

	LOG(GLOBAL, LOG_TOP|LOG_HEAT, 2, "heap bucket sizes are:\n");

	ASSERT(ALIGNED(HEADER_SIZE, HEAP_ALIGNMENT));

	ASSERT(BLOCK_SIZES[0] >= sizeof(heap_pc *));

	for(i=0; i<BLOCK_TYPES; i++)
	{
		/* need to be filled up */
		ASSERT(BLOCK_SIZES[i] > prev_sz);
		ASSERT(i == BLOCK_TYPES - 1 || ALIGNED(BLOCK_SIZES[i], HEAP_ALIGNMENT));

		prev_sz = BLOCK_SIZES[i];
		LOG(GLOBAL, LOG_TOP|LOG_HEAP, 2, "\t%d bytes\n", BLOCK_SIZES[i]);
	}

    /* we assume writes to some static vars are atomic,
     * i.e., the vars don't cross cache lines.  they shouldn't since
     * they should all be 4-byte-aligned in the data segment.
     * FIXME: ensure that release build aligns ok?
     * I would be quite surprised if static vars were not 4-byte-aligned!
     */
    ASSERT(ALIGN_BACKWARD(&heap_exiting, CACHE_LINE_SIZE()) ==
           ALIGN_BACKWARD(&heap_exiting + 1, CACHE_LINE_SIZE()));
    ASSERT(ALIGN_BACKWARD(&heap_unit_lock.owner, CACHE_LINE_SIZE()) ==
           ALIGN_BACKWARD(&heap_unit_lock.owner + 1, CACHE_LINE_SIZE()));

	ASSERT(heapmgt == &temp_heapmgt);
	heapmgt->global_heap_writable = true;
	threadunits_init(GLOBAL_DCONTEXT, &heapmgt->global_units, GLOBAL_UNIT_MIN_SIZE/* 32k */);

	heapmgt = HEAP_TYPE_ALLOC(GLOBAL_DCONTEXT, heap_management_t, ACCT_MEM_MGT, PROTECTED);
	memset(heapmgt, 0, sizeof(*heapmgt));

	ASSERT(sizeof(temp_heapmgt) == sizeof(*heapmgt));
	memcpy(heapmgt, &temp_heapmgt, sizeof(temp_heapmgt));

	threadunits_init(GLOBAL_DCONTEXT, &heapmgt->global_unprotected_units, GLOBAL_UNIT_MIN_SIZE);

	heap_reset_init();

}


/* allocate storage on the DR heap
 * returns NULL iff caller needs to grab dynamo_vm_areas_lock() and retry
 */
static void *
common_heap_alloc(thread_units_t *tu, size_t size HEAPACCT(which_heap_t which))
{
	heap_unit_t *u = tu->cur_unit;
	heap_pc p = NULL;
	int bucket  = 0;
	size_t alloc_size, aligned_size;


#if 0		/* hard to understand */

#if define(DEBUG_MEMORY) && define(DEBUG)
	/* need to be filled up */
#endif

	ASSERT(size > 0);
	ASSERT(size < MAX_VALID_HEAP_ALLOCATION && "potential integer overflow");

	if(size > MAX_VALID_HEAP_ALLOCATION)
		return NULL;

    /* NOTE - all of our buckets are sized to preserve alignment, so this can't change
     * which bucket is used. */
	aligned_size = ALIGNED_FORWARD(size, HEAP_ALIGNMENT);
	while(aligned_size > BLOCK_SIZES[bucket])
		bucket ++;
	if(bucket == BLOCK_SIZE-1)
		alloc_size = aligned_size + HEADER_SIZE;
	else
		alloc_size = BLOCK_SIZES[bucket];
	ASSERT(size <= alloc_size);

#ifdef DEBUG_MEMORY
    /*  use original calculated size for later check */
    check_alloc_size = alloc_size;
#endif

	if(alloc_size > MAXROOM)
	{
        /* too big for normal unit, build a special unit just for this allocation */
        /* don't need alloc_size or even aligned_size, just need size */
		heap_unit_t *new_unit, *prev;
		/* we page-align to avoid wasting space if unit gets reused later */
		size_t unit_size = ALIGNED_FORWARD(size + sizeof(heap_unit_t), PAGE_SIZE);

		ASSERT(size < unit_size);

		if(!safe_to_allocate_or_free_heap_units())
			return NULL;
		
        /* Can reuse a dead unit if large enough: we'll just not use any
         * excess size until this is freed and put back on dead list.
         * (Currently we don't put oversized units on dead list though.)
         */
		new_unit = heap_creat_unit(tu, unit_size, false/* can be reuse */);
		/* we want to commit the whole alloc right away */
		heap_unit_extend_commitment(new_unit, size, MEMPROT_READ|MEMPROT_WRITE);

		prev = tu->top_unit;
		alloc_size = size;
        /* insert prior to cur unit (new unit will be full, so keep cur unit
         * where it is)
         */
		while(prev != u && prev->next_local != u)
		{
			ASSERT(prev != NULL && prev->next_local != NULL);
			prev = prev->next_local;
		}
		if(prev == u)
		{
			ASSERT(prev == tu->top_unit);
			tu->top_unit = new_unit;
		}
		else
		{
			prev->next_local = new_unit;
		}
		new_unit->next_local = u;

#ifdef DEBUG_MEMORY
        LOG(THREAD, LOG_HEAP, 3,
            "\tCreating new oversized heap unit %d (%d [/%d] KB)\n",
            new_unit->id, UNIT_COMMIT_SIZE(new_unit)/1024,
            UNIT_RESERVED_SIZE(new_unit)/1024);
#endif

		p = new_unit->start_pc;
		new_unit->cur_pc += size;

		ACCOUNT_FOR_ALLOC(alloc_new, tu, which, size, size); /* use alloc_size? */
		goto done_allocating;
	}

	if(tu->free_list[bucket] != NULL)
	{
		if(bucket == BLOCK_TYPES-1)
		{
			/* variable-length blocks, try to find one big enough */
			size_t sz;
			heap_pc next = tu->free_list[bucket];
			heap_pc prev;

			/* need to be filled up */
			/* hard to understand */
		}
		else
		{
			/* fixed-length free block available */
			p = tu->free_list[bucket];
			tu->free_list[bucket] = *((heaP_pc *)p);
			ASSERT(ALIGNED(tu->free_list[bucket], HEAP_ALIGNMENT));
#ifdef DEBUG_MEMORY
            /* ensure memory we got from the free list is in a heap unit */
            ASSERT(find_heap_unit(tu, p, alloc_size) != NULL);
#endif
            ACCOUNT_FOR_ALLOC(alloc_reuse, tu, which, alloc_size, aligned_size);
		}
	}

	if(p == NULL)
	{
	
	}

	/* need to be filled up */
	/* DOSTATS() */

done_allocating:

#ifdef DEBUG_MEMORY
	/* need to be filled up */
#endif

#endif

	return p;
}


/* shared between global and global_unprotected */
static void *
common_global_heap_alloc(thread_units_t *tu, size_t size HEAPACCT(which_heap_t chich))
{
	void *p;
	acquire_recursive_lock(&global_alloc_lock);
	p = common_heap_alloc(tu, size HEAPACCT(which));
	release_recursive_lock(&global_alloc_lock);

	if(p == NULL)
	{
        /* circular dependence solution: we need to hold DR lock before
         * global alloc lock -- so we back out, grab it, and retry
         */
		dentre_vm_areas_lock();
		acquire_recursive_lock(&global_alloc_lock);
		p = common_heap_alloc(tu, size HEAPACCT(which));
		release_recursive_lock(&global_alloc_lock);
		dentre_vm_area_unlock();
	}

	ASSERT(p != NULL);

	return p;
}


/* these functions use the global heap instead of a thread's heap: */
void *
global_heap_alloc(size_t size HEAPACCT(which_heap_t which))
{
	void *p = common_global_heap_alloc(&heapmgt->global_units, size HEAPACCT(which));

	ASSERT(p != NULL);
	LOG(GLOBAL, LOG_HEAP, 6, "\nglobal alloc: "PFX" (%d bytes)\n", p, size);

	return p;

}

void *
heap_alloc(dcontext_t *dcontext, size_t size HEAPACCT(which_heap_t which))
{
	thread_units_t *tu;
	void *ret_val;

	if(dcontext == GLOBAL_DCONTEXT)
		return global_heap_alloc(size HEAPACCT(which));

	tu = ((thread_heap_t *)dcontext->heap_field)->local_heap;

	ret_val = common_heap_alloc(tu, size HEAPACCT(which));

	ASSERT(ret_val != NULL);

	return ret_val;
}

void *
global_unprotected_heap_alloc(size_t size HEAPACCT(which_heap_t which))
{
	void *p = common_global_heap_alloc(&heapmgt->global_unprotected_units,
									   size HEAPACCT(which));

    ASSERT(p != NULL);
    LOG(GLOBAL, LOG_HEAP, 6, "\nglobal unprotected alloc: "PFX" (%d bytes)\n", p, size);

    return p;
}

void *
nonpersistent_heap_alloc(dcontext_t *dcontext, size_t size HEAPACCT(which_heap_t which))
{
	void *p;
	if(DENTRE_OPTION(enable_reset))
	{
		if(dcontext == GLOBAL_DCONTEXT)
		{
			p = common_global_heap_alloc(&heapmgt->global_nonpersistent_units,
										 size HEAPACCT(which));
            LOG(GLOBAL, LOG_HEAP, 6,
                "\nglobal nonpersistent alloc: "PFX" (%d bytes)\n", p, size);
		}
		else
		{
			thread_units_t *nph = ((thread_heap_t *) dcontext->heap_field)->nonpersistent_heap;
			p = common_heap_alloc(nph, size HEAPACCT(which));
		}
	}
	else
	{
		p = heap_alloc(dcontext, size HEAPACCT(which));
	}

	ASSERT(p != NULL);

	return p;
}



/* We cannot incrementally keep dynamo vm area list up to date due to
 * circular dependencies bet vmareas and global heap (trust me, I've tried
 * to support it with reentrant routines and recursive locks, the hard part
 * is getting add_vm_area to be reentrant or to queue up adding areas,
 * I think this solution is much more elegant, plus it avoids race conditions
 * between DR memory allocation and the vmareas list by ensuring the list
 * is up to date at the exact time of each query).
 * Instead we on-demand walk the units.
 * Freed units can usually be removed incrementally, except when we
 * hold the heap_unit_lock when we run out of memory -- when we set
 * a flag telling the caller of this routine to remove all heap areas
 * from the vm list prior to calling us to add the real ones back in.
 * Re-adding everyone is the simplest policy, so we don't have to keep
 * track of who's been added.
 * The caller is assumed to hold the dynamo vm areas write lock.
 */
void 
heap_vmareas_synch_units()
{
	/* need to filled up */
	/* clean the heap unit in vmareas list and reconstruct it in vmareas*/
	/* not a good idea */
}
