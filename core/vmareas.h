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

#ifndef _VMAREAS_H_
#define _VMAREAS_H_

#include "globals.h"
void 
dentre_vm_areas_lock(void);

void 
dentre_vm_areas_unlock(void);

bool add_dentre_vm_area(app_pc start, app_pc end, uint prot, bool unmod_image _IF_DEBUG(char *comment));

#endif
