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

#ifndef _DE_STATS_H
#define _DE_STATS_H	1

#include "../globals.h"

#define DENTRE_MAGIC_STRING_LEN	16
#define DENTRE_MAGIC_STREING	"DENTRE_MAGIC_STRING"

#define STAT_NAME_MAX_LEN	50
typedef struct _single_stat_t
{
	char name[STAT_NAME_MAX_LEN];
	stats_int_t value;
}single_stat_t;

/* Parameter and statistics exported by DE via demarker.
 * There should be treated as read-only except for the log_ fields.
 * Unless otherwise mentioned, these stats art all process-wide.
 */
#define NUM_EVENTS	27
typedef struct _de_statistics_t
{
	char magicstring[DENTRE_MAGIC_STRING_LEN];
	process_id_t process_id;
	char process_name[MAXINUM_PATH];
	uint logmask;
	uint loglevel;
	char logdir[MAXINUM_PATH];
	uint prefctr_vals[NUM_EVENTS];		/* what's this */
	uint num_stats;

#ifdef NOT_DENTRE_CORE
	single_stat_t stats[1];
#else
#	ifdef DEBUG
#		define STATS_DEF(desc, name)	single_stat_t name##_pair;
#	else
#		define RSTATS_DEF(desc, name)	single_stat_t name##_pair;
#	endif
#	include "statsx.h"
#	undef STATS_DEF
#	undef RSTATS_DEF
#endif
} de_statistics_t;

#endif
