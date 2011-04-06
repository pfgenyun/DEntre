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

/* global thread-shared var */
bool dentre_initialized = false;

bool post_exec = false;
static uint starttime = 0;

static de_statistics_t nonshared_stats	VAR_IN_SECTION(NEVER_PROTECTED_SECTION)
		={{0},};


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

	return SUCCESS;

}
