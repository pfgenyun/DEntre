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


/* file "dispatch.h" */

#ifndef _DISPATCH_H_
#define _DISPATCH_H_ 1


/* hooks on entry/exit to/from DE */
#define NO_HOOK ((void (*)(void)) NULL)

#define HOOK_ENABLED_HELPER SELF_PROTECT_ON_CXT_SWITCH

#define HOOK_ENABLED (HOOK_ENABLED_HELPER || INTERNAL_OPTION(single_thread_in_DE))

#define ENTER_DE_HOOK (HOOK_ENABLED ? entering_dentre : NO_HOOK)
#define EXIT_DE_HOOK  (HOOK_ENABLED ? exiting_dentre : NO_HOOK)

#define ENTERING_DE() do {   \
        if (HOOK_ENABLED)    \
            ENTER_DE_HOOK(); \
    } while (0);


#endif
