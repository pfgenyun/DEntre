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

#ifndef PARAMS_IN_REGISTRY

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"

/* no logfile is set up yet */
/* here we set VERBOSE to infolevel */
#if defined(DEBUG) && defined(INTERNAL)
# define VERBOSE 1
/*# define VERBOSE 0  pengfei */
#else
# define VERBOSE 0
#endif

#if defined(NOT_DENTRE_CORE) || defined(NOT_DENTRE_CORE_PROPER)
/* for linking into linux preload we do use libc for now (xref i#46/PR 206369) */
# undef ASSERT
# undef ASSERT_NOT_IMPLEMENTED
# undef ASSERT_NOT_TESTED
# undef ASSERT_NOT_REACHED
# undef FATAL_USAGE_ERROR
# define ASSERT(x) /* nothing */
# define ASSERT_NOT_IMPLEMENTED(x) /* nothing */
# define ASSERT_NOT_TESTED(x) /* nothing */
# define ASSERT_NOT_REACHED() /* nothing */
# define FATAL_USAGE_ERROR(x, ...) /* nothing */
# define print_file(...) fprintf(__VA_ARGS__)
# undef STDERR
# define STDERR stderr
# undef our_snprintf
# define our_snprintf snprintf
# undef DECLARE_NEVERPROT_VAR
# define DECLARE_NEVERPROT_VAR(var, val) var = (val)
#endif

#ifdef DEBUG
DECLARE_NEVERPROT_VAR(static int infolevel, VERBOSE);
#	define INFO(level, fmt, ...)	\
		do \
		{	\
			if(infolevel >= (level))	\
				printf_file(STDERR, "<"fmt">\n", __VA_ARGS__);	\
		}while (0)         /* should not with ; */
#else
#	define INFO(level, fmt, ...) 
#endif


#define CFG_SFX_O32	"configO32"
#define CFG_SFX_N32	"configN32"
#define CFG_SFX_N64	"configN64"

#ifdef O32
#	define CFG_SFX	CFG_SFX_O32
#endif
#ifdef N32
#	define CFG_SFX	CFG_SFX_N32
#endif
#ifdef N64
#	define CFG_SFX	CFG_SFX_N64
#endif

# define GLOBAL_CONFIG_DIR "/etc/dentre"
# define LOCAL_CONFIG_ENV "HOME"
# define LOCAL_CONFIG_SUBDIR ".dentre"
# define GLOBAL_CONFIG_SUBDIR ""

/* we store values for each of these vars: */
static const char *const config_var[] = {
	DENTRE_VAR_HOME,
	DENTRE_VAR_LOGDIR,
	DENTRE_VAR_OPTIONS,
	DENTRE_VAR_AUTOINJECT,
	DENTRE_VAR_UNSUPPORTED,
	DENTRE_VAR_RUNUNDER,
	DENTRE_VAR_CMDLINE,
	DENTRE_VAR_ONCRASH,
	DENTRE_VAR_SAFEMARKER,
	DENTRE_VAR_CACHE_ROOT,
	DENTRE_VAR_CACHE_SHARED,
};

#define NUM_CONFIG_VAR	(sizeof(config_var)/sizeof(config_var[0]))

typedef struct _config_val_t
{
	char val[MAX_REGISTRY_PARAMETER];
	bool has_value;
	bool app_specific;
	bool from_env;
}config_val_t;

typedef struct _config_vals_t
{
	config_val_t vals[NUM_CONFIG_VAR];
	bool has_lconfig;
}config_vals_t;

typedef struct _config_info_t
{
	char fname_app[MAXINUM_PATH];
	char fname_default[MAXINUM_PATH];

    /* Two modes: if query != NULL, we search for var with name==query
     * and will fill in answer.  if query == NULL, then we fill in v.
     * Perhaps it would be worth the complexity to move fname_* to
     * config_vals_t and reduce stack space of config_info_t (can't
     * use heap very easily since used by preinject, injector, and DR).
     */
    const char *query;
	union
	{
		struct 
		{
			config_val_t answer;
			bool have_answer;
		}q;
		config_vals_t *v;
	}u;

}config_info_t;

static config_vals_t myvals;
static config_info_t config;

const char *
get_config_val_ex(const char *var, bool *app_specific, bool *from_env)
{
    uint i;
    config_info_t *cfg = &config;
    ASSERT(var != NULL);
    ASSERT(cfg->u.v != NULL);
    /* perf: we could stick the names in a hashtable */
    for (i = 0; i < NUM_CONFIG_VAR; i++) {
        if (strstr(var, config_var[i]) == var) {
            if (cfg->u.v->vals[i].has_value) {
                if (app_specific != NULL)
                    *app_specific = cfg->u.v->vals[i].app_specific;
                if (from_env != NULL)
                    *from_env = cfg->u.v->vals[i].from_env;
                return (const char *) cfg->u.v->vals[i].val;
            } else
                return NULL;
        }
    }
    return NULL;
}

const char *
get_config_val(const char *var)
{
    return get_config_val_ex(var, NULL, NULL);
}

static void
set_config_from_env(config_info_t *cfg)
{
    uint i;
    char buf[MAX_REGISTRY_PARAMETER];
    /* perf: we could stick the names in a hashtable */
    for (i = 0; i < NUM_CONFIG_VAR; i++) {
        /* env vars set any unset vars (lower priority than config file) */
        if (!cfg->u.v->vals[i].has_value) {
            const char *env = getenv(config_var[i]);
            if (env != NULL) {
                strncpy(cfg->u.v->vals[i].val, env,
                        BUFFER_SIZE_ELEMENTS(cfg->u.v->vals[i].val));
                cfg->u.v->vals[i].has_value = true;
                cfg->u.v->vals[i].app_specific = false;
                cfg->u.v->vals[i].from_env = true;
                INFO(1, "setting %s from env: \"%s\"", config_var[i], env);
            }
        }
    }
}
 
static void
process_config_line(char *line, config_info_t *cfg, bool app_specific, bool overwrite)
{
    uint i;
    ASSERT(line != NULL);
    if (cfg->query != NULL) {
        if (strstr(line, cfg->query) == line) {
            char *val = strchr(line, '=');
            if (val != NULL &&
                /* see comment below */
                val == line + strlen(cfg->query)) {
                strncpy(cfg->u.q.answer.val, val + 1,
                        BUFFER_SIZE_ELEMENTS(cfg->u.q.answer.val));
                cfg->u.q.answer.app_specific = app_specific;
                cfg->u.q.answer.from_env = false;
                cfg->u.q.have_answer = true;
            }
        }
        return;
    }
    /* perf: we could stick the names in a hashtable */
    for (i = 0; i < NUM_CONFIG_VAR; i++) {
        if (strstr(line, config_var[i]) == line) {
            char *val = strchr(line, '=');
            if (val == NULL ||
                /* we don't have any vars that are prefixes of others so we
                 * can do a hard match on whole var.
                 * for parsing simplicity we don't allow whitespace before '='.
                 */
                val != line + strlen(config_var[i])) {
                if (cfg->query == NULL) { /* only complain about this process */
                    FATAL_USAGE_ERROR(ERROR_CONFIG_FILE_INVALID, 3, line);
                    ASSERT_NOT_REACHED();
                }
            } else if (!cfg->u.v->vals[i].has_value || overwrite) {
                strncpy(cfg->u.v->vals[i].val, val + 1,
                        BUFFER_SIZE_ELEMENTS(cfg->u.v->vals[i].val));
                cfg->u.v->vals[i].has_value = true;
                cfg->u.v->vals[i].app_specific = app_specific;
                cfg->u.v->vals[i].from_env = false;
                INFO(1, "setting %s from file: \"%s\"", config_var[i],
                     cfg->u.v->vals[i].val);
            }
        }
    }
}

static void
read_config_file(file_t f, config_info_t *cfg, bool app_specific, bool overwrite)
{
#define BUFSIZE	(MAX_REGISTRY_PARAMETER+128)
	char buf[BUFSIZE];
	char *line, *newline=NULL;
	ssize_t bufread=0, bufwant;
	ssize_t len;


    while (true) {
        /* break file into lines */
        if (newline == NULL) {
            bufwant = BUFSIZE-1;
            bufread = os_read(f, buf, bufwant);
            ASSERT(bufread <= bufwant);
            if (bufread <= 0)
                break;
            buf[bufread] = '\0';
            newline = strchr(buf, '\n');
            line = buf;
        } else {
            line = newline + 1;
            newline = strchr(line, '\n');
            if (newline == NULL) {
                /* shift 1st part of line to start of buf, then read in rest */
                /* the memory for the processed part can be reused  */
                bufwant = line - buf;
                ASSERT(bufwant <= bufread);
                len = bufread - bufwant; /* what is left from last time */
                /* using memmove since strings can overlap */
                if (len > 0)
                    memmove(buf, line, len);
                bufread = os_read(f, buf+len, bufwant);
                ASSERT(bufread <= bufwant);
                if (bufread <= 0)
                    break;
                bufread += len; /* bufread is total in buf */
                buf[bufread] = '\0';
                newline = strchr(buf, '\n');
                line = buf;
            }
        }
        /* buffer is big enough to hold at least one line */
        ASSERT(newline != NULL);
        *newline = '\0';
        /* handle \r\n line endings */
        if (newline > line && *(newline-1) == '\r')
            *(newline-1) = '\0';
        /* now we have one line */
        INFO(1, "config file line: \"%s\"", line);
        /* we support blank lines and comments */
        if (line[0] == '\0' || line[0] == '#')
            continue;
        process_config_line(line, cfg, app_specific, overwrite);
        if (cfg->query != NULL && cfg->u.q.have_answer)
            break;
    }
}

static void
config_read(config_info_t *cfg, const char *appname_in, process_id_t pid, const char *sfx)
{
	file_t f_app = INVALID_FILE;
	file_t f_default = INVALID_FILE;
	const char *local, *global;
	const char *appname = appname_in;
	char buf[MAXINUM_PATH];
	bool check_global = true;

	ASSERT(cfg->query != NULL || cfg->u.v != NULL);

	if(appname == NULL)
		appname = get_application_short_name();

	local = getenv(LOCAL_CONFIG_ENV);

	if(local != NULL)
	{
		process_id_t pid_to_check = pid;
		if(pid == 0 && cfg == &config)
			pid_to_check = get_process_id();
		if(pid_to_check != 0)
		{
			
            /* 1) <local>/appname.<pid>.1config
             * only makes sense for main config for this process
             */
            snprintf(cfg->fname_app, BUFFER_SIZE_ELEMENTS(cfg->fname_app),
					"%s/%s/%s.%d.l%s",
					local, LOCAL_CONFIG_SUBDIR, appname, pid_to_check, sfx);
			NULL_TERMINATE_BUFFER(cfg->fname_app);
			INFO(1, "trying config file %s", cfg->fname_app);
			f_app = os_open(cfg->fname_app, OS_OPEN_READ);
			if(f_app != INVALID_FILE && cfg->query == NULL)
				cfg->u.v->has_lconfig = true;
		}
        /* 2) <local>/appname.config */
        if (f_app == INVALID_FILE) 
		{
            snprintf(cfg->fname_app, BUFFER_SIZE_ELEMENTS(cfg->fname_app), "%s/%s/%s.%s",
                     local, LOCAL_CONFIG_SUBDIR, appname, sfx);
            NULL_TERMINATE_BUFFER(cfg->fname_app);
            INFO(1, "trying config file %s", cfg->fname_app);
            f_app = os_open(cfg->fname_app, OS_OPEN_READ);
        }
        /* 3) <local>/default.0config */
        if (f_default == INVALID_FILE) 
		{
            snprintf(cfg->fname_default, BUFFER_SIZE_ELEMENTS(cfg->fname_default),
                     "%s/%s/default.0%s",
                     local, LOCAL_CONFIG_SUBDIR, sfx);
            NULL_TERMINATE_BUFFER(cfg->fname_default);
            INFO(1, "trying config file %s", cfg->fname_default);
            f_default = os_open(cfg->fname_default, OS_OPEN_READ);
        }
	}

	global = GLOBAL_CONFIG_DIR;

	if(check_global)
	{
		/* 4) <global>/appname.config */
		if(f_app == INVALID_FILE)
		{
			snprintf(cfg->fname_app, BUFFER_SIZE_ELEMENTS(cfg->fname_app),
				"%s%s/%s.%s",
				global, GLOBAL_CONFIG_SUBDIR, appname, sfx);
				NULL_TERMINATE_BUFFER(cfg->fname_app);
				INFO(2, "trying config file %s", cfg->fname_app);
				f_app = os_open(cfg->fname_app, OS_OPEN_READ);
		}
        /* 5) <global>/default.0config */
        if (f_default == INVALID_FILE) 
		{
            snprintf(cfg->fname_default, BUFFER_SIZE_ELEMENTS(cfg->fname_default),
                     "%s%s/default.0%s", global, GLOBAL_CONFIG_SUBDIR, sfx);
            NULL_TERMINATE_BUFFER(cfg->fname_default);
            INFO(2, "trying config file %s", cfg->fname_default);
            f_default = os_open(cfg->fname_default, OS_OPEN_READ);
        }
    }

    if (f_app != INVALID_FILE) {
        INFO(1, "reading app config file %s", cfg->fname_app);
        read_config_file(f_app, cfg, true, false);
        os_close(f_app);
    } else
        INFO(1, "WARNING: no app config file found%s", "");
    if (f_default != INVALID_FILE) {
        INFO(1, "reading default config file %s", cfg->fname_default);
        read_config_file(f_default, cfg, false, false);
        os_close(f_default);
    } else
        INFO(1, "no default config file found%s", "");
    
	/* 6) env vars fill in any still-unset values */
    if (appname_in == NULL) /* only consider env for cur process */
        set_config_from_env(cfg);
}

void 
config_init(void)
{
	config.u.v = &myvals;
	config_read(&config, NULL, 0, CFG_SFX);
}

void
config_reread(void)
{
	/* need to filled up */
}

int 
get_paramenter_ex(const char * name, char *value, int maxlen, bool ignore_cache)
{
	const char *val;

	if(ignore_cache)
		config_reread();

	val = get_config_val(name);
	if(val != NULL)
	{
		strncpy(value, val, maxlen-1);
		value[maxlen] = '\0';
		return GET_PARAMETER_SUCCESS;
	}
	
	return GET_PARAMETER_FAILURE;
}

int
get_parameter(const char *name, char *value, int maxlen)
{
	return get_parameter_ex(name, value, maxlen, false);
}

#else /* !PARAMS_IN_REGISTRY around whole file */

void
config_init()
{
}

void
config_exit()
{
}


#endif
