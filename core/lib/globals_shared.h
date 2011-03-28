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

#ifndef _GLOBALS_SHARED_H_
#define _GLOBALS_SHARED_H_

#ifndef __cplusplus
#	ifndef DE_DO_NOT_DEFINE_bool
typedef int		bool;
#	endif
#define true	(1)
#define false	(0)
#endif

#define TESTALL(mask, var)	((mask) & (var) == (mask))
#define TESTANY(mask, var)	((mask) & (var) != 0 )
#define TEST	TESTANY

#define EXPANDSTR(x)	#x
#define STRINGIFY(x)	EXPANDSTR(x)

#define DENTRE_VAR_RUNUNDER_ID	DENTRE_RUNUNDER

#define DENTRE_VAR_RUNUNDER	STRINGIFY(DENTRE_VAR_RUNUNDER_ID)


enum {
    /* FIXME: keep in mind that we only read decimal values */
    RUNUNDER_OFF                  = 0x00,   /* 0 */
    RUNUNDER_ON                   = 0x01,   /* 1 */
    RUNUNDER_ALL                  = 0x02,   /* 2 */

    /* Dummy field to keep track of which processes were RUNUNDER_EXPLICIT
     * before we moved to -follow_systemwide by default (for -early_injection)
     * (note this was the old RUNUNDER_EXPLICIT value) */
    RUNUNDER_FORMERLY_EXPLICIT    = 0x04,   /* 4 */

    RUNUNDER_COMMANDLINE_MATCH    = 0x08,   /* 8 */
    RUNUNDER_COMMANDLINE_DISPATCH = 0x10,  /* 16 */
    RUNUNDER_COMMANDLINE_NO_STRIP = 0x20,  /* 32 */
    RUNUNDER_ONCE                 = 0x40,  /* 64 */

    RUNUNDER_EXPLICIT             = 0x80,  /* 128 */
};


#endif
