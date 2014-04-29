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
#ifndef ECOUNT_H
#define ECOUNT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>


/** Number of PAPI events monitored */
#define NUM_EVENTS (4)

/** Maximum number of characters used for small strings, e.g. 'joules' */
#define EC_MIN_STR_LEN 64

/** Maximum number of characters used for large strings, e.g. 
    'PACKAGE_ENERGY:PACKAGE0' */
#define EC_MAX_STR_LEN 128

/**
  * Definition of struct to hold energy consumption and timing information.
  */
typedef struct {
    double start_time_ns;
    double stop_time_ns;
    long long values[NUM_EVENTS];
	char names[NUM_EVENTS][EC_MAX_STR_LEN];
	char units[NUM_EVENTS][EC_MIN_STR_LEN];
} e_data;

/**
  * Allocate resources used to collect energy counter data
  */
void ec_initialize();

/**
  * Begin collection of energy counter data
  */
void ec_start_e_counters();

/**
  * End collection of energy counter data
  */
void ec_stop_e_counters();

/**
  * Reset all energy counters to zero
  */
void ec_reset_e_counters(); 

/**
  * Get energy counter data
  */
e_data* ec_read_e_counters();

/**
  * Release resources used to gather energy counter data
  */
void ec_finalize();

#ifdef __cplusplus
}
#endif

#endif
