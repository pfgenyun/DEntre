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



int 
options_init(void);

#endif
