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
#include "heap.h"
#include "moduledb.h"

#include <string.h>

/* An array of pointers to the various exempt lists indexed by the 
 * moduledb_exempt_list_t enum.  We have the ugliness of an extra indirection
 * and dynamic sizing to move the array into the heap and avoid self 
 * protection changes. */
static char **exemption_lists = NULL;
#define GET_EXEMPT_LIST(list) (exemption_lists[(list)])


void 
process_control_init()
{
	/* need to be filled up */
}


static const char *const
exempt_list_names[MODULEDB_EXEMPT_NUM_LISTS] = { "rct", "image", "dll2heap", "dll2stack" };

void
moduledb_init()
{
	uint exempt_array_size = (MODULEDB_EXEMPT_NUM_LISTS) * sizeof(char *);
	ASSERT(exemption_lists == NULL);
	exemption_lists = (char **)
		global_heap_alloc(exempt_array_size HEAPACCT(ACCT_OTHER));
	ASSERT(exemption_lists != NULL);
	memset(exemption_lists, 0, exempt_array_size);

    DODEBUG({
        int i;
        for (i = 0; i < MODULEDB_EXEMPT_NUM_LISTS; i++)
            ASSERT(exempt_list_names[i] != NULL);
    });
}
