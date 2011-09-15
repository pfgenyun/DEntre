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

#ifndef MIPS_ARCH_H
#define MIPS_ARCH_H


/* Each thread needs its own copy of these routines, but not all
 * routines here are created in a thread-private: we could save space
 * by splitting into two separate structs.
 *
 * On MIPS , we only have thread-shared generated routines,
 * including do_syscall and shared_syscall and detach's post-syscall
 * continuation.
 */
typedef struct _generated_code_t
{
	byte *fcache_enter;
	byte *fcache_return;

	/* need to be filled up */

}generated_code_t;

#endif
