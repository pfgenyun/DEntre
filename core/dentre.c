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

#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "lib/de_stats.h"
#include "config.h"
#include "options.h"
#include "mips/instrument.h"
#ifdef PAPI
#	include "perfctr.h"
#endif
#include "heap.h"
#include "moduledb.h"
#include "mips/proc.h"
#include "module_shared.h"
#include "mips/arch_exports.h"
#include "synch.h"
#ifdef KSTATS
#	include "stats.h"
#endif
#include "monitor.h"
#include "link.h"
#include "perscache.h"
#include "hotpatch.h"
#include "mips/sideline.h"
#include "mips/proc.h"

/* global thread-shared var */
bool dentre_initialized = false;
bool dentre_heap_initialized = false;

bool dentre_exited = false;


bool post_exec = false;
static uint starttime = 0;

/* initial stack so we don't have to use app's */
byte *  initstack;

DECLARE_FREQPROT_VAR(static int num_known_threads, 0);
/*vfork threads that execve need to be separately delay-freed */
DECLARE_FREQPROT_VAR(static int num_execve_threads, 0);

/* used for synch to prevent thread creation/deletion in critical periods 
 * due to its use for flushing, this lock cannot be held while couldbelinking!
 */
DECLARE_CXTSWPROT_VAR(mutex_t thread_initexit_lock,
                      INIT_LOCK_FREE(thread_initexit_lock));

START_DATA_SECTION(NEVER_PROTECTED_SECTION, "w")

static de_statistics_t nonshared_stats	VAR_IN_SECTION(NEVER_PROTECTED_SECTION)
		={{0},};

/* Each lock protects its corresponding datasec_start, datasec_end, and
 * datasec_writable variables.
 */
static mutex_t datasec_lock[DATASEC_NUM] VAR_IN_SECTION(NEVER_PROTECTED_SECTION) = {{0}};

END_DATA_SECTION()


static app_pc datasec_start[DATASEC_NUM];
static app_pc datasec_end[DATASEC_NUM];

const uint DATASEC_SELFPROT[] = {
    0,
    SELFPROT_DATA_RARE,
    SELFPROT_DATA_FREQ,
    SELFPROT_DATA_CXTSW,
};

const char * const DATASEC_NAMES[] = {
    NEVER_PROTECTED_SECTION,
    RARELY_PROTECTED_SECTION,
    FREQ_PROTECTED_SECTION,
    CXTSW_PROTECTED_SECTION,
};


static void data_section_init();
static void data_section_exit();

de_statistics_t *stats = NULL;

/* FIXME : not static so os.c can hand walk it for dump core */
/* FIXME: use new generic_table_t and generic_hash_* routines */
thread_record_t ** all_threads; /* ALL_THREADS_HASH_BITS-bit addressed hash table */


static void
statistics_pre_init(void)
{
	stats = &nonshared_stats;
	stats->process_id = get_process_id();
	strncpy(stats->process_name, get_application_name(), MAXINUM_PATH);
	stats->process_name[MAXINUM_PATH-1] = '\0';
	ASSERT(strlen(stats->process_name) > 0);
	stats->num_stats = 0;
}


static void
statistics_init(void)
{
    /* should have called statistics_pre_init() first */
    ASSERT(stats == &nonshared_stats);
    ASSERT(stats->num_stats == 0);
#ifndef DEBUG
    if (!DENTRE_OPTION(global_rstats)) {
        /* references to stat values should return 0 (static var) */
        return;
    }
#endif
    stats->num_stats = 0
#ifdef DEBUG
# define STATS_DEF(desc, name) +1
#else
# define RSTATS_DEF(desc, name) +1
#endif
# include "lib/statsx.h"
#undef STATS_DEF
#undef RSTATS_DEF
        ;
    /* We inline the stat description to make it easy for external processes
     * to view our stats: they don't have to chase pointers, and we could put
     * this in shared memory easily.  However, we do waste some memory, but
     * not much in release build.
     */
#ifdef DEBUG
# define STATS_DEF(desc, statname) \
    strncpy(stats->statname##_pair.name, desc, \
            BUFFER_SIZE_ELEMENTS(stats->statname##_pair.name)); \
    NULL_TERMINATE_BUFFER(stats->statname##_pair.name);
#else
# define RSTATS_DEF(desc, statname) \
    strncpy(stats->statname##_pair.name, desc, \
            BUFFER_SIZE_ELEMENTS(stats->statname##_pair.name)); \
    NULL_TERMINATE_BUFFER(stats->statname##_pair.name);
#endif
# include "lib/statsx.h"
#undef STATS_DEF
#undef RSTATS_DEF
}

DENTRE_EXPORT int
dentre_app_init(void)
{

	if(!dentre_initialized)
	{
		DODEBUG(starttime = query_time_seconds(););

		if(getenv(DENTRE_VAR_EXECVE) != NULL)
		{
			post_exec = true;
			unsetenv(DENTRE_VAR_EXECVE);
			ASSERT(getenv(DENTRE_VAR_EXECVE) == NULL);
		}
		else
			post_exec = false;
	}

#ifdef DEBUG
#	ifndef INTERNAL
	nonshared_stats.logmask = LOG_ALL_RELEASE;
#	else
	nonshared_stats.logmask = LOG_ALL;
#	endif
	statistics_pre_init();
#endif

	config_init();
	options_init();
	utils_init();
	data_section_init();

#ifdef DEBUG
	/* need to be filled up */
#	ifdef PAPI
	/* need to be filled up */
	hardware_perfctr_init();
#	endif
#endif

#ifndef DEBUG
	statistics_pre_init();
#endif

	statistics_init();

#ifdef CLIENT_INTERFACE
	instrument_load_client_libs();
#endif

	vmm_heap_init();
	heap_init();
	dentre_heap_initialized = true;

	/* The process start event should be done after os_init() but before
	 * process_control_int() because the former initializes event logging
	 * and the latter can kill the process if a violation occurs.
	 */
	SYSLOG(SYSLOG_INFORMATION,
	       IF_CLIENT_INTERFACE_ELSE(INFO_PROCESS_START_CLIENT, INFO_PROCESS_START),
	       IF_CLIENT_INTERFACE_ELSE(3, 2),
	       get_application_name(), get_application_pid()
	       _IF_NOT_CLIENT_INTERFACE(get_application_md5()));

#ifdef PROCESS_CONTROL
    if(IS_PROCESS_CONTROL_ON())    /* Case 8594. */
        process_control_init();
#endif

	dentre_vm_areas_init();
	proc_init();
	modules_init();		/* before vm_areas_init() */
	os_init();
	arch_init();
	synch_init();

#ifdef KSTATS
	kstat_init();
#endif

	monitor_init();
	fcache_init();
	link_init();
	fragment_init();
	moduledb_init();	/* before vm_areas_init, after heap_init */
	perscache_init();	/* before vm_areas_init */

    if (!DENTRE_OPTION(thin_client)) {
#ifdef HOT_PATCHING_INTERFACE
        /* must init hotp before vm_areas_init() calls find_executable_vm_areas() */
        if (DENTRE_OPTION(hot_patching))
            hotp_init();
#endif
    }

#ifdef INTERNAL
/* need to be filled up */
#endif

	/* initial stack so we don't have to use app's 
	 * N.B.: we never de-allocate initstack (see comments in app_exit)
	 */
	initstack = (byte *) stack_alloc(DENTRE_STACK_SIZE);


	/* initialize thread hashtable */
	/* Note: for thin_client, this isn't needed if it is only going to
	 * look for spawned processes; however, if we plan to promote from 
	 * thin_client to hotp_only mode (highly likely), this would be needed.
	 * For now, leave it in there unless thin_client footprint becomes an 
	 * issue.
	 */

	int size;
	size = HASHTABLE_SIZE(ALL_THREADS_HASH_BITS) * (sizeof(thread_record_t*));
	all_threads = (thread_record_t**)global_heap_alloc(size HEAPACCT(ACCT_HTREAD_MGT));
	memset(all_threads, 0, size);

#ifdef SIDELINE
	/* initialize sideline thread after thread table is set up */
	if(dentre_options.sideline)
		sideline_init();
#endif


	/* thread-specific initialization for the first thread we inject in
	 * (in a race with injected threads, sometimes it is not the primary thread)
	 */
	dentre_thread_init(NULL _IF_CLIENT_INTERFACE(false));

	return SUCCESS;

}

static void
get_data_section_bounds(uint sec)
{
	/* need to be filled up */
}

static void 
data_section_init(void)
{
	uint i;
	for(i=0; i<DATASEC_NUM; i++)
	{
		if(datasec_start[i] != NULL)
		{
			return;
		}
		ASSIGN_INIT_LOCK_FREE(datasec_lock[i], datasec_selfproc_lock);

		if(TEST(DATASEC_SELFPROT[i], dentre_options.protect_mask))
		{
			get_data_section_bounds(i);
		}
	}

    DODEBUG(
	{
        /* ensure no overlaps */
        uint j;
        for (i=0; i<DATASEC_NUM; i++) 
		{
            for (j=i+1; j<DATASEC_NUM; j++) 
			{
                ASSERT(datasec_start[i] >= datasec_end[j] ||
                       datasec_start[j] >= datasec_end[i]);
            }
		}
	});
}


int
get_num_threads(void)
{
	return num_known_threads - num_execve_threads;
}


bool
is_thread_initialized(void)
{
#ifdef HAVE_TLS
	if(get_tls_thread_id() != get_sys_thread_id())
		return false;	
#endif
	return (get_thread_private_dcontext() != NULL);
}

dcontext_t *
create_new_dentre_context(bool initial, byte *dstack_in)
{
	dcontext_t *dcontext;
	size_t alloc = sizeof(dcontext_t) + proc_get_cache_line_size();
	void *alloc_start = (void *)
        ((TEST(SELFPROT_GLOBAL, dentre_options.protect_mask) &&
          !TEST(SELFPROT_DCONTEXT, dentre_options.protect_mask)) ?
         /* if protecting global but not dcontext, put whole thing in unprot mem */
         global_unprotected_heap_alloc(alloc HEAPACCT(ACCT_OTHER)) :
         global_heap_alloc(alloc HEAPACCT(ACCT_OTHER)));
	dcontext = (dcontext_t *) proc_bump_to_end_of_cache_line((ptr_uint_t)alloc_start);

	/* need to be filled up */

    /* Put here all one-time dcontext field initialization 
     * Make sure to update create_callback_dcontext to shared
     * fields across callback dcontexts for the same thread.
     */
    /* must set to 0 so can tell if initialized for callbacks! */
	memset(dcontext, 0x0, sizeof(dcontext_t));
	dcontext->allocated_start = alloc_start;

	/* we share a single dstack across all callbacks */
	if(initial)
	{
		if(dstack_in = NULL)
			dcontext->dstack = (byte *) stack_alloc(DENTRE_STACK_SIZE);
		else
			dcontext->dstack = dstack_in;
	}
	else
	{
		/* dstack may be pre-allocated only at thread init, not at callback */
		ASSERT(dstack_in = NULL);
	}

#ifdef RETURN_STACK
	/* need to be filled up */
#endif
	
	if(TEST(SELFPROT_DCONTEXT, dentre_options.protect_mask))
	{
		dcontext->upcontext.separate_upcontext = 
			global_unprotected_heap_alloc(sizeof(unprotected_context_t) HEAPACCT(ACCT_OTHER));

        LOG(GLOBAL, LOG_TOP, 2, "new dcontext="PFX", dcontext->upcontext="PFX"\n",
            dcontext, dcontext->upcontext.separate_upcontext);
		dcontext->upcontext_ptr = dcontext->upcontext.separate_upcontext;
	}
	else
		dcontext->upcontext_ptr = &(dcontext->upcontext.upcontext);

#ifdef HOT_PATCHING_INTERFACE
	/* need to be filled up */
#endif

	/* need to be filled up */
	dcontext->owning_thread = get_thread_id();
	dcontext->owning_process = get_process_id();

    /* thread_record is set in add_thread */
    /* all of the thread-private fcache and hashtable fields are shared
     * among all dcontext instances of a thread, so the caller must
     * set those fields
     */
    /* rest of dcontext initialization happens in initialize_dynamo_context(),
     * which is executed for each dr_app_start() and each
     * callback start
     */
	return dcontext;
}

/* thread-specific initialization 
 * if dstack_in is NULL, then a dstack is allocated; else dstack_in is used 
 * as the thread's dstack
 * returns -1 if current thread has already been initialized
 */
int
dentre_thread_init(byte *dstack_in _IF_CLIENT_INTERFACE(bool client_thread))
{
	dcontext_t *dcontext;

	bool reset_at_nth_thread_pending = false;
	bool under_dentre_control = true;

    APP_EXPORT_ASSERT(dentre_initialized || dentre_exited ||
                      get_num_threads() == 0 IF_CLIENT_INTERFACE(|| client_thread),
                      PRODUCT_NAME" not initialized");

    if (INTERNAL_OPTION(nullcalls))
        return SUCCESS;

	/* synch point so thread creation can be prevented for critical periods */
	mutex_lock(&thread_initexit_lock);

	while(dentre_exited)
	{
//        DODEBUG_ONCE(LOG(GLOBAL, LOG_THREADS, 1, 
//                         "Thread %d reached initialization point while dentr exiting, "
//                         "waiting for app to exit\n", get_thread_id());); 
        mutex_unlock(&thread_initexit_lock);
		thread_yield();

        /* just in case we want to support exited and then restarted at some 
         * point */
        mutex_lock(&thread_initexit_lock);
	}
	
    if (is_thread_initialized()) 
	{
        mutex_unlock(&thread_initexit_lock);
        return -1;
    }

	os_tls_init();
	dcontext = create_new_dentre_context(true/*initial*/, dstack_in);


}



