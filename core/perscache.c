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

void 
perscache_init(void)
{
	
    if (DENTRE_OPTION(use_persisted) && 
        DENTRE_OPTION(persist_per_user) &&
        DENTRE_OPTION(validate_owner_dir)) 
	{
        char dir[MAXINUM_PATH];

		/* need to be filled up */
	}
}
