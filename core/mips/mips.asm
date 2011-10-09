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


/*
 * mips.asm - mips specific assembly and trampoline code
 */

#include "asm_defines.asm"

/* push all register state to mcontext */
#define PUSH_DE_MCONTEXT(pc)	/*need to be filled up */
/* pop all register from mcontext to register*/
#define POP_DE_MCONTEXT(pc)		/*need to be filled up */

/*
 * dentre_app_take_over - Causes application to run under DEntre
 * control.  DEntre never releases control.
 */
//	DECLARE_EXPORTED_FUNC(dentre_app_take_over)
//GLOBAL_LABEL(dentre_app_take_over)

	/* grab exec state and pass as param in a dr_mcontext_t struct */
	PUSH_DE_MCONTEXT([REG_XSP])  /* push return address as pc */
	
	/* do the rest in C */
//	ld      REG_XA0, REG_XSP /* stack grew down, so dr_mcontext_t at tos */
	CALLC1(dentre_app_take_over_helper, REG_XA0)
	
	/* if we come back, then DE is not taking control so 
	 * clean up stack and return */
//	add      REG_XSP, DE_MCONTEXT_SIZE
	POP_DE_MCONTEXT([REG_XSP])  /* pop all state to register and change ip*/
	END_FUNC(dentre_app_take_over)

