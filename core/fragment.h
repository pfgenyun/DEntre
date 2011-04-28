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


/* Indicates an irregular fragment_t.  In particular, there are no
 * trailing linkstubs after this fragment_t struct.  Note that other
 * "fake fragment_t" flags should be set in combination with this one
 * (FRAG_IS_{FUTURE,EXTRA_VMAREA*,EMPTY_SLOT}, FRAG_FCACHE_FREE_LIST).
 */
#define FRAG_FAKE				0x000100

/* This is not a fragment_t but an fcache free list entry.
 * In current usage this is checked to see if the previous free list entry is
 * a free list entry (see fcache.c's free_list_header_t.flags).
 * This flag MUST be in the bottom 16 bits since free_list_header_t.flags
 * is a ushort!
 */
#define FRAG_FCACHE_FREE_LIST	0x000800

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



#endif
