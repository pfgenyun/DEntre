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


/*
 * link.c - fragment linker routines
 */

#include "globals.h"
#include "link.h"
#include "heap.h"
#include "vmareas.h"

static void
coarse_stubs_init();

void * stub_heap;


/* We save 1 byte per stub by not aligning to 16/24 bytes, since
 * infrequently executed and infrequently accessed (heap free list
 * adds to start so doesn't walk list).
 */
#define SEPARATE_STUB_ALLOC_SIZE(flags)	(DIRECT_EXIT_STUB_SIZE(flags))	/*15*23*/

/* thread-shared initialization that should be repeated after a reset */
void
link_reset_init(void)
{
	if(DENTRE_OPTION(separate_private_stubs) ||
	   DENTRE_OPTION(separate_shared_stubs))
	{
		stub_heap = special_heap_init(SEPARATE_STUB_ALLOC_SIZE(0/*default*/),
									  true/* must synch */, true /* +x */,
									  false/* not persistent*/);
#ifdef N64
#endif
	}
}

void 
link_init()
{
	link_reset_init();
	coarse_stubs_init();
}



/***************************************************************************
 * COARSE-GRAIN UNITS
 ***************************************************************************/

static vm_area_vector_t * coarse_stub_areas;

static void
coarse_stubs_init()
{
	VMVECTOR_ALLOC_VECTOR(coarse_stub_areas, GLOBAL_DCONTEXT,
						  VECTOR_SHARED | VECTOR_NEVER_MERGE,
						  coarse_stub_aresa);
}
