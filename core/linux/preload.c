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
#include <stdlib.h>		/* getenv atoi */
#include <string.h>		/* strstr strcmp */
#include "../config.h"

#define START_DENTRE	1
#define VERBOSE_INIT_FINI	0
#define VERBOSE	1
#define INIT_BEFORE_LIBC	0

#if VERBOSE
#define pf(fmt, args...)	printf(fmt, ## args)
#else
#define pf(...) 
#endif

#if START_DENTRE
const char *get_application_short_name(void);
int dentre_app_init(void);
int dentre_app_take_over(void);
#endif


static bool
take_over(const char *pname)
{
	char *plist;
	const char *runstr;
	int rununder;
	bool app_specific, from_env;
#ifdef INTERNAL
	if(strcmp(pname, "texec")==0)
	{
		pf("running texec, NOT take over!\n");
		return false;
	}
#endif

	if(pname[0] == '\0')
		return true;

	config_init();

	runstr = get_config_val_ex(DENTRE_VAR_RUNUNDER, &app_specific, &from_env);
	if(runstr==NULL || runstr[0]=='\0')
		return false;

	rununder = atoi(runstr);
	if(!app_specific && !from_env)
	{
		if(TEST(RUNUNDER_ALL, rununder))
			return true;
		else
			return false;
	}
	if(!TEST(RUNUNDER_ON, rununder))
		return false;
	
	plist = getenv("DENTRE_INCLUDE");
	if(plist != NULL)
		return strstr(plist, pname) ? true : false;

	plist = getenv("DENTRE_EXCLUDE");
	if(plist != NULL)
		return strstr(plist, pname) ? false : true;

	return true;
}

int 
#if INIT_BEFORE_LIBC
_init(int argc, char *arg0, ...)
{
	char **argv = &arg0;
	char **envp = &argv[argc + 1];
#else
_init()
{
#endif
	int init;
	const char *name;
#if VERBOSE_INIT_FINI
	fprintf(stderr, "preload initialized\n");
#endif

#if INIT_BEFORE_LIBC
	{
		int i;
		for(i=1; i<argc; i++)
			fprintf(stderr, "\targ %d = %s\n", i, argv[i]);
		fprintf(stderr, "env 0 is %s\n", env[0]);
		fprintf(stderr, "env 1 is %s\n", env[1]);
		fprintf(stderr, "env 2 is %s\n", env[2]);
	}
#endif

#if START_DENTRE
	pf("ready to start dentre\n");
	name = get_application_short_name();
	pf("preload _init: running %s\n", name);
	if(!take_over(name))
		return 0;

	init = dentre_app_init();
	pf("dentre_app_init return %d\n", init);
	dentre_app_take_over();
	pf("dentre started\n");
#endif

	return 0;
}

int
_fini()
{
#if VERBOSE_INIT_FINI
	fprintf(stderr, "preload finalized\n");
#endif

	return 0;
}

