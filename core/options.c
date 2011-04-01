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

#include "globals.h"
#include "utils.h"

#ifdef EXPOSE_INTERNAL_OPTIONS
#  define OPTION_COMMAND_INTERNAL OPTION_COMMAND
#else 
/* DON'T FIXME: In order to support easy switching of an internal
   option into user accessible one we could waste some memory by
   allocating fields in options_t even for INTERNAL options.  That would
   let us have INTERNAL_OPTION be a more flexible macro that can use
   either the constant value, or the dynamically set variable
   depending on the option declaration in optionsx.h.  However, 
   it increases the code size of this file (there is also a minor increase
   in other object files since more option fields need longer than 8-bit offsets)
   For now we can live without this.
*/
#  define OPTION_COMMAND_INTERNAL(type, name, default_value, command_line_option, statement, description, flag, pcache) /* nothing, */
#endif


/* all to default values */
#define OPTION_COMMAND(type, name, default_value, command_line_option, \
                       statement, description, flag, pcache) default_value,
/* read only source for default option values and names
 * no lock needed since never written
 */
const options_t default_options = {
#  include "optionsx.h"
};

#ifndef NOT_DYNAMORIO_CORE /*****************************************/

# define SELF_PROTECT_OPTIONS() SELF_PROTECT_DATASEC(DATASEC_RARELY_PROT)
# define SELF_UNPROTECT_OPTIONS() SELF_UNPROTECT_DATASEC(DATASEC_RARELY_PROT)
/* WARNING: testing positive direction is racy (other threads unprot .data
 * for brief windows), negative is reliable
 */
# define OPTIONS_PROTECTED() (DATASEC_PROTECTED(DATASEC_RARELY_PROT))

/* Holds a copy of the last read option string from the registry, NOT a
 * canonical option string.
 */
char option_string[MAX_OPTIONS_STRING] = {0,};
# define ASSERT_OWN_OPTIONS_LOCK(b, l) /* need to be filled up */
# define CORE_STATIC static
/* official options */
options_t dentre_options = {
# include "optionsx.h"
};


DECLEAR_CXTSWPROT_VAR(read_write_lock_t options_lock, INIT_READWRITE_LOCK(options_lock));

int options_init()
{
	int ret = 0;
	int retval;

	write_lock(&options_lock);
	ASSERT(sizeof(dentre_options) == sizeof(options_t));
	/* need to be filled up */
	write_unlock(&options_lock);
}



