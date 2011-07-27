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


/* linkstub_t flags
 * WARNING: flags field is a ushort, so max flag is 0x8000!
 */
enum {
    /***************************************************/
    /* these first flags are also used on the instr_t data structure */

    /* Type of branch and thus which struct is used for this exit.
     * Due to a tight namespace (flags is a ushort field), we pack our
     * 3 types into these 2 bits, so the LINKSTUB_ macros are used to
     * distinguish, rather than raw bit tests:
     *
     *   name          LINK_DIRECT LINK_INDIRECT  struct
     *   ---------------  ---         ---         --------------------------
     *   (subset of fake)  0           0          linkstub_t          
     *   normal direct     1           0          direct_linkstub_t
     *   normal indirect   0           1          indirect_linkstub_t
     *   cbr fallthrough   1           1          cbr_fallthrough_linkstub_t
     *
     * Note that we can have fake linkstubs that should be treated as
     * direct or indirect, so LINK_FAKE is a separate flag.
     */
    LINK_DIRECT          = 0x0001,
    LINK_INDIRECT        = 0x0002,
    /* more specifics on type of branch
     * must check LINK_DIRECT vs LINK_INDIRECT for JMP and CALL.
     * absence of all of these is relied on as an indicator of shared_syscall
     * in indirect_linkstub_target(), so we can't get rid of LINK_RETURN
     * and use absence of CALL & JMP to indicate it.
     */
    LINK_RETURN          = 0x0004,
    LINK_CALL            = 0x0008,
    LINK_JMP             = 0x0010,
    /* if need another flag, could use JMP|CALL to indicate JMP_PLT,
     * and make sure all testers for CALL also test for !JMP
     */
    LINK_IND_JMP_PLT     = 0x0020,

    LINK_SELFMOD_EXIT    = 0x0040,
#ifdef UNSUPPORTED_API
    LINK_TARGET_PREFIX   = 0x0080,
#endif
#ifdef N64
    /* PR 257963: since we don't store targets of ind branches, we need a flag
     * so we know whether this is a trace cmp exit, which has its own ibl entry
     */
    LINK_TRACE_CMP       = 0x0100,
#endif
   /* PR 286922: we support both OP_sys{call,enter}- and OP_int-based system calls */
    LINK_NI_SYSCALL_INT  = 0x0200,
    /* indicates whether exit is before a non-ignorable syscall */
    LINK_NI_SYSCALL      = 0x0400,
    LINK_FINAL_INSTR_SHARED_FLAG = LINK_NI_SYSCALL,
    /* end of instr_t-shared flags  */
    /***************************************************/

    LINK_FRAG_OFFS_AT_END= 0x0800,

    LINK_END_OF_LIST     = 0x1000,

    LINK_FAKE            = 0x2000,

    LINK_LINKED          = 0x4000,

    LINK_SEPARATE_STUB   = 0x8000,

    /* WARNING: flags field is a ushort, so max flag is 0x8000! */
};


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

void
set_last_exit(dcontext_t *dcontext, linkstub_t *l);

const linkstub_t *
get_starting_linkstub(void);

#endif
