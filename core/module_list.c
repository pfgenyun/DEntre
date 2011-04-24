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
#include "vmareas.h"
#include "heap.h"
#include "utils.h"
#include "module_shared.h"


/* Used for maintaining our module list.  Custom field points to
 * further module information from PE/ELF headers.
 * module_data_lock needs to be held when accessing the custom data fields.
 * Kept on the heap for selfprot (case 7957).
 * For Linux this is a vector of segments to handle non-contiguous
 * modules (i#160/PR 562667).
 */
vm_area_vector_t *loaded_module_areas;



void 
modules_init()
{
	VMVECTOR_ALLOC_VECTOR(loaded_module_areas, GLOBAL_DCONTEXT,
							/* case 10335: we always use module_data_lock */
						  VECTOR_SHARED | VECTOR_NEVER_MERGE | VECTOR_NO_LOCK,
						  loaded_module_areas);

	os_modules_init();
}
