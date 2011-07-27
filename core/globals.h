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

#ifndef _GLOBALS_H_
#define _GLOBALS_H_	1

#include <stdlib.h>
#include <stdio.h>

#include "lib/globals_shared.h"
//#include "linux/os_exports.h"	/* below use dcontext_t*/
#include "utils.h"
#include "lib/de_stats.h"
//#include "mips/arch_exports.h" /* below use dcontext_t*/
#include "options.h"

#define INVALID_THREAD_ID  0

typedef byte * cache_pc;	/* fragment cache pc */

#define SUCCESS	(1)
#define FAILURE	(0)

#ifdef USE_VISIBILITY_ATTRIBUTES
#	define DENTRE_EXPORT	__attribute__ ((visibility ("protected")))
#else
#define DENTRE_EXPORT	
#endif


extern bool dentre_exited;

/* global instance of statistics struct */
extern de_statistics_t *stats;

struct _dcontext_t;
typedef struct _dcontext_t dcontext_t;
struct vm_area_vector_t;
typedef struct vm_area_vector_t vm_area_vector_t;
struct _fragment_t;
typedef struct _fragment_t fragment_t;
struct _linkstub_t;
typedef struct _linkstub_t linkstub_t;
struct _coarse_info_t;
typedef struct _coarse_info_t coarse_info_t;
struct _future_fragment_t;
typedef struct _future_fragment_t future_fragment_t;



/* where the current app thread's control is */
typedef enum {
    WHERE_APP=0, 
    WHERE_INTERP,
    WHERE_DISPATCH,
    WHERE_MONITOR,
    WHERE_SYSCALL_HANDLER,
    WHERE_SIGNAL_HANDLER,
    WHERE_TRAMPOLINE,
    WHERE_CONTEXT_SWITCH,
    WHERE_IBL,
    WHERE_FCACHE,
    WHERE_UNKNOWN,
#ifdef HOT_PATCHING_INTERFACE
    WHERE_HOTPATCH,
#endif
    WHERE_LAST
} where_am_i_t;


typedef struct
{
	de_mcontext_t mcontext;			/* real machine context (in arch_exports.h) */
	int error;
    bool at_syscall;                /* for shared deletion syscalls_synch_flush,
                                     * as well as syscalls handled from dispatch,
                                     * and for reset to identify when at syscalls
                                     */
} unprotected_context_t;

struct _dcontext_t
{

    /* if SELFPROT_DCONTEXT, must split dcontext into unprotected and
     * protected fields depending on whether they must be read-only
     * when in the code cache.
     * we waste sizeof(unprotected_context_t) bytes to provide runtime flexibility:
     */
    union {
        /* we use separate_upcontext if 
         *    (TEST(SELFPROT_DCONTEXT, dynamo_options.protect_mask))
         * else we use the inlined upcontext
         */
        unprotected_context_t *separate_upcontext;
        unprotected_context_t upcontext;
    } upcontext;
    /* HACK for mips.asm lack of runtime param access: this is either
     * a self-ptr (to inlined upcontext) or if we separate upcontext it points there.
     */
    unprotected_context_t *upcontext_ptr;
    

    /* The next application pc to execute.
     * Also used to store the cache pc to execute when entering the code cache,
     * and set to the sentinel value BACK_TO_NATIVE_AFTER_SYSCALL for native_exec.
     * FIXME: change to a union?
     */
	app_pc	next_tag;

	linkstub_t *	last_exit;	/* last exit from cache */
	byte *	dstack;	/* thread-private dynamo stack */	

    /* These are expected to retain their values across an entire native execution.
     * We assume that callbacks and native_exec executions are properly nested, and
     * we don't share these across callbacks, using the original value on returning.
     */
	app_pc	native_exec_retval;		/* native_exec return address */
	app_pc	native_exec_retloc;		/* native_exec return address app stack location */

#ifdef RETURN_STACK
	/* need to be filled up */
#endif

    /* Coarse-grain cache exits require extra state storage as they do not
     * use per-exit separate data structures
     */
	union
	{
        /* Indirect branches store on exit the source tag (with the type of
         * branch coming from fake linkstubs), while direct store the source unit
         */
		app_pc	src_tag;
		coarse_info_t *dir_exit;
	}coarse_exit;

	/************* end of offset-crucial fields *********************/

    /* FIXME: now that we initialize a new thread's dcontext right away, and
     * a new callback's as well, we should be able to get rid of this
     */
	bool	initialized;	/* has this context been used yet? */
	thread_id_t		owning_thread;
	process_id_t	owning_process;	/* handle shared address space w/o shared pid */
	where_am_i_t	whereami;	/* where control is at the moment */
	void *	allocated_start;	/* used for cache alignment */
	fragment_t * last_fragment;	/* cached value of linkstub_fragment(last_exit) */

    reg_t          sys_param0;      /* used for post_system_call */
    reg_t          sys_param1;      /* used for post_system_call */
    reg_t          sys_param2;      /* used for post_system_call */
    reg_t          sys_param3;      /* used for post_system_call */

#ifdef N64
	bool	n64_mode;
#endif

	void *	heap_field;

	bool	signals_pending;

    /* must store post-intercepted-syscall target to allow using normal
     * dispatch() for native_exec syscalls
     */
	app_pc	native_exec_postsyscall;	

};


#include "mips/arch_exports.h"
#include "linux/os_exports.h"

/* size of each Dynamo thread-private stack */
#define DENTRE_STACK_SIZE dentre_options.stack_size	/* 20k or 12k*/

typedef struct _thread_record_t
{
	thread_id_t id;		/* thread id */
	process_id_t pid;	/* thread group id */
	bool execve;		/* exiting due to execve*/

	uint num;							/* creation ordinal */
	bool under_dentre_control;			/* used for deciding whether to intercept events */
	dcontext_t *dcontext;					/* allows other threads to see this thread's context */
	struct _thread_record_t *next;
}thread_record_t;

/* in dentre.c */
/* 9-bit addressed hash table takes up 2K, has capacity of 512
 * we never resize, assuming won't be seeing more than a few hundred threads
 */
#define ALL_THREADS_HASH_BITS 9
int get_num_threads(void);

int dentre_thread_init(byte *dstack_in _IF_CLIENT_INTERFACE(bool client_thread));

#define GLOBAL_DCONTEXT	((dcontext_t *)PTR_UINT_MINUS_1)

#endif
