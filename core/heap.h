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

#ifndef _HEAP_H_
#define _HEAP_H_	1

#ifdef HEAP_ACCOUNTING
typedef enum {
    ACCT_FRAGMENT=0,
    ACCT_COARSE_LINK,
    ACCT_FRAG_FUTURE,
    ACCT_FRAG_TABLE,
    ACCT_IBLTABLE,
    ACCT_TRACE,
    ACCT_FCACHE_EMPTY,
    ACCT_VMAREA_MULTI,
    ACCT_IR,
    ACCT_AFTER_CALL,
    ACCT_VMAREAS,
    ACCT_SYMBOLS,
# ifdef SIDELINE
    ACCT_SIDELINE,
# endif
    ACCT_THCOUNTER,
    ACCT_TOMBSTONE, /* N.B.: leaks in this category are not reported;
                     * not currently used */

    ACCT_HOT_PATCHING,
    ACCT_THREAD_MGT,
    ACCT_MEM_MGT,
    ACCT_STATS,
    ACCT_SPECIAL,
# ifdef CLIENT_INTERFACE
    ACCT_CLIENT,
# endif
    ACCT_LIBDUP, /* private copies of system libs => may leak */
    /* NOTE: Also update the whichheap_name in heap.c when adding here */
    ACCT_OTHER,
    ACCT_LAST
} which_heap_t;

# define HEAPACCT(x) , x
# define IF_HEAPACCT_ELSE(x, y) x
#else
# define HEAPACCT(x)
# define IF_HEAPACCT_ELSE(x, y) y
#endif


void vmm_heap_init(void);

#endif
