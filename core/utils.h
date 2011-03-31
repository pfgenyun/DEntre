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

#ifndef _UTILS_H_
#define _UTILS_H_	1

#ifdef assert
# undef assert
#endif
/* avoid mistake of lower-case assert */
#define assert assert_no_good_use_ASSERT_instead

#ifdef DEBUG
# ifdef INTERNAL
/* cast to void to avoid gcc warning "statement with no effect" when used as
 * a statement and x is a compile-time false
 * FIXME: cl debug build allocates a local for each ?:, so if this gets
 * unrolled in some optionsx or other expansion we could have stack problems!
 */
#   define ASSERT(x) \
        ((void)((!(x)) ? (internal_error(__FILE__, __LINE__, #x), 0) : 0))
/* make type void to avoid gcc 4.1 warnings about "value computed is not used"
 * (case 7851).  can also use statement expr ({;}) but only w/ gcc, not w/ cl.
 */
#   define ASSERT_MESSAGE(msg, x) ((!(x)) ? \
        (internal_error(msg " @" __FILE__, __LINE__, #x), (void)0) : (void)0)
#   define REPORT_CURIOSITY(x) do {                                                   \
            if (!ignore_assert(__FILE__":"STRINGIFY(__LINE__), "curiosity : "#x)) {   \
                report_dynamorio_problem(NULL, DUMPCORE_CURIOSITY, NULL, NULL,        \
                                         "CURIOSITY : %s in file %s line %d", #x,     \
                                          __FILE__, __LINE__);                        \
            }                                                                         \
        } while (0)
#   define ASSERT_CURIOSITY(x) do {                                                   \
           if (!(x)) {                                                                \
               REPORT_CURIOSITY(x);                                                   \
           }                                                                          \
       } while (0)  
#   define ASSERT_CURIOSITY_ONCE(x) do {                                              \
           if (!(x)) {                                                                \
               DO_ONCE({REPORT_CURIOSITY(x);});                                       \
           }                                                                          \
       } while (0)  
# else 
/* cast to void to avoid gcc warning "statement with no effect" */
#   define ASSERT(x) \
        ((void)((!(x)) ? (internal_error(__FILE__, __LINE__, ""), 0) : 0))
#   define ASSERT_MESSAGE(msg, x) ((void)((!(x)) ? (internal_error(__FILE__, __LINE__, ""), 0) : 0))
#   define ASSERT_CURIOSITY(x) ((void) 0)
#   define ASSERT_CURIOSITY_ONCE(x) ((void) 0)
# endif /* INTERNAL */
# define ASSERT_NOT_TESTED() SYSLOG_INTERNAL_WARNING_ONCE("Not tested @%s:%d", \
                                                          __FILE__, __LINE__)
#else
# define ASSERT(x)         ((void) 0)
# define ASSERT_MESSAGE(msg, x) ASSERT(x)
# define ASSERT_NOT_TESTED() ASSERT(true)
# define ASSERT_CURIOSITY(x) ASSERT(true)
# define ASSERT_CURIOSITY_ONCE(x) ASSERT(true)
#endif /* DEBUG */

#define ASSERT_NOT_REACHED() ASSERT(false)
#define ASSERT_BUG_NUM(num, x) ASSERT_MESSAGE("Bug #"#num, x)
#define ASSERT_NOT_IMPLEMENTED(x) ASSERT_MESSAGE("Not implemented", x)



#ifdef DEBUG
# define LOG(file, mask, level, ...) do {        \
  if (stats != NULL &&                           \
      stats->loglevel >= (level) &&              \
      (stats->logmask & (mask)) != 0)            \
      print_log(file, mask, level, __VA_ARGS__); \
  } while (0)
  /* use DOELOG for customer visible logging. statement can be a {} block */
# define DOELOG(level, mask, statement) do {    \
  if (stats != NULL &&                          \
      stats->loglevel >= (level) &&             \
      (stats->logmask & (mask)) != 0)           \
    statement                                   \
  } while (0)
# define DOCHECK(level, statement) do {     \
  if (DYNAMO_OPTION(checklevel) >= (level)) \
    statement                               \
  } while (0)
# ifdef INTERNAL
#  define DOLOG DOELOG
#  define LOG_DECLARE(declaration) declaration
# else
#  define DOLOG(level, mask, statement)
#  define LOG_DECLARE(declaration)
# endif /* INTERNAL */
# define THREAD ((dcontext == NULL) ? INVALID_FILE : \
                 ((dcontext == GLOBAL_DCONTEXT) ? main_logfile : dcontext->logfile))
# define THREAD_GET get_thread_private_logfile()
# define GLOBAL main_logfile
#else  /* !DEBUG */
/* make use of gcc macro varargs, LOG's args may be ifdef DEBUG */
/* the macro is actually ,fmt... but C99 requires one+ argument which we just strip */
# define LOG(file, mask, level, ...)
# define DOLOG(level, mask, statement)
# define DOELOG DOLOG
# define LOG_DECLARE(declaration)
# define DOCHECK(level, statement) /* nothing */
#endif

#endif
