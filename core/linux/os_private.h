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

#ifndef _OS_PRIVATE_H_
#define _OS_PRIVATE_H_	1


/* thread-local data that's os-private, for modularity */
typedef struct _os_thread_data_t {
    /* store stack info at thread startup, since stack can get fragmented in
     * /proc/self/maps w/ later mprotects and it can be hard to piece together later
     */
    app_pc stack_base;
    app_pc stack_top;

#ifdef RETURN_AFTER_CALL
    app_pc stack_bottom_pc;     /* return target in the loader at program startup */
#endif

    /* for thread suspension */
    /* This lock synchronizes suspension and resumption and controls
     * access to suspend_count and the bools below in thread_suspend
     * and suspend_resume.  handle_suspend_signal() does not use the
     * mutex as it is not safe to do so, but our suspend and resume
     * synch avoids any need for it there.
     */
    mutex_t suspend_lock;
    int suspend_count;
    /* We would use event_t's here except we can't use mutexes in
     * our signal handler 
     */
    bool suspended;
    bool wakeup;
    bool resumed;
    struct sigcontext *suspended_sigcxt;

    /* PR 297902: for thread termination */
    bool terminate;
    bool terminated;

    /* for re-entrant suspend signals */
    int processing_signal;
} os_thread_data_t;


void signal_init(void);
void signal_thread_init(dcontext_t *dcontext);

#endif
