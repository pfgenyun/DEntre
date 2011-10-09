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


#ifndef _ASM_DEFINES_ASM_
#define _ASM_DEFINES_ASM_ 1


/* Preprocessor macro definitions shared among all .asm files.
 * Since cpp macros can't generate newlines we have a later
 * script replace @N@ for us.
 */

#define START_FILE .text
#define END_FILE /* nothing */
#define DECLARE_FUNC(symbol) \
.align 0 @N@\
.global symbol @N@\
.hidden symbol @N@\
.type symbol, %function
#define DECLARE_EXPORTED_FUNC(symbol) \
.align 0 @N@\
.global symbol @N@\
.type symbol, %function
#define END_FUNC(symbol) /* nothing */
#define DECLARE_GLOBAL(symbol) \
.global symbol @N@\
.hidden symbol
#define GLOBAL_LABEL(label) label
#define ADDRTAKEN_LABEL(label) label
#define WORD word ptr
#define DWORD dword ptr
#define QWORD qword ptr

#define CALLC1(callee, p1)    /* need to be filled up */


#endif
