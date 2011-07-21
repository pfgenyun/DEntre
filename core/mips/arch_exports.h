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

#include "../globals.h"

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



/* Merge w/ _LENGTH enum below? */
/* not ifdef X64 to simplify code */
/* need to be filled up */
#define SIZE64_MOV_XAX_TO_TLS         4
#define SIZE64_MOV_XBX_TO_TLS         4
#define SIZE64_MOV_PTR_IMM_TO_XAX     4
#define SIZE64_MOV_PTR_IMM_TO_TLS     4 
#define SIZE32_MOV_XAX_TO_TLS         4
#define SIZE32_MOV_XBX_TO_TLS         4
#define SIZE32_MOV_XAX_TO_TLS_DISP32  4
#define SIZE32_MOV_XBX_TO_TLS_DISP32  4
#define SIZE32_MOV_XAX_TO_ABS         4
#define SIZE32_MOV_XBX_TO_ABS         4
#define SIZE32_MOV_PTR_IMM_TO_XAX     4
#define SIZE32_MOV_PTR_IMM_TO_TLS     4


#ifdef N64
#	define FLAG_IS_32(flags)	(TEST(FLAG_32_BIT, (flags)))
#else
#	define FLAG_IS_32(flags)	true
#endif


/* exported for DYNAMO_OPTION(separate_private_stubs)
 * FIXME: find better way to export -- would use global var accessed
 * by macro, but easiest to have as static initializer for heap bucket
 */
/* for -thread_private, we're relying on the fact that
 * SIZE32_MOV_XAX_TO_TLS == SIZE32_MOV_XAX_TO_ABS, and that
 * x64 always uses tls
 */
#define DIRECT_EXIT_STUB_SIZE32	\
	(SIZE32_MOV_XAX_TO_TLS + SIZE32_MOV_PTR_IMM_TO_XAX + JMP_LONG_LENGTH)
#define DIRECT_EXIT_STUB_SIZE64	\
	(SIZE64_MOV_XAX_TO_TLS + SIZE64_MOV_PTR_IMM_TO_XAX + JMP_LONG_LENGTH)
#define DIRECT_EXIT_STUB_SIZE(flags)	\
	(FLAG_IS_32(flag) ? DIRECT_EXIT_STUB_SIZE32 : DIRECT_EXIT_STUB_SIZE64)

/* need to be filled up */
enum {
    MAX_INSTR_LENGTH = 4,
    /* size of 32-bit-offset jcc instr, assuming it has no
     * jcc branch hint!
     */
    CBR_LONG_LENGTH  = 4,
    JMP_LONG_LENGTH  = 4,
    JMP_SHORT_LENGTH = 4,
    CBR_SHORT_REWRITE_LENGTH = 4, /* FIXME: use this in mangle.c */
    RET_0_LENGTH     = 4,
    PUSH_IMM32_LENGTH = 4,

    /* size of 32-bit call and jmp instructions w/o prefixes. */
    CTI_IND1_LENGTH    = 4, /*  */
    CTI_IND2_LENGTH    = 4, /*  */
    CTI_IND3_LENGTH    = 4, /*  */
    CTI_DIRECT_LENGTH  = 4, /*  */
    CTI_IAT_LENGTH     = 4, /*  */
    CTI_FAR_ABS_LENGTH = 4, /*  */
                            /*  */

    INT_LENGTH = 4,
    SYSCALL_LENGTH = 4,
    SYSENTER_LENGTH = 4,
};


typedef struct _table_stat_state_t
{
	/* need to be filled up */
}table_stat_state_t;

/* All spill slots are grouped in a separate struct because with
 * -no_ibl_table_in_tls, only these slots are mapped to TLS (and the
 * table address/mask pairs are not).
 */
typedef struct _spill_state_t
{
	reg_t t1, t2, t3, t4;
	dcontext_t *dcontext;
}spill_state_t;

typedef struct _local_state_t
{
	spill_state_t spill_space;
}local_state_t;

typedef struct _local_state_extended_t
{
	spill_state_t spill_space;
	table_stat_state_t table_space;
}local_state_extended_t;

#endif
