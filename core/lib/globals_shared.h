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


#ifndef _GLOBALS_SHARED_H_
#define _GLOBALS_SHARED_H_	1

#include <sys/types.h>

#define MAXINUM_PATH	260

#ifndef DE_DO_NOT_DEFINE_uint
	typedef unsigned int uint;
#endif


#ifdef N64
#	ifndef DE_DO_NOT_DEFINE_uint64
typedef unsigned long int uint64;
#	endif
#	ifndef DE_DO_NOT_DEFINE_int64
typedef long int int64;
#	endif
#else
#	ifndef DE_DO_NOT_DEFINE_uint64
typedef unsigned long long int uint64;
#	endif
#	ifndef DE_DO_NOT_DEFINE_int64
typedef long long int int64;
#	endif
#endif


/* a register value: could be of any type; size is what matters. */
#ifdef N64
typedef uint64 reg_t;
#else
typedef uint reg_t;
#endif
/* integer whose size is based on pointers: ptr diff, mask, etc. */
typedef reg_t ptr_uint_t;
#ifdef N64
typedef int64 ptr_int_t;
#else
typedef int ptr_int_t;
#endif
/* for memory region sizes, use size_t */

typedef pid_t thread_id_t;
typedef pid_t process_id_t;


#ifdef N64
typedef int64 stats_int_t;
#else
typedef int stats_int_t;
#endif


#ifndef NULL
#  define NULL (0)
#endif

#ifndef __cplusplus
#	ifndef DE_DO_NOT_DEFINE_bool
typedef int		bool;
#	endif
#define true	(1)
#define false	(0)
#endif

typedef int file_t;
/** The sentinel value for an invalid file_t. */
#define INVALID_FILE -1


#define TESTALL(mask, var)	((mask) & (var) == (mask))
#define TESTANY(mask, var)	((mask) & (var) != 0 )
#define TEST	TESTANY

#define EXPANDSTR(x)	#x
#define STRINGIFY(x)	EXPANDSTR(x)

#define DENTRE_VAR_RUNUNDER_ID	DENTRE_RUNUNDER

/* FIXME: the environment vars need to be renamed - it will be a pain */
#define DENTRE_HOME_ID			DENTRE_HOME
#define DENTRE_LOGDIR_ID		DENTRE_LOGDIR
#define DENTRE_OPTIONS_ID		DENTRE_OPTIONS
#define DENTRE_AUTOINJECT_ID	DENTRE_AUTOINJECT
#define DENTRE_UNSUPPORTED_ID	DENTRE_UNSUPPORTED
#define DENTRE_RUNUNDER_ID		DENTRE_RUNUNDER
#define DENTRE_CMDLINE_ID		DENTRE_CMDLINE
#define DENTRE_ONCRASH_ID		DENTRE_ONCRASH
#define DENTRE_SAFEMARKER_ID	DENTRE_SAFEMARKER 
/* NT only, value should be all CAPS and specifies a boot option to match */

#define DENTRE_VAR_CACHE_ROOT_ID   DENTRE_CACHE_ROOT
/* we have to create our own properly secured directory, that allows
 * only trusted producers to create DLLs, and all publishers to read
 * them.  Note that per-user directories may also be created by the trusted component
 * allowing users to safely use their own private caches.
 */
#define DENTRE_VAR_CACHE_SHARED_ID   DENTRE_CACHE_SHARED
/* a directory giving full write privileges to Everyone.  Therefore
 * none of its contents can be trusted without explicit verification.
 * Expected to be a subdirectory of DENTRE_CACHE_ROOT.
 */

#define DENTRE_VAR_HOME			STRINGIFY(DENTRE_VAR_HOME_ID)
#define DENTRE_VAR_LOGDIR		STRINGIFY(DENTRE_VAR_LOGDIR_ID)
#define DENTRE_VAR_OPTIONS		STRINGIFY(DENTRE_VAR_OPTIONS_ID)
#define DENTRE_VAR_AUTOINJECT	STRINGIFY(DENTRE_VAR_AUTOINJECT_ID)
#define DENTRE_VAR_UNSUPPORTED	STRINGIFY(DENTRE_VAR_UNSUPPORTED_ID)
#define DENTRE_VAR_RUNUNDER		STRINGIFY(DENTRE_VAR_RUNUNDER_ID)
#define DENTRE_VAR_CMDLINE		STRINGIFY(DENTRE_VAR_CMDLINE_ID)
#define DENTRE_VAR_ONCRASH		STRINGIFY(DENTRE_VAR_ONCRASH_ID)
#define DENTRE_VAR_SAFEMARKER	STRINGIFY(DENTRE_VAR_SAFEMARKER_ID)
#define DENTRE_VAR_CACHE_ROOT	STRINGIFY(DENTRE_VAR_CACHE_ROOT_ID)
#define DENTRE_VAR_CACHE_SHARED	STRINGIFY(DENTRE_VAR_CACHE_SHARED_ID)


#  define DENTRE_VAR_EXECVE  "DENTRE_POST_EXECVE"
#  define DENTRE_VAR_EXECVE_LOGDIR  "DENTRE_EXECVE_LOGDIR"

enum {
    /* FIXME: keep in mind that we only read decimal values */
    RUNUNDER_OFF                  = 0x00,   /* 0 */
    RUNUNDER_ON                   = 0x01,   /* 1 */
    RUNUNDER_ALL                  = 0x02,   /* 2 */

    /* Dummy field to keep track of which processes were RUNUNDER_EXPLICIT
     * before we moved to -follow_systemwide by default (for -early_injection)
     * (note this was the old RUNUNDER_EXPLICIT value) */
    RUNUNDER_FORMERLY_EXPLICIT    = 0x04,   /* 4 */

    RUNUNDER_COMMANDLINE_MATCH    = 0x08,   /* 8 */
    RUNUNDER_COMMANDLINE_DISPATCH = 0x10,  /* 16 */
    RUNUNDER_COMMANDLINE_NO_STRIP = 0x20,  /* 32 */
    RUNUNDER_ONCE                 = 0x40,  /* 64 */

    RUNUNDER_EXPLICIT             = 0x80,  /* 128 */
};


/* convenience macros for secure string buffer operations */
#define BUFFER_SIZE_BYTES(buf)      sizeof(buf)
#define BUFFER_SIZE_ELEMENTS(buf)   (BUFFER_SIZE_BYTES(buf) / sizeof(buf[0]))
#define BUFFER_LAST_ELEMENT(buf)    buf[BUFFER_SIZE_ELEMENTS(buf) - 1]
#define NULL_TERMINATE_BUFFER(buf)  BUFFER_LAST_ELEMENT(buf) = 0


/* Maximum length of any registry parameter. Note that some are further
 * restricted to MAXIMUM_PATH from their usage. */
#define MAX_REGISTRY_PARAMETER	512

#define MAX_OPTIONS_STRING		512

# define IF_WINDOWS(x)
# define IF_WINDOWS_(x)
# define _IF_WINDOWS(x)
# define IF_WINDOWS_ELSE_0(x) (0)
# define IF_WINDOWS_ELSE(x,y) (y)
# define IF_WINDOWS_ELSE_NP(x,y) y
# define IF_LINUX(x) x
# define IF_LINUX_ELSE(x,y) x
# define IF_LINUX_(x) x,
# define _IF_LINUX(x) , x

#endif
