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
#include "fcache.h"
#include "dispatch.h"

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


DECLARE_FREQPROT_VAR(static uint threads_ever_count, 0);

DECLARE_CXTSWPROT_VAR(mutex_t all_threads_lock, INIT_LOCK_FREE(all_threads_lock));

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

	/* we need to special-case the 1st thread */
	signal_thread_inherit(get_thread_private_dcontext(), NULL);


	/* We move vm_areas_init() below dynamo_thread_init() so we can have
	 * two things: 1) a dcontext and 2) a SIGSEGV handler, for TRY/EXCEPT
	 * inside vm_areas_init() for PR 361594's probes and for safe_read().
	 * This means vm_areas_thread_init() runs before vm_areas_init().
	 */
	if (!DENTRE_OPTION(thin_client)) 
	{
        vm_areas_init();
#ifdef RCT_IND_BRANCH
	/* need to be filled up */
#endif
    } 
	else 
	{
		/* This is needed to handle exceptions in thin_client mode, mostly
		 * internal ones, but can be app ones too. */
		dentre_vm_areas_lock();
		find_dentre_library_vm_areas();
		dentre_vm_areas_unlock();
	}


#ifdef CLIENT_INTERFACE
	/* client last, in case it depends on other inits: must be after
	 * dynamo_thread_init so the client can use a dcontext (PR 216936).
	 * Note that we *load* the client library before installing our hooks,
	 * but call the client's init routine afterward so that we correctly
	 * report crashes (PR 200207).
	 * Note: DllMain in client libraries can crash and we still won't
	 *       report; better document that client libraries shouldn't have
	 *       DllMain.
	 */
    instrument_init();
#endif

	/* need to be filled up */

	/* this thread is now entering DR */
	ENTERING_DE();

	/* need to be filled up */

	dentre_initialized = true;

	/* need to be filled up */

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



/* This routine is called not only at thread initialization,
 * but for every callback, etc. that gets a fresh execution
 * environment!
 */
void 
initialize_dentre_context(dcontext_t *dcontext)
{
	
    /* we can't just zero out the whole thing b/c we have persistent state
     * (fields kept across callbacks, like dstack, module-private fields, next & prev, etc.)
     */
	memset(dcontext->upcontext_ptr, 0, sizeof(unprotected_context_t));
	dcontext->initialized = true;
	dcontext->whereami = WHERE_APP;
	dcontext->next_tag = NULL;
	dcontext->native_exec_retval = NULL;
	dcontext->native_exec_retloc = NULL;
	dcontext->native_exec_postsyscall = NULL;
#ifdef RETURN_STACK
	/* need to be filled up */
#endif

#ifdef N64
	/* need to be filled up */
#endif

	dcontext->sys_param0 = 0;
	dcontext->sys_param1 = 0;
	dcontext->sys_param2 = 0;

	dcontext->signals_pending = false;

	set_last_exit(dcontext, (linkstub_t *) get_starting_linkstub);

#ifdef NATIVE_RETURN
	/* need to be filled up */
#endif

#ifdef PROFILE_RDTSC
	/* need to be filled up */
#endif

#ifdef DEBUG
	/* need to be filled up */
#endif

#ifdef HOT_PATCHING_INTERFACE 
	/* need to be filled up */
#endif

    /* We don't need to initialize dcontext->coarse_exit as it is only
     * read when last_exit indicates a coarse exit, which sets the fields.
     */
}


void add_thread(process_id_t pid, thread_id_t tid, 
				bool under_dentre_control, dcontext_t *dcontext)
{
	thread_record_t *tr;
	uint hindex;

	ASSERT(all_threads != NULL);

	/* add entry to thread hashtable */
	tr = (thread_record_t *)global_heap_alloc(sizeof(thread_record_t)
											HEAPACCT(ACCT_THREAD_MGT));

	tr->pid = pid;
	tr->execve = false;
	tr->id = tid;
	ASSERT(tid != INVALID_THREAD_ID); /* ensure os never assigns invalid id to a thread */
	tr->under_dentre_control = under_dentre_control;
	tr->dcontext = dcontext;
	if(dcontext != NULL)
		dcontext->thread_record = tr;

	mutex_lock(&all_threads_lock);
	tr->num = threads_ever_count++;
	hindex = HASH_FUNC_BITS(tr->id, ALL_THREADS_HASH_BITS);
	tr->next = all_threads[hindex];
	all_threads[hindex] = tr;
	/* must be inside all_threads_lock to avoid race w/ get_list_of_threads */
//    RSTATS_ADD_PEAK(num_threads, 1);
//    RSTATS_INC(num_threads_created);
	num_known_threads++;
	mutex_unlock(&all_threads_lock);
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
	initialize_dentre_context(dcontext);
	set_thread_private_dcontext(dcontext);

    /* sanity check */
    ASSERT(get_thread_private_dcontext() == dcontext);

	/* set local state pointer for access from other threads */
	dcontext->local_state = get_local_state();

    /* For hotp_only, the thread should run native, not under dr.  However,
     * the core should still get control of the thread at hook points to track 
     * what the application is doing & at patched points to execute hot patches.
     * It is the same for thin_client except that there are fewer hooks, only to
     * follow children.
     */
    if (RUNNING_WITHOUT_CODE_CACHE())
        under_dentre_control = false;

    /* add entry to thread hashtable before creating logdir so have thread num.
     * otherwise we'd like to do this only after we'd fully initialized the thread, but we
     * hold the thread_initexit_lock, so nobody should be listing us -- thread_lookup
     * on other than self, or a thread list, should only be done while the initexit_lock
     * is held.  CHECK: is this always correct?  thread_lookup does have an assert
     * to try and enforce but cannot tell who has the lock.
     */
	add_thread(get_process_id(), get_thread_id(), under_dentre_control, dcontext);

	/* need to be filled up */

#ifdef DEBUG
	/* need to be filled up */
#endif

#ifdef DEADLOCK_AVOIDANCE
	/* need to be filled up */
#endif

#ifdef KSTATS
	/* need to be filled up */
#endif

	os_thread_init(dcontext);
	arch_thread_init(dcontext);
	synch_thread_init(dcontext);

	if(!DENTRE_OPTION(thin_client))
		vm_areas_thread_init(dcontext);

	monitor_thread_init(dcontext);
	fcache_thread_init(dcontext);
	link_thread_init(dcontext);
	fragment_thread_init(dcontext);


    if (!DENTRE_OPTION(thin_client)) {
#ifdef CLIENT_INTERFACE
		/* put client last, may depend on other thread inits.
		 * Note that we are calling this prior to instrument_init()
		 * now (PR 216936), which is required to initialize
		 * the client dcontext field prior to instrument_init().
		 */
		instrument_thread_init(dcontext, client_thread);
#endif

#ifdef SIDELINE
		/* need to be filled up */
#endif
    }

	/* need to be filled up */

	return SUCCESS;
}


/* enter/exit DE hooks */
void
entering_dentre(void)
{
	/* need to be filled up */
}

void
exiting_dentre(void)
{
	/* need to be filled up */
}


