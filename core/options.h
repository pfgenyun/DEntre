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
