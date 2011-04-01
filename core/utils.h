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

void print_log(file_t logfile, uint mask, uint level, char *fmt, ...);
file_t get_thread_private_logfile(void);

#ifdef DEBUG
#   define DODEBUG(statement) do { statement } while (0)
#   define DEBUG_DECLARE(declaration) declaration
#   define DOSTATS(statement) do { statement } while (0)
    /* FIXME: move to stats.h */
    /* Note : stats macros are called in places where it is not safe to hold any lock
     * (such as special_heap_create_unit, others?), if ever go back to using a mutex 
     * to protect the stats need to update such places
     */  
    /* global and thread local stats, can be used as lvalues, 
     * not used if not DEBUG */
    /* We assume below that all stats are aligned and thus reading and writing
     * stats are atomic operations on Intel x86 */
    /* In general should prob. be using stats_add_[peak, max] instead of
     * stats_[peak/max] since they tie the adjustment of the stat to the 
     * setting of the max, otherwise you're open to race conditions involving
     * multiple threads adjusting the same stats and setting peak/max FIXME
     */
#   define GLOBAL_STATS_ON() (stats != NULL && INTERNAL_OPTION(global_stats))
#   define THREAD_STAT(dcontext, stat)       \
        (dcontext->thread_stats)->stat##_thread
#   define THREAD_STATS_ON(dcontext)                                    \
        (dcontext != NULL && INTERNAL_OPTION(thread_stats) &&           \
         dcontext->thread_stats != NULL)
#   define DO_THREAD_STATS(dcontext, statement) do {                    \
        if (THREAD_STATS_ON(dcontext)) {                                \
            statement;                                                  \
        }                                                               \
    } while (0)
#   define XSTATS_WITH_DC(var, statement) do {                          \
        dcontext_t *var = NULL;                                      \
        if (INTERNAL_OPTION(thread_stats))                              \
           var = get_thread_private_dcontext();                         \
        statement;                                                      \
    } while (0)

#   define STATS_INC XSTATS_INC
/* we'll expose more *_DC as we need them */
#   define STATS_INC_DC XSTATS_INC_DC
#   define STATS_DEC XSTATS_DEC
#   define STATS_ADD XSTATS_ADD
#   define STATS_SUB XSTATS_SUB
#   define STATS_INC_ASSIGN XSTATS_INC_ASSIGN
#   define STATS_ADD_ASSIGN XSTATS_ADD_ASSIGN
#   define STATS_MAX XSTATS_MAX
#   define STATS_TRACK_MAX XSTATS_TRACK_MAX
#   define STATS_PEAK XSTATS_PEAK
#   define STATS_ADD_MAX XSTATS_ADD_MAX
#   define STATS_ADD_PEAK XSTATS_ADD_PEAK
#   define STATS_RESET XSTATS_RESET

#else
#   define DODEBUG(statement)
#   define DEBUG_DECLARE(declaration)
#   define DOSTATS(statement)
#   define THREAD_STATS_ON(dcontext) false
#   define XSTATS_WITH_DC(var, statement) statement
#   define DO_THREAD_STATS(dcontext, statement) /* nothing */
#   define GLOBAL_STATS_ON() (stats != NULL && DYNAMO_OPTION(global_rstats))

/* Would be nice to catch incorrect usage of STATS_INC on a release-build
 * stat: if rename release vars, have to use separate GLOBAL_RSTAT though.
 */
#   define STATS_INC(stat) /* nothing */
#   define STATS_INC_DC(dcontext, stat) /* nothing */
#   define STATS_DEC(stat) /* nothing */
#   define STATS_ADD(stat, value) /* nothing */
#   define STATS_SUB(stat, value) /* nothing */
#   define STATS_INC_ASSIGN(stat, var) /* nothing */
#   define STATS_ADD_ASSIGN(stat, var, value) /* nothing */
#   define STATS_MAX(stat_max, stat_cur) /* nothing */
#   define STATS_TRACK_MAX(stats_track_max, val) /* nothing */
#   define STATS_PEAK(stat) /* nothing */
#   define STATS_ADD_MAX(stat_max, stat_cur, value) /* nothing */
#   define STATS_ADD_PEAK(stat, value) /* nothing */
#   define STATS_RESET(stat) /* nothing */
#endif /* DEBUG */


/* what-to-log bitmask values */
/* N.B.: if these constants are changed, win32gui must also be changed!
 * They are also duplicated in instrument.h -- too hard to get them to
 * automatically show up in right place in header files for release.
 */
#define LOG_NONE           0x00000000
#define LOG_STATS          0x00000001
#define LOG_TOP            0x00000002
#define LOG_THREADS        0x00000004
#define LOG_SYSCALLS       0x00000008
#define LOG_ASYNCH         0x00000010
#define LOG_INTERP         0x00000020
#define LOG_EMIT           0x00000040
#define LOG_LINKS          0x00000080
#define LOG_CACHE          0x00000100
#define LOG_FRAGMENT       0x00000200
#define LOG_DISPATCH       0x00000400
#define LOG_MONITOR        0x00000800
#define LOG_HEAP           0x00001000
#define LOG_VMAREAS        0x00002000
#define LOG_SYNCH          0x00004000
#define LOG_MEMSTATS       0x00008000
#define LOG_OPTS           0x00010000
#define LOG_SIDELINE       0x00020000
#define LOG_SYMBOLS        0x00040000
#define LOG_RCT            0x00080000
#define LOG_NT             0x00100000
#define LOG_HOT_PATCHING   0x00200000
#define LOG_HTABLE         0x00400000
#define LOG_MODULEDB       0x00800000
#define LOG_LOADER         0x01000000

#define LOG_ALL_RELEASE    0x01e0ffff
#define LOG_ALL            0x01ffffff

typedef void * contention_event_t;
																	  
typedef struct _mutex_t
{
	/* need to be filled up */
}mutex_t;


typedef struct _read_write_lock_t
{
	mutex_t lock;
	volatile int num_readers;
	thread_id_t writer;
	volatile int num_pending_readers;
	contention_event_t writer_waiting_reader;
	contention_event_t reader_waiting_writer;

}read_write_lock_t;


#define INIT_READWRITE_LOCK(lock)	/* need to be filled up */

void write_lock(read_write_lock_t *rw);
void write_unlock(read_write_lock_t *rw);



#endif
