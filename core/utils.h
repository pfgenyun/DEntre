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


#ifdef CLIENT_INTERFACE
# ifdef DEBUG
#  define CLIENT_ASSERT(x, msg) apicheck(x, msg)
# else
#  define CLIENT_ASSERT(x, msg) /* PR 215261: nothing in release builds */
# endif
#else
# define CLIENT_ASSERT(x, msg) ASSERT_MESSAGE(msg, x)
#endif


#define CHECK_TRUNCATE_TYPE_byte(val) ((val) >= 0 && (val) <= UCHAR_MAX)
#define CHECK_TRUNCATE_TYPE_sbyte(val) ((val) <= SCHAR_MAX && ((int64)(val)) >= SCHAR_MIN)
#define CHECK_TRUNCATE_TYPE_ushort(val) ((val) >= 0 && (val) <= USHRT_MAX)
#define CHECK_TRUNCATE_TYPE_short(val) ((val) <= SHRT_MAX && ((int64)(val)) >= SHRT_MIN)
#define CHECK_TRUNCATE_TYPE_uint(val) ((val) >= 0 && (val) <= UINT_MAX)
#ifdef LINUX
/* We can't do the proper int check on Linux because it generates a warning if val has
 * type uint that I can't seem to cast around and is impossible to ignore -
 * "comparison is always true due to limited range of data type".
 * See http://gcc.gnu.org/ml/gcc/2006-01/msg00784.html and note that the suggested
 * workaround option doesn't exist as far as I can tell. We are potentially in trouble
 * if val has type int64, is negative, and too big to fit in an int. */
# define CHECK_TRUNCATE_TYPE_int(val) ((val) <= INT_MAX)
#else
# define CHECK_TRUNCATE_TYPE_int(val) ((val) <= INT_MAX && ((int64)(val)) >= INT_MIN)
#endif
#ifdef X64
# define CHECK_TRUNCATE_TYPE_size_t(val) ((val) >= 0)
/* avoid gcc warning: always true anyway since stats_int_t == int64 */
# define CHECK_TRUNCATE_TYPE_stats_int_t(val) (true)
#else
# define CHECK_TRUNCATE_TYPE_size_t CHECK_TRUNCATE_TYPE_uint
# define CHECK_TRUNCATE_TYPE_stats_int_t CHECK_TRUNCATE_TYPE_int
#endif

/* FIXME: too bad typeof is a GCC only extension - so need to pass both var and type */

/* var = (type) val; should always be preceded by a call to ASSERT_TRUNCATE  */
/* note that it is also OK to use ASSERT_TRUNCATE(type, type, val) for return values */
#define ASSERT_TRUNCATE(var, type, val) ASSERT(sizeof(type) == sizeof(var) && \
                                               CHECK_TRUNCATE_TYPE_##type(val) && \
                                               "truncating "#var" to "#type)

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


/* thread synchronization */
#define LOCK_FREE_STATE -1     /* allows a quick >=0 test for contention */

#define CONTENTION_EVENT_NOT_CREATED ((contention_event_t)0)

typedef void * contention_event_t;
																	  
typedef struct _mutex_t
{
	/* need to be filled up */
	volatile int lock_requests;
	contention_event_t contended_event;
}mutex_t;

typedef struct _spin_mutex_t
{
	mutex_t lock;
}spin_lock_t;

typedef struct _read_write_lock_t
{
	mutex_t lock;
	volatile int num_readers;
	thread_id_t writer;
	volatile int num_pending_readers;
	contention_event_t writer_waiting_reader;
	contention_event_t reader_waiting_writer;

}read_write_lock_t;


#ifdef DEADLOCK_AVOIDANCE
#  define LOCK_RANK(lock) lock##_rank
/* This should be the single place where all ranks are declared */
/* Your lock should preferably take the last possible rank in this
 * list, usually at the location marked as ADD HERE  */

enum {
    LOCK_RANK(outermost_lock), /* pseudo lock, sentinel of thread owned locks list */
    LOCK_RANK(thread_in_DR_exclusion),  /* outermost */
    LOCK_RANK(all_threads_synch_lock),  /* < thread_initexit_lock */

    LOCK_RANK(trace_building_lock), /* < bb_building_lock, < table_rwlock */

    LOCK_RANK(bb_building_lock), /* < change_linking_lock + all vm and heap locks */
    /* decode exception -> check if should_intercept requires all_threads 
     * FIXME: any other locks that could be interrupted by exception that
     * could be app's fault?
     */
    LOCK_RANK(thread_initexit_lock), /* < all_threads_lock, < snapshot_lock */

    /* FIXME: grabbed on an exception, which could happen anywhere! 
     * possible deadlock if already held */
    LOCK_RANK(all_threads_lock),  /* < global_alloc_lock */

    LOCK_RANK(linking_lock),  /* < dynamo_areas < global_alloc_lock */

#ifdef SHARING_STUDY
    LOCK_RANK(shared_blocks_lock), /* < global_alloc_lock */
    LOCK_RANK(shared_traces_lock), /* < global_alloc_lock */
#endif

    LOCK_RANK(synch_lock), /* per thread, < protect_info */

    LOCK_RANK(protect_info), /* < cache and heap traversal locks */

#if defined(CLIENT_SIDELINE) && defined(CLIENT_INTERFACE)
    LOCK_RANK(sideline_mutex), 
#endif

    LOCK_RANK(shared_cache_flush_lock), /* < shared_cache_count_lock,
                                           < shared_delete_lock,
                                           < change_linking_lock */
    LOCK_RANK(shared_delete_lock), /* < change_linking_lock < shared_vm_areas */
    LOCK_RANK(lazy_delete_lock), /* > shared_delete_lock, < shared_cache_lock */

    LOCK_RANK(shared_cache_lock), /* < dynamo_areas, < allunits_lock,
                                   * < table_rwlock for shared cache regen/replace,
                                   * < shared_vm_areas for cache unit flush,
                                   * < change_linking_lock for add_to_free_list */

    LOCK_RANK(change_linking_lock), /* < shared_vm_areas, < all heap locks */
    LOCK_RANK(shared_vm_areas), /* > change_linking_lock, < executable_areas  */
    LOCK_RANK(shared_cache_count_lock),
    LOCK_RANK(tracedump_mutex),  /* < table_rwlock, > change_linking_lock */

#if defined(CLIENT_SIDELINE) && defined(CLIENT_INTERFACE)
    LOCK_RANK(fragment_delete_mutex),
#endif

    LOCK_RANK(emulate_write_lock), /* in future may be < emulate_write_areas */

    LOCK_RANK(unit_flush_lock), /* > shared_delete_lock */

    LOCK_RANK(maps_iter_buf_lock), /* < executable_areas, < module_data_lock,
                                    * < hotp_vul_table_lock */

#ifdef HOT_PATCHING_INTERFACE
    /* This lock's rank needs to be after bb_building_lock because 
     * build_bb_ilist() is where injection takes place, which means the 
     * bb lock has been acquired before any hot patching related work is done 
     * on a bb.
     */
    LOCK_RANK(hotp_vul_table_lock), /* > bb_building_lock, 
                                     * < dynamo_areas, < heap_unit_lock. */
#endif
    LOCK_RANK(coarse_info_lock), /* < special_heap_lock, < global_alloc_lock,
                                  * > change_linking_lock */
    LOCK_RANK(special_units_list_lock), /* < special_heap_lock */
    LOCK_RANK(special_heap_lock), /* > bb_building_lock, > hotp_vul_table_lock
                                   * < dynamo_areas, < heap_unit_lock */
    LOCK_RANK(coarse_info_incoming_lock), /* > special_heap_lock, > coarse_info_lock,
                                           * > change_linking_lock */

    /* (We don't technically need a coarse_table_rwlock separate from table_rwlock
     * anymore but having it gives us flexibility so I'm leaving it)
     */
    LOCK_RANK(coarse_table_rwlock), /* < global_alloc_lock, < coarse_th_table_rwlock */
    /* We make the th table separate (we look in it while holding master table lock) */
    LOCK_RANK(coarse_th_table_rwlock), /* < global_alloc_lock */
    /* We make the pc table separate (we write it while holding master table lock) */
    LOCK_RANK(coarse_pclookup_table_rwlock), /* < global_alloc_lock */

    LOCK_RANK(executable_areas), /* < dynamo_areas < global_alloc_lock
                                  * < process_module_vector_lock (diagnostics)
                                  */
#ifdef RCT_IND_BRANCH
    LOCK_RANK(rct_module_lock), /* > coarse_info_lock, > executable_areas,
                                 * < heap allocation */
#endif
#ifdef RETURN_AFTER_CALL
    LOCK_RANK(after_call_lock), /* < table_rwlock, > bb_building_lock,
                                 * > coarse_info_lock, > executable_areas */
#endif
    LOCK_RANK(process_module_vector_lock), /* < snapshot_lock > all_threads_synch_lock */
    /* For Loglevel 1 and higher, with LOG_MEMSTATS, the snapshot lock is
     * grabbed on an exception, possible deadlock if already held FIXME */
    LOCK_RANK(snapshot_lock),   /* < dynamo_areas */
    LOCK_RANK(written_areas), /* > executable_areas
                               * < dynamo_areas < global_alloc_lock */
#ifdef PROGRAM_SHEPHERDING
    LOCK_RANK(futureexec_areas), /* > executable_areas
                                  * < dynamo_areas < global_alloc_lock */
#endif
    LOCK_RANK(pretend_writable_areas), /* < dynamo_areas < global_alloc_lock */
    LOCK_RANK(patch_proof_areas), /* < dynamo_areas < global_alloc_lock */
    LOCK_RANK(emulate_write_areas), /* < dynamo_areas < global_alloc_lock */
    LOCK_RANK(IAT_areas), /* < dynamo_areas < global_alloc_lock */
    LOCK_RANK(module_data_lock),  /* < loaded_module_areas */
#ifdef CLIENT_INTERFACE
    /* PR 198871: this same label is used for all client locks */
    LOCK_RANK(dr_client_mutex), /* > module_data_lock */
    LOCK_RANK(client_thread_count_lock), /* > dr_client_mutex */
    LOCK_RANK(client_flush_request_lock), /* > dr_client_mutex */
    LOCK_RANK(callback_registration_lock), /* > dr_client_mutex */
    LOCK_RANK(client_tls_lock), /* > dr_client_mutex */
#endif
    LOCK_RANK(table_rwlock), /* > dr_client_mutex */
    LOCK_RANK(loaded_module_areas),  /* < dynamo_areas < global_alloc_lock */
    LOCK_RANK(aslr_areas), /* < dynamo_areas < global_alloc_lock */
    LOCK_RANK(aslr_pad_areas), /* < dynamo_areas < global_alloc_lock */
    LOCK_RANK(native_exec_areas), /* < dynamo_areas < global_alloc_lock */
    LOCK_RANK(thread_vm_areas), /* currently never used */

    LOCK_RANK(app_pc_table_rwlock), /* > after_call_lock, > rct_module_lock,
                                     * > module_data_lock */

    LOCK_RANK(dead_tables_lock), /* < heap_unit_lock */
    LOCK_RANK(aslr_lock),

#ifdef HOT_PATCHING_INTERFACE
    LOCK_RANK(hotp_only_tramp_areas_lock),  /* > hotp_vul_table_lock,
                                             * < global_alloc_lock */
    LOCK_RANK(hotp_patch_point_areas_lock), /* > hotp_vul_table_lock,
                                             * < global_alloc_lock */
#endif
#ifdef CALL_PROFILE
    LOCK_RANK(profile_callers_lock), /* < global_alloc_lock */
#endif
    LOCK_RANK(coarse_stub_areas), /* < global_alloc_lock */
    LOCK_RANK(moduledb_lock), /* < global heap allocation */
    LOCK_RANK(pcache_dir_check_lock),
    LOCK_RANK(suspend_lock),
    LOCK_RANK(shared_lock),
    /* ADD HERE a lock around section that may allocate memory */

    /* N.B.: the order of allunits < global_alloc < heap_unit is relied on
     * in the {fcache,heap}_low_on_memory routines.  IMPORTANT - any locks
     * added between the allunits_lock and heap_unit_lock must have special
     * handling in the fcache_low_on_memory() routine. 
     */
    LOCK_RANK(allunits_lock),  /* < global_alloc_lock */
    LOCK_RANK(fcache_unit_areas), /* > allunits_lock, 
                                     < dynamo_areas, < global_alloc_lock */
    IF_LINUX_(IF_DEBUG(LOCK_RANK(elf_areas))) /* < all_memory_areas */
    IF_LINUX_(LOCK_RANK(all_memory_areas))    /* < dynamo_areas */
    LOCK_RANK(landing_pad_areas_lock),  /* < global_alloc_lock, < dynamo_areas */
    LOCK_RANK(dynamo_areas),    /* < global_alloc_lock */
    LOCK_RANK(map_intercept_pc_lock), /* < global_alloc_lock */
    LOCK_RANK(global_alloc_lock),/* < heap_unit_lock */
    LOCK_RANK(heap_unit_lock),   /* recursive */
    LOCK_RANK(vmh_lock),        /* lowest level */
    LOCK_RANK(last_deallocated_lock),
    /*---- no one below here can be held at a memory allocation site ----*/

    LOCK_RANK(tls_lock), /* if used for get_thread_private_dcontext() may
                          * need to be even lower: as it is, only used for set */
    LOCK_RANK(reset_pending_lock), /* > heap_unit_lock */

    LOCK_RANK(initstack_mutex),  /* FIXME: NOT TESTED */

    LOCK_RANK(event_lock),  /* FIXME: NOT TESTED */
    LOCK_RANK(do_threshold_mutex),  /* FIXME: NOT TESTED */
    LOCK_RANK(threads_killed_lock),  /* FIXME: NOT TESTED */
    LOCK_RANK(child_lock),  /* FIXME: NOT TESTED */
    
#ifdef SIDELINE
    LOCK_RANK(sideline_lock), /* FIXME: NOT TESTED */
    LOCK_RANK(do_not_delete_lock),/* FIXME: NOT TESTED */
    LOCK_RANK(remember_lock),/* FIXME: NOT TESTED */
    LOCK_RANK(sideline_table_lock),/* FIXME: NOT TESTED */
#endif
#ifdef SIMULATE_ATTACK
    LOCK_RANK(simulate_lock),
#endif
#ifdef KSTATS
    LOCK_RANK(process_kstats_lock),
#endif
#ifdef N64
    LOCK_RANK(request_region_be_heap_reachable_lock), /* > heap_unit_lock, vmh_lock
                                                       * < report_buf_lock (for assert) */
#endif
    LOCK_RANK(report_buf_lock),
    /* FIXME: if we crash while holding the all_threads_lock, snapshot_lock 
     * (for loglevel 1+, logmask LOG_MEMSTATS), or any lock below this
     * line (except the profile_dump_lock, and possibly others depending on
     * options) we will deadlock
     */
    LOCK_RANK(memory_info_buf_lock),

    LOCK_RANK(logdir_mutex),     /* recursive */
    LOCK_RANK(diagnost_reg_mutex),

    LOCK_RANK(prng_lock),
    /* ---------------------------------------------------------- */
    /* No new locks below this line, reserved for innermost ASSERT,
     * SYSLOG and STATS facilities */
    LOCK_RANK(options_lock),
    LOCK_RANK(eventlog_mutex), /* < datasec_selfprot_lock only for hello_message */
    LOCK_RANK(datasec_selfprot_lock),
    LOCK_RANK(thread_stats_lock),
    LOCK_RANK(innermost_lock), /* innermost internal lock, head of all locks list */
};

struct _thread_locks_t;
typedef struct _thread_locks_t thread_locks_t;

extern mutex_t outermost_lock;

void locks_thread_init(dcontext_t *dcontext);
void locks_thread_exit(dcontext_t *dcontext);
uint locks_not_closed(void);
bool thread_owns_no_locks(dcontext_t *dcontext);
bool thread_owns_one_lock(dcontext_t *dcontext, mutex_t *lock);
bool thread_owns_two_locks(dcontext_t *dcontext, mutex_t *lock1, mutex_t *lock2);
bool thread_owns_first_or_both_locks_only(dcontext_t *dcontext, mutex_t *lock1, mutex_t *lock2);


/* We need the (mutex_t) type specifier for direct initialization, 
   but not when defining compound structures, hence NO_TYPE */
#  define INIT_LOCK_NO_TYPE(name, rank) {LOCK_FREE_STATE,               \
                                         CONTENTION_EVENT_NOT_CREATED,  \
                                         name, rank,                    \
                                         INVALID_THREAD_ID,             \
                                         NULL, NULL,                    \
                                         0, 0, 0, 0, 0,                 \
                                         NULL, NULL}
#else
/* Ignore the arguments */
#  define INIT_LOCK_NO_TYPE(name, rank) {LOCK_FREE_STATE, CONTENTION_EVENT_NOT_CREATED}
#endif /* DEADLOCK_AVOIDANCE */


#define STRUCTURE_TYPE(x)

#define ASSIGN_INIT_LOCK_FREE(var, lock) do {                           \
     mutex_t initializer_##lock = STRUCTURE_TYPE(mutex_t) INIT_LOCK_NO_TYPE(\
            #lock "(mutex)" "@" __FILE__ ":" STRINGIFY(__LINE__),       \
                                                 LOCK_RANK(lock));      \
     var = initializer_##lock;                                          \
   } while (0)

#define INIT_READWRITE_LOCK(lock) STRUCTURE_TYPE(read_write_lock_t)		\
	{																	\
		INIT_LOCK_NO_TYPE(                                              \
		#lock "(readwrite)" "@" __FILE__ ":" STRINGIFY(__LINE__) ,       \
		LOCK_RANK(lock)),                                                \
		0, INVALID_THREAD_ID,                                            \
		0,                                                               \
		CONTENTION_EVENT_NOT_CREATED, CONTENTION_EVENT_NOT_CREATED       \
   }

void write_lock(read_write_lock_t *rw);
void write_unlock(read_write_lock_t *rw);

void read_lock(read_write_lock_t *rw);
void read_unlock(read_write_lock_t *rw);


/* Current implementation uses integers representing 32 bits */
typedef uint bitmap_element_t;
typedef bitmap_element_t bitmap_t[];

/* Note that we have some bitmap operations in linux/signal.c for
 *  kernel version of sigset_t as well as in
 *  win32/ntdll.c:tls_{alloc,free} which could use some of these
 *  facilities, but for now we leave those as more OS specific
 */

#define BITMAP_DENSITY   32
#define BITMAP_MASK(i)   (1 << ((i) % BITMAP_DENSITY))
#define BITMAP_INDEX(i)  ((i) / BITMAP_DENSITY)
#define BITMAP_NOT_FOUND ((uint)-1)

void bitmap_initialize_free(bitmap_t b, uint bitmap_size);
bool bitmap_check_consistency(bitmap_t b, uint bitmap_size, uint expect);


size_t get_random_offset(size_t max_offset);


/* alignment helpers, alignment must be power of 2 */
#define ALIGNED(x, alignment) ((((ptr_uint_t)x) & ((alignment)-1)) == 0)
#define ALIGN_FORWARD(x, alignment) \
    ((((ptr_uint_t)x) + ((alignment)-1)) & (~((alignment)-1)))


#endif
