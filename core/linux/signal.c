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

#include <stdio.h>

#include "../globals.h"
#include "../utils.h"
#include "../heap.h"

#include <string.h>

typedef struct _thread_sig_info_t
{
	/* need to be filled up */
} thread_sig_info_t;

static bool
os_itimers_thread_shared()
{
	static bool itimers_shared;
	static bool cached = false;

    if (!cached) {
        file_t f = os_open("/proc/version", OS_OPEN_READ);
        if (f != INVALID_FILE) {
            char buf[128];
            int major, minor, rel;
            os_read(f, buf, BUFFER_SIZE_ELEMENTS(buf));
            NULL_TERMINATE_BUFFER(buf);
            if (sscanf(buf, "%*s %*s %d.%d.%d", &major, &minor, &rel) == 3) {
                /* Linux NPTL in kernel 2.6.12+ has POSIX-style itimers shared
                 * among threads.
                 */
                LOG(GLOBAL, LOG_ASYNCH, 1, "kernel version = %d.%d.%d\n",
                    major, minor, rel);
                itimers_shared = (major >= 2 && minor >= 6 && rel >= 12);
                cached = true;
            }
        }
        if (!cached) {
            /* assume not shared */
            itimers_shared = false;
            cached = true;
        }
        LOG(GLOBAL, LOG_ASYNCH, 1, "itimers are %s\n",
            itimers_shared ? "thread-shared" : "thread-private");
    }
    return itimers_shared;
}

void
signal_init()
{
	IF_N64(ASSERT(ALIGNED(offsetof(sigpending_t, fpstate), 16)));
	os_itimers_thread_shared();
}


void
signal_thread_init(dcontext_t *dcontext)
{
#ifdef HAVE_SIGALTSTACK
	int rc;
#endif

	thread_sig_info_t *info = HEAP_TYPE_ALLOC(dcontext, thread_sig_info_t,
												ACCT_OTHER, PROTECTED);
	dcontext->signal_field = (void *) info;

	/* all fields want to be initialized to 0 */
	memset(info, 0, sizeof(thread_sig_info_t));

	/* need to be filled up */
}
