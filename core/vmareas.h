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

enum {
    /* these are bitmask flags */
    VECTOR_SHARED        = 0x0001,
    VECTOR_FRAGMENT_LIST = 0x0002, /* for vmareas.c-only use */
    /* never merge adjacent regions */
    VECTOR_NEVER_MERGE_ADJACENT = 0x0004,
    /* results in an assert if a new region overlaps existing */
    VECTOR_NEVER_OVERLAP = 0x0008,
    /* case 10335: if a higher-level lock is being used, set this
     * flag to avoid the redundant vector-level lock
     */
    VECTOR_NO_LOCK       = 0x0010,
};


/* This vector data structure is only exposed here for quick length checks.
 * For external (non-vmareas.c) users, the vmvector_* interface is the
 * preferred way of manipulating vectors.
 *
 * Each vector is kept sorted by area.  Since there are no overlaps allowed
 * among areas in the same vector (they're merged to preserve that), sorting
 * by start_pc or by end_pc produce identical results.
 */
struct vm_area_vector_t
{
	struct vm_area_t *buf;
	int size;			/* capacity */
	int length;			/* num of buf[i] */
	uint flags;			/* VECTOR_* flags */

    /* often thread-shared, so needs a lock
     * read-write lock for performance, and to allow a high-level writer
     * to perform a read (don't need full recursive lock)
     */
	read_write_lock_t lock;

    /* Callbacks to support payloads */
    /* Frees a payload */
	void (*free_payload_func)(void *);


    /* Returns the payload to use for a new region split from the given data's
     * region.
     */
	void *(*split_payload_func)(void *);


    /* Should adjacent/overlapping regions w/ given payloads be merged?
     * If it returns false, adjacent regions are not merged, and
     * a new overlapping region is split (the split_payload_func is
     * called) and only nonoverlapping pieces are added.
     * If NULL, it is assumed to return true for adjacent but false
     * for overlapping.
     * The VECTOR_NEVER_MERGE_ADJACENT flag takes precedence over this function.
     */
	bool (*should_merge_func)(bool adjacent, void *, void *);


    /* Merge adjacent or overlapping regions: dst is first arg.
     * If NULL, the free_payload_func will be called for src.
     * If non-NULL, the free_payload_func will NOT be called.
     */
	void *(*merge_payload_func)(void *dst, void *src);
};


/* vm_area_vectors should NOT be declared statically if their locks need to be
 * accessed on a regular basis.  Instead, allocate them on the heap with this macro:
 */
#define VMVECTOR_ALLOC_VECTOR(v, dc, flags, lockname)			\
	do															\
	{															\
		v = vmvector_creat_vector((dc), (flags));				\
		ASSIGN_INIT_READWRITE_LOCK_FREE((v)->lock, lockname);	\
	}while(0);



/* this routine does NOT initialize the rw lock!  use VMVECTOR_ALLOC_VECTOR instead */
vm_area_vector_t *
vmvector_create_vector(dcontext_t *dcontext, uint flags);

void 
dentre_vm_areas_lock(void);

void 
dentre_vm_areas_unlock(void);

bool 
add_dentre_vm_area(app_pc start, app_pc end, uint prot, bool unmod_image _IF_DEBUG(char *comment));

void mark_dentre_vm_areas_stale(void);

#endif
