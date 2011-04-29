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

#ifndef _OS_SHARED_H_
#define _OS_SHARED_H_	1

enum {VM_ALLOCATION_BOUNDARY = 64*1024}; 

enum {
    HEAP_ERROR_SUCCESS = 0,
    /* os_heap_reserve_in_region() only, couldn't find a place to reserve within region */
    HEAP_ERROR_CANT_RESERVE_IN_REGION = 1,
    /* os_heap_reserve() only, Linux only, mmap failed to reserve at preferred address */ 
    HEAP_ERROR_NOT_AT_PREFERRED = 2,
};
typedef uint heap_error_code_t;

void os_init(void);

void * os_heap_reserve(void *preferred, size_t size, heap_error_code_t *error_code, 
		bool executable);

bool os_heap_commit(void *p, size_t size, uint prot, heap_error_code_t *error_code);

void update_all_memory_areas(app_pc start, app_pc end_in, uint prot, int type);

/* file operations */
/* defaults to read only access, if write is not set ignores others */
#define OS_OPEN_READ        0x01
#define OS_OPEN_WRITE       0x02
#define OS_OPEN_APPEND      0x04
#define OS_OPEN_REQUIRE_NEW 0x08
#define OS_EXECUTE          0x10 /* only used on win32, currently */
#define OS_SHARE_DELETE     0x20 /* only used on win32, currently */
#define OS_OPEN_FORCE_OWNER 0x40 /* only used on win32, currently */
#define OS_OPEN_ALLOW_LARGE 0x80 /* only used on linux32, currently */
/* always use OS_OPEN_REQUIRE_NEW when asking for OS_OPEN_WRITE, in
 * order to avoid hard link or symbolic link attacks if the file is in
 * a world writable locations and the process may have high
 * privileges.
 */

file_t os_open(const char *fname, int os_open_flags);
file_t os_open_directory(const char *fname, int os_open_flags);

void os_close(file_t f);
/* returns number of bytes written, negative if failure */
ssize_t os_write(file_t f, const void *buf, size_t count);
/* returns number of bytes read, negative if failure */
ssize_t os_read(file_t f, void *buf, size_t count);
void os_flush(file_t f);

/* For use with os_file_seek(), specifies the origin at which to apply the offset
 * NOTE - keep in synch with DR_SEEK_* in insturment.h and SEEK_* from Linux headers */
#define OS_SEEK_SET 0  /* start of file */
#define OS_SEEK_CUR 1  /* current file position */
#define OS_SEEK_END 2  /* end of file */
/* seek the current file position to offset bytes from origin, return true if successful */
bool os_seek(file_t f, int64 offset, int origin);
/* return the current file position, -1 on failure */
int64 os_tell(file_t f);

bool os_delete_file(const char *file_name);
bool os_delete_mapped_file(const char *filename);
bool os_rename_file(const char *orig_name, const char *new_name, bool replace);
/* These routines do not update dynamo_areas; use the non-os_-prefixed versions */

process_id_t get_process_id(void);
char *get_application_pid(void);
char *get_application_name(void);
const char *get_application_short_name(void);

uint query_time_seconds();


#define DE_MEMPROT_NONE  0x00 /**< No read, write, or execute privileges. */
#define DE_MEMPROT_READ  0x01 /**< Read privileges. */
#define DE_MEMPROT_WRITE 0x02 /**< Write privileges. */
#define DE_MEMPROT_EXEC  0x04 /**< Execute privileges. */

#define MEMPROT_NONE  DE_MEMPROT_NONE
#define MEMPROT_READ  DE_MEMPROT_READ
#define MEMPROT_WRITE DE_MEMPROT_WRITE
#define MEMPROT_EXEC  DE_MEMPROT_EXEC


/**
 * Flags describing memory used by de_query_memory_ex().
 */
typedef enum {
    DE_MEMTYPE_FREE,  /**< No memory is allocated here */
    DE_MEMTYPE_IMAGE, /**< An executable file is mapped here */
    DE_MEMTYPE_DATA,  /**< Some other data is allocated here */
} dr_mem_type_t;


/***************************************************************************
 * SELF_PROTECTION
 */

/* Values for protect_mask to specify what is write-protected from malicious or
 * inadvertent modification by the application.
 * DATA_CXTSW and GLOBAL are done on each context switch
 * the rest are on-demand:
 * DATASEGMENT, DATA_FREQ, and GENCODE only on the rare occasions when we write to them
 * CACHE only when emitting or {,un}linking
 * LOCAL only on path in DR that needs to write to local
 */
enum {
    /* Options specifying protection of our DR dll data sections: */
    /* .data == variables written only at init or exit time or rarely in between */
    SELFPROT_DATA_RARE   = 0x001,
    /* .fspdata == frequently written enough that we separate from .data.
     * FIXME case 8073: currently these are unprotected on every cxt switch
     */
    SELFPROT_DATA_FREQ   = 0x002,
    /* .cspdata == so frequently written that to protect them requires unprotecting
     * every context switch.
     */
    SELFPROT_DATA_CXTSW  = 0x004,

    /* if GLOBAL && !DCONTEXT, entire dcontext is unprotected, rest of
     *   global allocs are protected;
     * if GLOBAL && DCONTEXT, cache-written fields of dcontext are unprotected,
     *   rest are protected;
     * if !GLOBAL, DCONTEXT should not be used
     */
    SELFPROT_GLOBAL      = 0x008,
    SELFPROT_DCONTEXT    = 0x010, /* means we split out unprotected_context_t -- 
                                   * no actual protection unless SELFPROT_GLOBAL */
    SELFPROT_LOCAL       = 0x020,
    SELFPROT_CACHE       = 0x040, /* FIXME: thread-safe NYI when doing all units */
    SELFPROT_STACK       = 0x080, /* essentially always on with clean-dstack dispatch()
                                   * design, leaving as a bit in case we do more later */
    /* protect our generated thread-shared and thread-private code */
    SELFPROT_GENCODE     = 0x100,
    /* FIXME: TEB page on Win32 
     * Other global structs, like thread-local callbacks on Win32?
     * PEB page?
     */
    /* options that require action on every context switch
     * FIXME: global heap used to be much rarer before shared
     * fragments, only containing "important" data, which is why we
     * un-protected on every context switch.  We should re-think that
     * now that most things are shared.
     */
    SELFPROT_ON_CXT_SWITCH = (SELFPROT_DATA_CXTSW | SELFPROT_GLOBAL
                              /* FIXME case 8073: this is only temporary until
                               * we finish implementing .fspdata unprots */
                              | SELFPROT_DATA_FREQ),
    SELFPROT_ANY_DATA_SECTION = (SELFPROT_DATA_RARE | SELFPROT_DATA_FREQ |
                                 SELFPROT_DATA_CXTSW),
};

enum
{
	DATASEC_NEVER_PROC = 0,
	DATASEC_RARELY_PROC,
	DATASEC_FREQ_PROC,
	DATASEC_CXTSW_PROC,
	DATASEC_NUM,
};

extern const uint DATASEC_SELFPROC[];
extern const char * const DATASEC_NAMES[];

extern const uint datasec_writable_neverprot;
extern uint datasec_writable_rareprot;
extern uint datasec_writable_freqprot;
extern uint datasec_writable_cxtswprot;


/* this is a uint not a bool */
#define DATASEC_WRITABLE(which) \
    ((which) == DATASEC_RARELY_PROT ? datasec_writable_rareprot : \
     ((which) == DATASEC_CXTSW_PROT ? datasec_writable_cxtswprot : \
      ((which) == DATASEC_FREQ_PROT ? datasec_writable_freqprot : \
       datasec_writable_neverprot)))


/* these must be plain literals since we need these in pragmas/attributes */
#define NEVER_PROTECTED_SECTION  ".nspdata"
#define RARELY_PROTECTED_SECTION ".data"
#define FREQ_PROTECTED_SECTION   ".fspdata"
#define CXTSW_PROTECTED_SECTION  ".cspdata"


#define DECLARE_CXTSWPROT_VAR(var, ...)							\
	START_DATA_SECTION(CXTSW_PROTECTED_SECTION, "w")			\
	var VAR_IN_SECTION(CXTSW_PROTECTED_SECTION) = __VA_ARGS__;	\
	END_DATA_SECTION()


#define DECLARE_FREQPROT_VAR(var, ...)                        \
    START_DATA_SECTION(FREQ_PROTECTED_SECTION, "w")           \
    var VAR_IN_SECTION(FREQ_PROTECTED_SECTION) = __VA_ARGS__; \
    END_DATA_SECTION() 


#if defined(CLIENT_INTERFACE) || defined(HOT_PATCHING_INTERFACE)
/* Note that this is NOT identical to module_handle_t: on Linux this
 * is a pointer to a loader data structure and NOT the base address
 * (xref PR 366195).
 */
typedef void * shlib_handle_t;
typedef void (*shlib_routine_ptr_t)();

shlib_handle_t load_shared_library(char *name);
#endif


#if defined(CLIENT_INTERFACE)
shlib_routine_ptr_t lookup_library_routine(shlib_handle_t lib, char *name);
void unload_shared_library(shlib_handle_t lib);
void shared_library_error(char *buf, int maxlen);
/* addr is any pointer known to lie within the library */
bool shared_library_bounds(IN shlib_handle_t lib, IN byte *addr,
                           OUT byte **start, OUT byte **end);
#endif

#endif
