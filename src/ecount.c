/**
  * Copyright 2013-14 Reservoir Labs, Inc.
  *
  *	Licensed under the Apache License, Version 2.0 (the "License");
  *	you may not use this file except in compliance with the License.
  *	You may obtain a copy of the License at
  *
  *		http://www.apache.org/licenses/LICENSE-2.0
  *
  *	Unless required by applicable law or agreed to in writing, software
  *	distributed under the License is distributed on an "AS IS" BASIS,
  *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  *	See the License for the specific language governing permissions and
  *	limitations under the License.
  */

#include "ecount.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <papi.h>

#define TRUE 1
#define FALSE 0

/**
  * Counter names. These names may vary across different Intel chips. Xeon 
  * series chips may have DRAM energy counters, while Core series chips may
  * have GPU energy counters. See papi_native_avail for the events supported 
  * by your processor.
  */
char ec_events[NUM_EVENTS][BUFSIZ] = {                                   
    "PACKAGE_ENERGY:PACKAGE0",                                                  
    "PACKAGE_ENERGY:PACKAGE1",                                                  
    "DRAM_ENERGY:PACKAGE0",                                                     
    "DRAM_ENERGY:PACKAGE1",                                                     
};    

/** Is the energy counter library initialized? */
int ec_is_initialized = FALSE;

/** Pointer to container for energy counter data */
e_data* counters;

/** Event set identifier (used by PAPI) */
int event_set = PAPI_NULL;

/**
  * Allocate resources used to collect energy counter data
  */
void ec_initialize() {
    int ret, num_comp, cid, code;
    const PAPI_component_info_t *comp_info = NULL;
	PAPI_event_info_t evinfo;

	if ( ec_is_initialized == TRUE ) {
		return;
	} else {
		ec_is_initialized = TRUE;
	}

	// Allocate memory for results
    counters = (e_data*)malloc(sizeof(e_data));
    // Initialize PAPI
	if ( !PAPI_is_initialized() ) {
    	ret = PAPI_library_init(PAPI_VER_CURRENT);
    	if (PAPI_VER_CURRENT != ret ) {
        	fprintf(stderr, "[PAPI-RAPL] init failed: %d\n", ret);
        	exit(1);
    	}
	}

    // Search PAPI components for RAPL (Intel energy counting) events
    num_comp = PAPI_num_components();

    for (cid = 0; cid < num_comp; ++cid) {
        comp_info = PAPI_get_component_info(cid);
        if (NULL == comp_info ) {
            fprintf(stderr, "[PAPI-RAPL] get component info failed\n");
            exit(1);
        }
        if (strstr(comp_info->name, "rapl")) {
            if (0 == comp_info->num_native_events) {
                fprintf(stderr, "[PAPI-RAPL] no native events found\n");
            }
            break;
        }
    }

    if (cid == num_comp) {
        fprintf(stderr, "[PAPI-RAPL] no rapl events found\n");
    }

    // Create PAPI event set
    ret = PAPI_create_eventset(&event_set);
    if (PAPI_OK != ret) {
        fprintf(stderr, "[PAPI-RAPL] could not create event set: %d: %s\n", ret, PAPI_strerror(ret));
    }

    for (int i = 0; i < NUM_EVENTS; ++i) {
        ret = PAPI_add_named_event(event_set, ec_events[i]);
        if (PAPI_OK != ret) {
            fprintf(stderr, "[PAPI-RAPL] could not add event: %s\n", ec_events[i]);
        } else {
			PAPI_event_name_to_code( ec_events[i], &code );
			PAPI_get_event_info(code,&evinfo);
			strncpy(counters->units[i], evinfo.units, PAPI_MIN_STR_LEN);
			strncpy(counters->names[i], ec_events[i], PAPI_MAX_STR_LEN);
		}
    }
}

/**
  * Begin collection of energy counter data
  */
void ec_start_e_counters() {
    ec_reset_e_counters();
    counters->start_time_ns = PAPI_get_real_nsec();
    PAPI_start(event_set);
}

/**
  * End collection of energy counter data
  */
void ec_stop_e_counters() {
    PAPI_stop(event_set, counters->values);
    counters->stop_time_ns = PAPI_get_real_nsec();
}


/**
  * Resets all energy counters to zero
  */
void ec_reset_e_counters() {
    PAPI_reset(event_set);
}

/**
  * Get energy counter data. Must be called after initialize(), 
  * start_e_counters() and stop_e_counters(). 
  *
  *
  * @return A pointer to energy counter data. The counter data structure is 
  *         deallocated when finalize() is called.
  */
e_data* ec_read_e_counters() {
    return counters;
}

/**
  * Release resources used to gather energy counter data
  */
void ec_finalize() {
	if ( ec_is_initialized == TRUE ) {
		PAPI_cleanup_eventset(event_set);
		PAPI_destroy_eventset(&event_set);
		event_set = PAPI_NULL;
		ec_is_initialized = FALSE;
    	free(counters);
	}
}
