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

/* global thread-shared var */
bool dentre_initialized = false;

bool dentre_exited = false;


bool post_exec = false;
static uint starttime = 0;

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
