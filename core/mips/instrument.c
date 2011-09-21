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

#include <string.h>
#include <stdlib.h>

#include "../globals.h"
#include "instrument.h"
#include "../options.h"
#include "../utils.h"

#ifdef CLIENT_INTERFACE
/* Acquire when registering or unregistering event callbacks */
DECLARE_CXTSWPROT_VAR(static mutex_t callback_registration_lock,
                      INIT_LOCK_FREE(callback_registration_lock));

/* Structures for maintaining lists of event callbacks */
typedef void (*callback_t)(void);
typedef struct _callback_list_t {
    callback_t *callbacks;      /* array of callback functions */
    size_t num;                 /* number of callbacks registered */
    size_t size;                /* allocated space (may be larger than num) */
} callback_list_t;


/* An array of client libraries.  We use a static array instead of a
 * heap-allocated list so we can load the client libs before
 * initializing DR's heap.
 */
typedef struct _client_lib_t {
    client_id_t id;
    char path[MAXIMUM_PATH];
    /* PR 366195: dlopen() handle truly is opaque: != start */
    shlib_handle_t lib;
    app_pc start;
    app_pc end;
    char options[MAX_OPTION_LENGTH];
    /* We need to associate nudge events with a specific client so we
     * store that list here in the client_lib_t instead of using a
     * single global list.
     */
    callback_list_t nudge_callbacks;
} client_lib_t;

/* these should only be modified prior to instrument_init(), since no
 * readers of the client_libs array (event handlers, etc.) use synch
 */
static client_lib_t client_libs[MAX_CLIENT_LIBS] = {{0,}};
static size_t num_client_libs = 0;


#if defined(CLIENT_INTERFACE) || defined(HOT_PATCHING_INTERFACE)
shlib_handle_t 
load_shared_library(char *name)
{
    return dlopen(name, RTLD_LAZY);
}
#endif


/* This should only be called prior to instrument_init(),
 * since no readers of the client_libs array use synch
 * and since this routine assumes .data is writable.
 */
static void
add_client_lib(char *path, char *id_str, char *options)
{
    client_id_t id;
    shlib_handle_t client_lib;
    DEBUG_DECLARE(size_t i);

    ASSERT(!dynamo_initialized);

    /* if ID not specified, we'll default to 0 */
    id = (id_str == NULL) ? 0 : strtoul(id_str, NULL, 16);

#ifdef DEBUG
    /* Check for conflicting IDs */
    for (i=0; i<num_client_libs; i++) {
        CLIENT_ASSERT(client_libs[i].id != id, "Clients have the same ID");
    }
#endif

    if (num_client_libs == MAX_CLIENT_LIBS) {
        CLIENT_ASSERT(false, "Max number of clients reached");
        return;
    }

    LOG(GLOBAL, LOG_INTERP, 4, "about to load client library %s\n", path);

    client_lib = load_shared_library(path);
    if (client_lib == NULL) {
        char msg[MAXIMUM_PATH*4];
        char err[MAXIMUM_PATH*2];
        shared_library_error(err, BUFFER_SIZE_ELEMENTS(err));
        snprintf(msg, BUFFER_SIZE_ELEMENTS(msg),
                 "\n\tError opening instrumentation library %s:\n\t%s",
                 path, err);
        NULL_TERMINATE_BUFFER(msg);

        /* PR 232490 - malformed library names or incorrect
         * permissions shouldn't blow up an app in release builds as
         * they may happen at customer sites with a third party
         * client.
         */
        /* PR 408318: 32-vs-64 errors should NOT be fatal to continue
         * in debug build across execve chains
         */
        if (strstr(err, "wrong ELF class") == NULL)
            CLIENT_ASSERT(false, msg);
        SYSLOG(SYSLOG_ERROR, CLIENT_LIBRARY_UNLOADABLE, 3, 
               get_application_name(), get_application_pid(), msg);
    }
    else {
        /* version check */
        int *uses_dr_version = NULL;
//        int *uses_dr_version = (int *)
//            lookup_library_routine(client_lib, USES_DR_VERSION_NAME);
//        if (uses_dr_version == NULL ||
//            *uses_dr_version < OLDEST_COMPATIBLE_VERSION ||
//            *uses_dr_version > NEWEST_COMPATIBLE_VERSION) {
//            /* not a fatal usage error since we want release build to continue */
//            CLIENT_ASSERT(false, 
//                          "client library is incompatible with this version of DR");
//            SYSLOG(SYSLOG_ERROR, CLIENT_VERSION_INCOMPATIBLE, 2, 
//                   get_application_name(), get_application_pid());
//        }
//        else {
            size_t idx = num_client_libs++;
            DEBUG_DECLARE(bool ok;)
            client_libs[idx].id = id;
            client_libs[idx].lib = client_lib;
            DEBUG_DECLARE(ok =)
                shared_library_bounds(client_lib, (byte *) uses_dr_version,
                                      &client_libs[idx].start, &client_libs[idx].end);
            ASSERT(ok);

            LOG(GLOBAL, LOG_INTERP, 1, "loaded %s at "PFX"-"PFX"\n",
                path, client_libs[idx].start, client_libs[idx].end);

            strncpy(client_libs[idx].path, path,
                    BUFFER_SIZE_ELEMENTS(client_libs[idx].path));
            NULL_TERMINATE_BUFFER(client_libs[idx].path);

            if (options != NULL) {
                strncpy(client_libs[idx].options, options,
                        BUFFER_SIZE_ELEMENTS(client_libs[idx].options));
                NULL_TERMINATE_BUFFER(client_libs[idx].options);
            }

            /* We'll look up dr_init and call it in instrument_init */
//        }
    }
}

void
instrument_load_client_libs(void)
{
    if (!IS_INTERNAL_STRING_OPTION_EMPTY(client_lib)) {
        char buf[MAX_LIST_OPTION_LENGTH];
        char *path;

        string_option_read_lock();
        strncpy(buf, INTERNAL_OPTION(client_lib), BUFFER_SIZE_ELEMENTS(buf));
        string_option_read_unlock();
        NULL_TERMINATE_BUFFER(buf);

        /* We're expecting path;ID;options triples */
        path = buf;
        do {
            char *id = NULL;
            char *options = NULL;
            char *next_path = NULL;

            id = strstr(path, ";");
            if (id != NULL) {
                id[0] = '\0';
                id++;

                options = strstr(id, ";");
                if (options != NULL) {
                    options[0] = '\0';
                    options++;

                    next_path = strstr(options, ";");
                    if (next_path != NULL) {
                        next_path[0] = '\0';
                        next_path++;
                    }
                }
            }

            add_client_lib(path, id, options);
            path = next_path;
        } while (path != NULL);
    }
}


void 
instrument_thread_init(dcontext_t *dcontext, bool client_thread)
{
    /* Note that we're called twice for the initial thread: once prior
     * to instrument_init() (PR 216936) to set up the dcontext client
     * field (at which point there should be no callbacks since client
     * has not had a chance to register any), and once after
     * instrument_init() to call the client event.
     */
	if(dcontext->client_data == NULL)
		dcontext->client_data = HEAP_TYPE_ALLOC(dcontextm, client_data_t,
												ACCT_OTHER, UNPROTECTED);
	memset(dcontext->client_data, 0x0, sizeof(client_data_t));

	/* need to be filled up  */
}

#endif

