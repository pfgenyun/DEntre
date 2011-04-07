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

#include <string.h>
#include <stdio.h>

#include "globals.h"
#include "utils.h"
#include "config.h"
#include "options.h"


#ifndef EXPOSE_INTERNAL_OPTIONS
/* default values for internal options are kept in a separate struct */
#  define OPTION_COMMAND_INTERNAL(type, name, default_value, command_line_option, statement, description, flag, pcache) default_value, 
#  define OPTION_COMMAND(type, name, default_value, command_line_option, \
                         statement, description, flag, pcache) /* nothing */
/* read only source for default internal option values and names
 * no lock needed since never written
 */
const internal_options_t default_internal_options = {
#  include "optionsx.h"
};
#undef OPTION_COMMAND_INTERNAL
#undef OPTION_COMMAND
#endif


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
/* Temporary string, static to save stack space in synchronize_dynamic_options().
 * FIXME case 8074: should protect this better as w/o DR dll randomization attacker
 * can repeatedly try to clobber.  Move to heap?  Or shrink stack space elsewhere
 * and put back as synchronize_dynamic_options() local var.
 */
DECLARE_FREQPROT_VAR(char new_option_string[MAX_OPTIONS_STRING], {0,});
/* temporary structure */
options_t temp_options = {
# include "optionsx.h"
};

/* dynamo_options and temp_options, the two writable option structures,
 * are both protected by this lock, which is kept outside of the protected 
 * section to ease bootstrapping issues
 */
DECLEAR_CXTSWPROT_VAR(read_write_lock_t options_lock, INIT_READWRITE_LOCK(options_lock));

#else /* !NOT_DYNAMORIO_CORE ****************************************/
# define ASSERT_OWN_OPTIONS_LOCK(b, l)
# define CORE_STATIC
#endif /* !NOT_DYNAMORIO_CORE ****************************************/

#undef OPTION_COMMAND


/* PARSING HANDLER */
/* returns the space delimited or quote-delimited word
 * starting at strpos in the string str, or NULL if none
 * wordbuf is a caller allocated buffer of size wordbuflen that will hold 
     the copied word from the original string, since it assumes it cannot modify it
 */
static char*
getword(const char *str, const char **strpos, char *wordbuf, uint wordbuflen)
{
    uint i = 0;
    const char *pos = *strpos;
    char quote = '\0';

    if (pos < str || pos >= str + strlen(str))
        return NULL;

    if (*pos == '\0')
        return NULL; /* no more words */

    /* eat leading spaces */
    while (*pos == ' ' || *pos == '\t') {
        pos++;
    }

    /* extract the word */
    if (*pos == '\'' || *pos == '\"' || *pos == '`') {
        quote = *pos;
        pos++; /* don't include surrounding quotes in word */
    }
    while (*pos) {
        if (quote != '\0') {
            /* if quoted, only end on matching quote */
            if (*pos == quote) {
                pos++; /* include the quote */
                break;
            }
        } else {
            /* if not quoted, end on whitespace */
            if (*pos == ' ' || *pos == '\t' || 
                *pos == '\n' || *pos == '\r')
                break;
        }
        if (i<wordbuflen-1) {
            wordbuf[i++] = *pos;
            pos++;
        } else {
			/* need to filled up */
//            OPTION_PARSE_ERROR(ERROR_OPTION_TOO_LONG_TO_PARSE, 4, 
//                               get_application_name(), get_application_pid(), 
//                               strpos, IF_DEBUG_ELSE("Terminating", "Continuing"));
            /* just return truncated form */
            break;
        }
    }
    if (i == 0)
        return NULL; /* no more words */

    ASSERT(i<wordbuflen);
    wordbuf[i] = '\0';
    *strpos = pos;

    return wordbuf;
}

#define ISBOOL_bool 1
#define ISBOOL_uint 0
#define ISBOOL_uint_size 0
#define ISBOOL_uint_time 0
#define ISBOOL_uint_addr 0
#define ISBOOL_pathstring_t 0
#define ISBOOL_liststring_t 0


static void
parse_bool(bool *var, void *value)
{ 
    *var = *(bool*)value;
}

static void
parse_uint(uint *var, void *value)
{
    uint num;
    char *opt = (char*)value;

    if ((sscanf(opt, "0x%x", &num) == 1) ||
        (sscanf(opt, "%d", &num) == 1)) { /* atoi(opt) */
        *var = num;
    } else {
        /* var should be pre-initialized to default */
		/* need to be filled up */
//        OPTION_PARSE_ERROR(ERROR_OPTION_BAD_NUMBER_FORMAT, 4, 
//                           get_application_name(), get_application_pid(), opt,
//                           IF_DEBUG_ELSE("Terminating", "Continuing"));
    }
}

static void
parse_uint_size(uint *var, void *value)
{ 
    char unit;
    int num;
    int factor = 0;

    if (sscanf((char*)value, "%d%c", &num, &unit) == 1) {
        /* no unit specifier, default unit is Kilo for compatibility */
        factor = 1024;
    } else {
        switch (unit) {
        case 'B': // plain bytes
        case 'b': factor = 1; break;
        case 'K': // Kilo (bytes)
        case 'k': factor = 1024; break;
        case 'M': // Mega (bytes)
        case 'm': factor = 1024*1024; break;
        default: 
            /* var should be pre-initialized to default */
			/* need to be filled up */
//            OPTION_PARSE_ERROR(ERROR_OPTION_UNKNOWN_SIZE_SPECIFIER, 4, 
//                               get_application_name(), get_application_pid(),
//                               (char *)value, IF_DEBUG_ELSE("Terminating", "Continuing"));
            return;
        }
    }
    *var = num * factor;
}

static void
parse_uint_time(uint *var, void *value)
{ 
    char unit;
    int num;
    int factor = 0;

    if (sscanf((char*)value, "%d%c", &num, &unit) == 1) {
        /* no unit specifier, default unit is milliseconds */
        factor = 1;
    } else {
        switch (unit) {
        case 's': factor = 1000; break; // seconds in milliseconds
        case 'm': factor = 1000*60; break; // minutes in milliseconds
        default: 
            /* var should be pre-initialized to default */
			/* need to be filled up */
//	         OPTION_PARSE_ERROR(ERROR_OPTION_UNKNOWN_TIME_SPECIFIER, 4, 
//                               get_application_name(), get_application_pid(), 
//                               (char *)value, IF_DEBUG_ELSE("Terminating", "Continuing")); 
            return;
        }
    }
    *var = num * factor;
}

static void
parse_uint_addr(ptr_uint_t *var, void *value)
{
    ptr_uint_t num;
    char *opt = (char*)value;

    if (sscanf(opt, PIFX, &num) == 1) { /* atoi(opt) */
        *var = num;
    } else {
        /* var should be pre-initialized to default */
		/* need to be filled up */
//        OPTION_PARSE_ERROR(ERROR_OPTION_BAD_NUMBER_FORMAT, 4, 
//                           get_application_name(), get_application_pid(), opt,
//                           IF_DEBUG_ELSE("Terminating", "Continuing"));
    }
}

static inline void
parse_pathstring_t(pathstring_t *var, void *value) { 
    strncpy(*var, (char*)value, MAX_PATH_OPTION_LENGTH-1);
    if (strlen((char *)value) > MAX_PATH_OPTION_LENGTH) {
			/* need to be filled up */
//        OPTION_PARSE_ERROR(ERROR_OPTION_TOO_LONG_TO_PARSE, 4,
//                           get_application_name(), get_application_pid(), 
//                           (char *)value, IF_DEBUG_ELSE("Terminating", "Continuing"));
    }
    /* truncate if max (strncpy doesn't put NULL) */
    (*var)[MAX_PATH_OPTION_LENGTH-1] = '\0';
}

static void
parse_liststring_t(liststring_t *var, void *value) { 
    /* Case 5727: append by default (separating via ';') for liststring_t, as
     * opposed to what we do for all other option types where the final
     * specifier overwrites all previous.  The special prefix '#' can be used to
     * indicate overwrite.
     */
    size_t len;
    if (*((char *)value) == '#') {
        strncpy(*var, (((char*)value)+1), MAX_LIST_OPTION_LENGTH-1);
        len = strlen(((char *)value)+1);
    } else {
        len = strlen(*var) + strlen((char *)value);
        if (*var[0] != '\0') {
            len++; /*;*/
            strncat(*var, ";", MAX_LIST_OPTION_LENGTH-1 - strlen(*var));
        }
        strncat(*var, (char*)value, MAX_LIST_OPTION_LENGTH-1 - strlen(*var));
    }
    if (len >= MAX_LIST_OPTION_LENGTH) {
        /* FIXME: no longer is value always the single too-long factor
         * (could be appending a short option to a very long string), so
         * should we change the message to "option is too long, truncating"?
         */
			/* need to be filled up */
//        OPTION_PARSE_ERROR(ERROR_OPTION_TOO_LONG_TO_PARSE, 4,
//                           get_application_name(), get_application_pid(), 
//                           *var, IF_DEBUG_ELSE("Terminating", "Continuing"));
    }
    /* truncate if max (strncpy doesn't put NULL, even though strncat does) */
    (*var)[MAX_LIST_OPTION_LENGTH-1] = '\0';
}


#define OPTION_COMMAND(type, name, default_value, command_line_option,	\
						statement, description, flag, pcache)			\
{																		\
	value = NULL;														\
	if(ISBOOL_##type)													\
	{																	\
		if(!strcmp(opt+1, command_line_option))							\
		{																\
			value = &value_true;										\
		}																\
		else if(!strcmp(opt+1, "no_", 3)								\
			&&  !strcmp(opt+4, command_line_option))					\
		{																\
			value = &value_false;										\
		}																\
	}																	\
	else																\
	{																	\
		if(!strcmp(opt+1, command_line_option))							\
		{																\
			value = getword(optstr, &pos, wordbuffer, sizeof(wordbuffer));	\
		}																\
	}																	\
																		\
	if(value)															\
	{																	\
		parse_##type(&options->name, value);							\
		statement;														\
		continue;														\
	}																	\
}																		\

static int
set_dentre_options_common(options_t *options, char *optstr, bool for_this_process)
{
	char *opt;
	const char *pos = optstr;
	const char *prev_pos;
	bool got_badopt = false;
	char badopt[MAX_OPTION_LENGTH];

	char wordbuffer[MAX_OPTION_LENGTH];

	void *value = NULL;
	bool value_true = true;
	bool value_false = false;

	if(optstr == NULL)
		return 0;
	
	/* need to be filled up */
//	ASSERT_OWN_OPTIONS_LOCK(options==&dynamo_options || options==&temp_options, &options_lock);
//	ASSERT(!OPTIONS_PROTECTED());
	prev_pos = pos;
	while((opt = getword(optstr, &pos, wordbuffer, sizeof(wordbuffer))) != NULL)
	{
		if(opt[0] == '-')
		{
			/* need to be filled up */
//			#include "optionsx.h"
		}

		if(!got_badopt)
		{
			snprintf(badopt, BUFFER_SIZE_ELEMENTS(badopt), "%s", opt);
			NULL_TERMINATE_BUFFER(badopt);
		}

		got_badopt = true;
		prev_pos = pos;  /* can omit */
	}

	if(got_badopt)
	{
		/* need to be filled up */
//		OPTION_PARSE_ERROR(ERROR_OPTION_UNKNOW, 4, get_application_name(),
//				get_application_id(), badopt, 
//				IF_DEBUG_ELSE("Terminating", "Continuing"));
	}

	return (int)got_badopt;

}

#undef OPTION_COMMAND


CORE_STATIC int
set_dentre_options(options_t *options, char *optstr)
{
	/* need to be filled up */
	set_dentre_options_common(options, optstr, true);
}


static bool
check_option_compatibility()
{
	/* need to be filled up */	
	return true;
}


int options_init()
{
	int ret = 0;
	int retval;

	write_lock(&options_lock);
	ASSERT(sizeof(dentre_options) == sizeof(options_t));
	retval = get_parameter(PARAM_STR(DENTRE_VAR_OPTIONS), option_string,
							sizeof(option_string));
	if(IS_GET_PARAMETER_SUCCESS(retval))
		ret = set_dentre_options(&dentre_options, option_string);

	check_option_compatibility();

	write_unlock(&options_lock);

	return ret;
}



