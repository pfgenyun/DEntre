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

#ifndef _LINK_H_
#define _LINK_H_	1

#include "globals.h"

/* linkstub_t heap layout is now quite variable.  linkstubs are laid out
 * after the fragment_t structure, which is itself variable.
 * The indirect_linkstub_t split was case 6468
 * The cbr_fallthrough_linkstub_t split was case 6528
 *
 *   fragment_t/trace_t
 *   array composed of three different sizes of linkstub_t subclasses:
 *     direct_linkstub_t
 *     cbr_fallthrough_linkstub_t
 *     indirect_linkstub_t
 *   post_linkstub_t
 *   
 * There are three types of specially-supported basic blocks that
 * have no post_linkstub_t:
 *   
 *   fragment_t
 *   indirect_linkstub_t
 *   
 *   fragment_t
 *   direct_linkstub_t
 *   direct_linkstub_t
 *
 *   fragment_t
 *   direct_linkstub_t
 *   cbr_fallthrough_linkstub_t
 */

/* To save memory, we have three types of linkstub structs: direct,
 * cbr_fallthrough, and indirect.  They each contain the flags field
 * at the same offset, so this struct can be used as a superclass.
 */
struct _linkstub_t
{
	ushort flags;	/* contains LINK_ flags above */

    /* We limit all fragment bodies to USHRT_MAX size
     * thus we can save space by making the branch ptr a ushort offset.
     * Do not directly access this field -- use EXIT_CTI_PC()
     */
	ushort cti_offset;	/* offset from fragment start_pc of this cti */

#ifdef CUSTOM_EXIT_STUBS
    ushort         fixed_stub_offset;  /* offset in bytes of fixed part of exit stub from
                                          stub_pc, which points to custom prefix of stub */
#endif

#ifdef PROFILE_LINKCOUNT
    linkcount_type_t count;
#endif
};


void link_init(void);
void link_reset_init(void);


#endif
