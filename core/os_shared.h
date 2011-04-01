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


/* these must be plain literals since we need these in pragmas/attributes */
#define NEVER_PROTECTED_SECTION  ".nspdata"
#define RARELY_PROTECTED_SECTION ".data"
#define FREQ_PROTECTED_SECTION   ".fspdata"
#define CXTSW_PROTECTED_SECTION  ".cspdata"


#define DECLEAR_CXTSWPROT_VAR(var, ...)							\
	START_DATA_SECTION(CXTSW_PROTECTED_SECTION, "w")			\
	var VAR_IN_SECTION(CXTSW_PROTECTED_SECTION) = __VA_ARGS__;	\
	END_DATA_SECTION()

#endif
