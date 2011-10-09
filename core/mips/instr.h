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

/* file "instr.h" -- mips-specific instr_t definitions and utilities */

#ifndef _INSTR_H_
#define _INSTR_H_ 1


typedef enum
{
    REG_ZERO,	//0
    REG_AT,		//1
    REG_V0,		//2,3
    REG_V1,
    REG_A0,		//4-7
    REG_A1,
	REG_A2,
	REG_A3,
	REG_T0,		//8-15
	REG_T1,		
	REG_T2,		
	REG_T3,		
	REG_T4,		
	REG_T5,		
	REG_T6,		
	REG_T7,	
	REG_S0,		//16-23
	REG_S1,
	REG_S2,
	REG_S3,
	REG_S4,
	REG_S5,
	REG_S6,
	REG_S7,
    REG_T8,		//24,25
	REG_T9,	
	REG_K0,		//26,27
	REG_K1,
	REG_GP,		//28
	REG_SP,		//29
	REG_FP,		//30
	REG_RA		//31
}MIPS_REG;


/* Platform-independent full-register specifiers */
#ifdef N64
/* need to be filled up */
#else
# define REG_XZERO REG_ZERO  
# define REG_XAT   REG_AT  
# define REG_XV0   REG_V0  
# define REG_XV1   REG_V1  
# define REG_XA0   REG_A0  
# define REG_XA1   REG_A1  
# define REG_XA2   REG_A2  
# define REG_XA3   REG_A3  
# define REG_XT0   REG_T0
# define REG_XT1   REG_T1
# define REG_XT2   REG_T2
# define REG_XT3   REG_T3
# define REG_XT4   REG_T4
# define REG_XT5   REG_T5
# define REG_XT6   REG_T6
# define REG_XT7   REG_T7
# define REG_XS0   REG_S0
# define REG_XS1   REG_S1
# define REG_XS2   REG_S2
# define REG_XS3   REG_S3
# define REG_XS4   REG_S4
# define REG_XS5   REG_S5
# define REG_XS6   REG_S6
# define REG_XS7   REG_S7
# define REG_XT8   REG_T8
# define REG_XT9   REG_T9
# define REG_XK0   REG_K0
# define REG_XK1   REG_K1
# define REG_XGP   REG_GP
# define REG_XSP   REG_SP
# define REG_XFP   REG_FP
# define REG_XRA   REG_RA
#endif


#endif
