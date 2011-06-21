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

/* global thread-shared var */
bool dentre_initialized = false;
bool dentre_heap_initialized = false;

bool dentre_exited = false;


bool post_exec = false;
static uint starttime = 0;

DECLARE_FREQPROT_VAR(static int num_known_threads, 0);
/*vfork threads that execve need to be separately delay-freed */
DECLARE_FREQPROT_VAR(static int num_execve_threads, 0);


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
	int size;

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
