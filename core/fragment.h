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

#ifndef _FRAGMENT_H_
#define _FRAGMENT_H_	1

#include <limits.h>

#include "globals.h"


/* Flags, stored in fragment_t->flags bitfield
 */
#define FRAG_IS_TRACE               0x000004

/* Indicates an irregular fragment_t.  In particular, there are no
 * trailing linkstubs after this fragment_t struct.  Note that other
 * "fake fragment_t" flags should be set in combination with this one
 * (FRAG_IS_{FUTURE,EXTRA_VMAREA*,EMPTY_SLOT}, FRAG_FCACHE_FREE_LIST).
 */
#define FRAG_FAKE					0x000100

/* This is not a fragment_t but an fcache free list entry.
 * In current usage this is checked to see if the previous free list entry is
 * a free list entry (see fcache.c's free_list_header_t.flags).
 * This flag MUST be in the bottom 16 bits since free_list_header_t.flags
 * is a ushort!
 */
#define FRAG_FCACHE_FREE_LIST		0x000800

#define FRAG_SHARED					0x1000000

/* Indicates coarse-grain cache management, i.e., batch units with
 * no individual fragment_t.
 */
#define FRAG_COARSE_GRAIN			0x10000000



#ifdef N64
/* this fragment contains 32-bit code */
# define FRAG_32_BIT                0x400000
# ifdef RETURN_STACK
#  error RETURN_STACK not compatible with X64
# endif
#elif defined(RETURN_STACK)
/* used for RETURN_STACK */
# define FRAG_ENDS_WITH_RETURN      0x400000
#endif



/* to save space size field is a ushort => maximum fragment size */
enum { MAX_FRAGMENT_SIZE = USHRT_MAX };


/* fragment structure used for basic blocks and traces 
 * this is the core structure shared by everything
 * trace heads and traces extend it below
 */
struct _fragment_t
{
    /* WARNING: the tag offset is assumed to be 0 in x86/emit_utils.c
     * Also, next and flags' offsets must match future_fragment_t's
     * And flags' offset must match fcache.c's empty_slot_t as well as
     * vmarea.c's multi_entry_t structs
     */
	app_pc tag;		/* non-zero fragment tag used for lookups */

    /* Contains FRAG_ flags.  Should only be modified for FRAG_SHARED fragments
     * while holding the change_linking_lock.
     */
	uint flags;

    /* size in bytes of the fragment (includes body and stubs, and for
     * selfmod fragments also includes selfmod app code copy and size field)
     */    
	ushort size;

    /* both of these fields are tiny -- padding shouldn't be more than a cache line
     * size (32 loongson), prefix should be even smaller.
     * they combine with size to shrink fragment_t for us
     * N.B.: byte is an unsigned char
     */
	byte prefix_size;		/* size of prefix, after which is non-ind. br. entry */
	byte fcache_extra;		/* padding to fit in fcache slot, also includes the header */

	cache_pc start_pc;		/* very top of fragment's code, equals
							 * entry point when indirect branch target */
	union
	{
        /* For a live fragment, we store a list of other fragments' exits that target
         * this fragment (outgoing exit stubs are all allocated with fragment_t struct,
         * use FRAGMENT_EXIT_STUBS() to access).
         */
		linkstub_t *incoming_stub;

        /* For a pending-deletion fragment (marked with FRAG_WAS_DELETED),
         * we store translation info.
         */
		translation_info_t *translation_info;
	}in_xlate;

	fragment_t *next_vmarea;
	fragment_t *prev_vmarea;

	union
	{
		fragment_t *also_vmarea;	/* for chaining fragments across vmarea lists */

        /* For lazily-deleted fragments, we store the flushtime here, as this
         * field is no longer used once a fragment is not live.
         */
		uint flushtime;
	}also;

#ifdef DEBUG
	int id;		/* thread-shared-unique fragment identifier */
#endif

#ifdef CUSTOM_TRACES_RET_REMOVAL
    int num_calls;
    int num_rets;
#endif
};/* fragment__t */


/* Structure used for future fragments, separate to save memory.
 * next and flags must be at same offset as for fragment_t, so that
 * hashtable (next) and link.c (flags) can polymorphize fragment_t
 * and future_fragment_t.  The rule is enforced in fragment_init.
 */
struct _future_fragment_t
{
	app_pc tag;		/* non-zero fragment tag used for lookups */
	uint flags;		/* contains FRAG_ flags */
	linkstub_t *incoming_stubs;	/* list of other fragments' exits that target
									   this fragment */
};


/* We keep basic blocks and traces in separate hashtables.  This is to
 * speed up indirect_branch_lookup that looks for traces only, but it
 * means our lookup function has to look in both hashtables.  This has
 * no noticeable performance impact.  A strategy of having an
 * all-fragment hashtable and a trace-only hashtable that mirrors just
 * the traces of the all-fragment hashtable performs similarly but is
 * more complicated since fragments need two different next fields,
 * plus it uses more memory because traces are in two hashtables
 * simultaneously.
 *
 * FIXME: Shared bb IBL routines indirectly access only a few fields
 * from each fragment_table_t which will touch a separate cache line for
 * each.  However, trace IBL routines don't indirect so I don't expect
 * a performance hit of using the current struct layout.  
 * FIXME: The bb IBL routines however are shared and therefore
 * indirect, so splitting the fragment_table_t in two compactable
 * structures may be worth trying.
 */
typedef struct _per_thread_t 
{
	/* need to be filled up */
}per_thread_t;


void 
fragment_init(void);

void 
fragment_reset_init(void);

#endif
