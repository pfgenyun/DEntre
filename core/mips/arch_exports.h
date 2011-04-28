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

#ifndef _ARCH_EXPORTS_H_
#define _ARCH_EXPORTS_H_	1

/* Translation table entry (case 3559).
 * PR 299783: for now we only support pc translation, not full arbitrary reg
 * state mappings, which aren't needed for DR but may be nice for clients.
 */
typedef struct _translation_entry_t
{
	/* offset from fragment start_pc */
	ushort cache_offs;
	/* TRANSLATE_ flags */
	ushort flags;
	app_pc app;
}translation_entry_t;

/* Translation table that records info for translating cache pc to app
 * pc without reading app memory (used when it is unsafe to do so).
 * The table records only translations at change points, so the
 * recreater must interpolate between them, using either a stride of 0
 * if the previous translation entry is marked "identical" or a stride
 * equal to the instruction length as we decode from the cache if the
 * previous entry is !identical=="contiguous".
 */
typedef struct _translation_info_t
{
	uint num_entries;
	/* an array of num_entries elements */
	translation_entry_t translation[1];
}translation_info_t;

void arch_init(void);



#endif
