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

#ifndef _MODULEDB_H_
#define _MODULEGB_H_	1

#include "globals.h"

#ifdef PROCESS_CONTROL
# define PROCESS_CONTROL_MODE_OFF                   0x0
# define PROCESS_CONTROL_MODE_WHITELIST             0x1
# define PROCESS_CONTROL_MODE_BLACKLIST             0x2

/* This mode is identical to whitelist mode, but requires that the user specify
 * an exe by name and its hashes; no anonymous hashes or exe names with no
 * hashes.  Case 10969. */
# define PROCESS_CONTROL_MODE_WHITELIST_INTEGRITY   0x4

# define IS_PROCESS_CONTROL_MODE_WHITELIST() \
     (TEST(PROCESS_CONTROL_MODE_WHITELIST, DENTRE_OPTION(process_control)))
# define IS_PROCESS_CONTROL_MODE_BLACKLIST() \
     (TEST(PROCESS_CONTROL_MODE_BLACKLIST, DENTRE_OPTION(process_control)))
# define IS_PROCESS_CONTROL_MODE_WHITELIST_INTEGRITY() \
     (TEST(PROCESS_CONTROL_MODE_WHITELIST_INTEGRITY, DENTRE_OPTION(process_control)))
# define IS_PROCESS_CONTROL_ON()                \
     (IS_PROCESS_CONTROL_MODE_WHITELIST() ||    \
      IS_PROCESS_CONTROL_MODE_BLACKLIST() ||    \
      IS_PROCESS_CONTROL_MODE_WHITELIST_INTEGRITY())

void process_control(void);
void process_control_init(void);
#endif

#endif
