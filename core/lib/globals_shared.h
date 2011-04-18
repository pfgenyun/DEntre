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

#define MAX_CLIENT_LIBS 16

#ifndef DR_DO_NOT_DEFINE_uint
typedef unsigned int uint;
#endif
#ifndef DR_DO_NOT_DEFINE_ushort
typedef unsigned short ushort;
#endif
#ifndef DR_DO_NOT_DEFINE_byte
typedef unsigned char byte;
#endif
#ifndef DR_DO_NOT_DEFINE_sbyte
typedef signed char sbyte;
#endif
typedef byte * app_pc;

#ifndef DE_DO_NOT_DEFINE_uint
	typedef unsigned int uint32;
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

typedef uint client_id_t;

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


#ifndef IN
# define IN /* marks input param */
#endif
#ifndef OUT
# define OUT /* marks output param */
#endif
#ifndef INOUT
# define INOUT /* marks input+output param */
#endif


#ifdef N64
# define POINTER_MAX ULLONG_MAX
# define SSIZE_T_MAX LLONG_MAX
# define POINTER_MAX_32BIT ((ptr_uint_t)UINT_MAX) 
#else
# define POINTER_MAX UINT_MAX
# define SSIZE_T_MAX INT_MAX
#endif

#define PTR_UINT_0       ((ptr_uint_t)0U)
#define PTR_UINT_1       ((ptr_uint_t)1U)
#define PTR_UINT_MINUS_1 ((ptr_uint_t)-1)


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

#define MAX_LIST_OPTION_LENGTH	512

#define MAX_OPTION_LENGTH		512

#define MAX_PATH_OPTION_LENGTH	512

#ifdef X64
# define IF_X64(x) x
# define IF_X64_ELSE(x, y) x
# define IF_X64_(x) x,
# define _IF_X64(x) , x
# define IF_NOT_X64(x)
# define _IF_NOT_X64(x)
#else
# define IF_X64(x)
# define IF_X64_ELSE(x, y) y
# define IF_X64_(x)
# define _IF_X64(x)
# define IF_NOT_X64(x) x
# define _IF_NOT_X64(x) , x
#endif

#ifdef N64
# define IF_N64(x) x
# define IF_N64_ELSE(x, y) x
# define IF_N64_(x) x,
# define _IF_N64(x) , x
# define IF_NOT_N64(x)
# define _IF_NOT_N64(x)
#else
# define IF_N64(x)
# define IF_N64_ELSE(x, y) y
# define IF_N64_(x)
# define _IF_N64(x)
# define IF_NOT_N64(x) x
# define _IF_NOT_N64(x) , x
#endif

/* DR_API EXPORT END */

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


/* macros to make conditional compilation look prettier */
#ifdef DEBUG
# define IF_DEBUG(x) x
# define _IF_DEBUG(x) , x
# define IF_DEBUG_ELSE(x, y) x
# define IF_DEBUG_ELSE_0(x) (x)
# define IF_DEBUG_ELSE_NULL(x) (x)
#else
# define IF_DEBUG(x)
# define _IF_DEBUG(x)
# define IF_DEBUG_ELSE(x, y) y
# define IF_DEBUG_ELSE_0(x) 0
# define IF_DEBUG_ELSE_NULL(x) (NULL)
#endif


#ifdef INTERNAL
# define IF_INTERNAL(x) x
# define IF_INTERNAL_ELSE(x,y) x
#else
# define IF_INTERNAL(x)
# define IF_INTERNAL_ELSE(x,y) y
#endif


#ifdef CLIENT_INTERFACE
# define IF_CLIENT_INTERFACE(x) x
# define IF_CLIENT_INTERFACE_ELSE(x, y) x
# define _IF_CLIENT_INTERFACE(x) , x
# define _IF_NOT_CLIENT_INTERFACE(x)
/* _IF_CLIENT_INTERFACE is too long */
# define _IF_CLIENT(x) , x
#else
# define IF_CLIENT_INTERFACE(x)
# define IF_CLIENT_INTERFACE_ELSE(x, y) y
# define _IF_CLIENT_INTERFACE(x)
# define _IF_NOT_CLIENT_INTERFACE(x) , x
# define _IF_CLIENT(x)
#endif


#ifdef PROGRAM_SHEPHERDING
# define IF_PROG_SHEP(x) x
#else
# define IF_PROG_SHEP(x)
#endif

#if defined(PROGRAM_SHEPHERDING) && defined(RCT_IND_BRANCH)
# define IF_RCT_IND_BRANCH(x) x
#else
# define IF_RCT_IND_BRANCH(x)
#endif

#if defined(PROGRAM_SHEPHERDING) && defined(RETURN_AFTER_CALL)
# define IF_RETURN_AFTER_CALL(x) x
# define IF_RETURN_AFTER_CALL_ELSE(x, y) x
#else
# define IF_RETURN_AFTER_CALL(x)
# define IF_RETURN_AFTER_CALL_ELSE(x, y) y
#endif

#ifdef HOT_PATCHING_INTERFACE
# define IF_HOTP(x) x
#else
# define IF_HOTP(x)
#endif


#ifdef HAVE_TLS
# define IF_HAVE_TLS_ELSE(x, y) x
# define IF_NOT_HAVE_TLS(x)
#else
# define IF_HAVE_TLS_ELSE(x, y) y
# define IF_NOT_HAVE_TLS(x) x
#endif


#ifdef VMX86_SERVER
# define IF_VMX86(x) x
# define IF_VMX86_ELSE(x,y) x
# define _IF_VMX86(x) , x
# define IF_NOT_VMX86(x) 
#else
# define IF_VMX86(x)
# define IF_VMX86_ELSE(x,y) y
# define _IF_VMX86(x)
# define IF_NOT_VMX86(x) x
#endif

typedef enum {
    SYSLOG_INFORMATION = 0x1,
    SYSLOG_WARNING     = 0x2,
    SYSLOG_ERROR       = 0x4,
    SYSLOG_CRITICAL    = 0x8,
    SYSLOG_NONE        = 0x0,
    SYSLOG_ALL         = SYSLOG_INFORMATION | SYSLOG_WARNING | SYSLOG_ERROR | SYSLOG_CRITICAL
} syslog_event_type_t;

typedef char pathstring_t[MAX_PATH_OPTION_LENGTH];

typedef char liststring_t[MAX_LIST_OPTION_LENGTH];

/* These constants & macros are used by core, share and preinject, so this is
 * the only place they will build for win32 and linux! */
/* return codes for [gs]et_parameter style functions
 * failure == 0 for compatibility with get_parameter()
 * if GET_PARAMETER_NOAPPSPECIFIC is returned, that means the
 *  parameter returned is from the global options, because there
 *  was no app-specific key present.
 * Note: constants shouldn't be added to this enum without checking whether
 *       the macros below will work; they should if errors are all negative and
 *       valid codes are all positive.
 */
enum {
    GET_PARAMETER_BUF_TOO_SMALL = -1,
    GET_PARAMETER_FAILURE = 0,
    GET_PARAMETER_SUCCESS = 1,
    GET_PARAMETER_NOAPPSPECIFIC = 2,
    SET_PARAMETER_FAILURE = GET_PARAMETER_FAILURE,
    SET_PARAMETER_SUCCESS = GET_PARAMETER_SUCCESS
};
#define IS_GET_PARAMETER_FAILURE(x) ((x) <= 0 )
#define IS_GET_PARAMETER_SUCCESS(x) ((x) > 0 )


#define COMPANY_NAME "DynamoRIO" /* used for reg key */
#define COMPANY_NAME_EVENTLOG "DynamoRIO" /* used for event log */
/* used in (c) stmt in log file and in resources */
#define COMPANY_LONG_NAME "DynamoRIO developers"


#define PFX		"0x""%08x"
#define PIFX	"0x""%0x"

/* need to be filled up */
#define DENTRE_OPTION(opt)	dentre_options.opt


#ifdef EXPOSE_INTERNAL_OPTIONS
  /* Use only for experimental non-release options */
  /* Internal option value can be set on the command line only in INTERNAL builds */
  /* We should remove the ASSERT if we convert an internal option into non-internal */
# define INTERNAL_OPTION(opt) ((IS_OPTION_INTERNAL(opt)) ? (DENTRE_OPTION(opt)) : \
                               (ASSERT_MESSAGE("non-internal option argument "#opt, false), \
                                DENTRE_OPTION(opt)))
#else
  /* Use only for experimental non-release options, 
     default value is assumed and command line options are ignored */
  /* We could use IS_OPTION_INTERNAL(opt) ? to determine whether an option is defined as INTERNAL in
     optionsx.h and have that be the only place to modify to transition between internal and external options.
     The compiler should be able to eliminate the inappropriate part of the constant expression,
     if the specific option is no longer defined as internal so we don't have to modify the code.
     Still I'd rather have explicit uses of DYNAMO_OPTION or INTERNAL_OPTION for now.
  */
# define INTERNAL_OPTION(opt) DEFAULT_INTERNAL_OPTION_VALUE(opt)
#endif /* EXPOSE_INTERNAL_OPTIONS */



#endif
