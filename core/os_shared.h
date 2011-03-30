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
#define _OS_SHARED_H_


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

char *get_application_pid(void);
char *get_application_name(void);
const char *get_application_short_name(void);


#endif
