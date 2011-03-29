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

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include "globals.h"

void 
getnamefrompid(int pid, char *name, uint maxlen)
{
    int fd,n;
    char tempstring[200+1], *lastpart;

    /*this is a shitty way of getting the process name,
    but i can't think of anything better... */

    snprintf(tempstring, 200+1, "/proc/%d/cmdline", pid);
    fd = open_syscall(tempstring, O_RDONLY, 0);
    /* buffer overflow even if only off by 1 can be devastating */
    n = read_syscall(fd, tempstring, 200);   
    tempstring[n] = '\0';
    lastpart = rindex(tempstring, '/');

    if (lastpart == NULL)
      lastpart = tempstring;
    else
      lastpart++; /*don't include last '/' in name*/ 

    strncpy(name, lastpart, maxlen-1);
    name[maxlen-1]  = '\0'; /* if max no null */

    close_syscall(fd);
}

