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

#ifndef _OPTIONS_H_
#define _OPTIONS_H_	1

/* Security policy flags.  Note that if the off flag is not explicitly
 * used in an option default value definition, then the option doesn't
 * support that flag.
*/
typedef enum {
    /* security mechanism needed for detection can be completely disabled */
    OPTION_ENABLED     = 0x1,   /* if set, security mechanism is on */
    OPTION_DISABLED    = 0x0,   /* if not set, security mechanism is off  */

    /* security policy can be built on top.  For stateless policies
     * all flags can be dynamic options.  For stateful security mechanisms as
     * long as OPTION_ENABLED stays on for gathering information, the
     * security policies OPTION_BLOCK|OPTION_REPORT can be dynamically
     * set.  If no detection action is set, a detection check may be skipped.
     */

    /* associated security policy can be either blocking or not */
    /* If set, app action (i.e., bad behavior) is disallowed and remediation
     * action (kill thread, kill process, throw exception) is performed.
     * Note: detect_mode will override this flag.
     * FIXME: app_thread_policy_helper seems to be the one place where 
     * detect_mode doesn't override this; case 9088 tracks this; xref case 8451
     * for an explanation of why this seemed to leave as such.
     */
    OPTION_BLOCK       = 0x2,
    OPTION_NO_BLOCK    = 0x0,

    /* policies that lend themselves to standard attack handling may use this flag */
    OPTION_HANDLING    = 0x4,   /* if set, overrides default attack handling */
    OPTION_NO_HANDLING = 0x0,   /* when not set, default attack handling is used */

    /* report or stay silent */
    OPTION_REPORT      = 0x8,   /* if set, report action is being taken */
    OPTION_NO_REPORT   = 0x0,   /* if not set, action is taken silently */

    /* Block ignoring detect_mode; to handle case 10610. */
    OPTION_BLOCK_IGNORE_DETECT  = 0x20,

    /* modifications in security policy or detection mechanism are controlled with */
    OPTION_CUSTOM      = 0x100, /* alternative policy bit - custom meaning per option */
    OPTION_NO_CUSTOM   = 0x0,   /* alternative policy bit - custom meaning per option */
} security_option_t;


/* for all option uses */
/* N.B.: for 64-bit should we make the uint_size options be size_t?
 * For now we're ok with all such options maxing out at 4GB, and in fact
 * some like the fcache options are assumed to not be larger.
 */
#define uint_size uint
#define uint_time uint
/* So far all addr_t are external so we don't have a 64-bit problem */
#define uint_addr ptr_uint_t


/* to dispatch on string default values, kept in struct not enum */
#define ISSTRING_bool 0
#define ISSTRING_uint 0
#define ISSTRING_uint_size 0
#define ISSTRING_uint_time 0
#define ISSTRING_ptr_uint_t 0
#define ISSTRING_pathstring_t 1
#define ISSTRING_liststring_t 1


/* Does this option affect persistent cache formation? */
typedef enum {
    OP_PCACHE_NOP    = 0, /* No effect on pcaches */
    OP_PCACHE_LOCAL  = 1, /* Can only relax (not tighten), and when it relaxes any
                           * module that module is marked via
                           * os_module_set_flag(MODULE_WAS_EXEMPTED).
                           */
    OP_PCACHE_GLOBAL = 2, /* Affects pcaches but not called out as local. */
} op_pcache_t;


#define OPTION_STRING(x) 0 /* no string in enum */
#define EMPTY_STRING     0 /* no string in enum */
#define OPTION_COMMAND(type, name, default_value, command_line_option, statement, \
                       description, flag, pcache)                                 \
    OPTION_DEFAULT_VALUE_##name = (ptr_uint_t) default_value, \
    OPTION_IS_INTERNAL_##name = false, \
    OPTION_IS_STRING_##name = ISSTRING_##type, \
    OPTION_AFFECTS_PCACHE_##name = pcache,
#define OPTION_COMMAND_INTERNAL(type, name, default_value, command_line_option, \
                                statement, description, flag, pcache)           \
    OPTION_DEFAULT_VALUE_##name = (ptr_uint_t) default_value, \
    OPTION_IS_INTERNAL_##name = true, \
    OPTION_IS_STRING_##name = ISSTRING_##type, \
    OPTION_AFFECTS_PCACHE_##name = pcache,
enum option_is_internal {
#   include "optionsx.h"
};
#undef OPTION_COMMAND
#undef OPTION_COMMAND_INTERNAL
#undef OPTION_STRING
#undef EMPTY_STRING


#define OPTION_STRING(x) x /* no string in enum */
#define EMPTY_STRING    {0} /* no string in enum */

/* the Option struct typedef */
#ifdef EXPOSE_INTERNAL_OPTIONS
#  define OPTION_COMMAND_INTERNAL(type, name, default_value, command_line_option, \
                                  statement, description, flag, pcache)           \
    type name;
#else 
#  define OPTION_COMMAND_INTERNAL(type, name, default_value, command_line_option, \
                                  statement, description, flag, pcache) /* nothing */
#endif

#define OPTION_COMMAND(type, name, default_value, command_line_option, statement, \
                       description, flag, pcache)                                 \
    type name;
typedef struct _options_t {
#   include "optionsx.h"
} options_t;

#undef OPTION_COMMAND
#undef OPTION_COMMAND_INTERNAL


#ifndef EXPOSE_INTERNAL_OPTIONS
/* special struct for internal option default values */
#  define OPTION_COMMAND(type, name, default_value, command_line_option, statement, \
                         description, flag, pcache) /* nothing */
#  define OPTION_COMMAND_INTERNAL(type, name, default_value, command_line_option, \
                                  statement, description, flag, pcache)           \
    const type name;
typedef struct _internal_options_t {
#   include "optionsx.h"
} internal_options_t;
#  undef OPTION_COMMAND
#  undef OPTION_COMMAND_INTERNAL
#endif

#undef uint_size
#undef uint_time
#undef uint_addr


/* For default integer values we use an enum, while for default string values we
 * use the default_options struct instance that is already being used
 * for option parsing, and a second one for internal options when !INTERNAL
 */
extern const options_t default_options;

#ifndef EXPOSE_INTERNAL_OPTIONS
/* only needs to contain string options, but no compile-time way to
 * do that without having OPTION_COMMAND_INTERNAL_STRING()!
 */
extern const internal_options_t default_internal_options;
#endif

#define IS_OPTION_INTERNAL(name) (OPTION_IS_INTERNAL_##name)
#define IS_OPTION_STRING(name) (OPTION_IS_STRING_##name)

#define DEFAULT_OPTION_VALUE(name) (IS_OPTION_STRING(name) ? (int)default_options.name : \
                                    OPTION_DEFAULT_VALUE_##name)
#ifdef EXPOSE_INTERNAL_OPTIONS
#  define DEFAULT_INTERNAL_OPTION_VALUE DEFAULT_OPTION_VALUE
#else
#  define DEFAULT_INTERNAL_OPTION_VALUE(name) (IS_OPTION_STRING(name) ? \
                                               default_internal_options.name : \
                                               OPTION_DEFAULT_VALUE_##name)
#endif

#define RUNNING_WITHOUT_CODECACHE()	/* need to be filled up */

extern char option_string[];
extern options_t dentre_options;
extern read_write_lock_t options_lock;

/* check for empty is considered safe w/o the read lock
 * this takes the field name only, and not {DYNAMO,INTERNAL}_OPTION macro,
 * since those macros will ASSERT_OWN_READWRITE_LOCK(<is_stringtype>, &options_lock)
 */
#define IS_STRING_OPTION_EMPTY(op) ((dentre_options.op)[0] == '\0')

/* single character check for ALL is considered safe w/o the read lock
 * similarly to IS_STRING_OPTION_EMPTY see above
 */
#define IS_LISTSTRING_OPTION_FORALL(op) ((dentre_options.op)[0] == '*')

#ifdef EXPOSE_INTERNAL_OPTIONS
#  define IS_INTERNAL_STRING_OPTION_EMPTY(op) IS_STRING_OPTION_EMPTY(op)
#else
#  define IS_INTERNAL_STRING_OPTION_EMPTY(op) ((default_internal_options.op)[0] == '\0')
#endif


/* full access to string requires read lock */
static inline void
string_option_read_lock() { 
    read_lock(&options_lock); 
}
static inline void
string_option_read_unlock() { 
    read_unlock(&options_lock); 
}


int 
options_init(void);

#endif
