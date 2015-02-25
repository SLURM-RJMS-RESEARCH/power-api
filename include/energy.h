/*
  * Copyright 2013-15 Reservoir Labs, Inc.
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

/**
 * @file
 * This file contains the functions related to energy counters.
 */

#ifndef __ENERGY_H__
#define __ENERGY_H__

#ifndef __POWER_API_H__
    #error "Never directly include this file, rather use power_api.h"
#endif

//====-------------------------------------------------------------------------
// Public data types
//-----------------------------------------------------------------------------

/** 
 * Energy measurement results.
 * The structure is returned by pwr_stop_energy_count(). The result is made of
 * an execution time and various hardware counter values. The number of 
 * hardware counters in the result depends on what is available on your machine.
 * Only energy is measured by the counters, thus valid units are "J" and "nJ".
 */
typedef struct {
    double duration;        //!< Execution time, in s.
    unsigned long nbValues; //!< How many values are profiled
    long long *values;      //!< Counter values
	char **names;           //!< Counter names
	char **units;           //!< Counter units
} pwr_emeas_t;


//====-------------------------------------------------------------------------
// Public Functions
//-----------------------------------------------------------------------------

/**
 * Starts the energy counters, measuring the current energy consumption.
 *
 * @param ctx The current library context.
 */
void pwr_start_energy_count(pwr_ctx_t *ctx);

/**
 * Stops the energy counters and retrive the energy consummed since the last
 * call to pwr_start_energy.
 *
 * @param ctx The current library context.
 *
 * @return A pointer to energy measurement results.
 */
const pwr_emeas_t *pwr_stop_energy_count(pwr_ctx_t *ctx);

#endif

