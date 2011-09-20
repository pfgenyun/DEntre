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

#ifndef _MONITOR_H_
#define _MONITOR_H_	1

//#include "globals.h"

typedef struct _monitor_data_t
{
	/* need to be filled up  */
}monitor_data_t;


void monitor_init(void);
void monitor_thread_init(dcontext_t *dcontext);

#endif
