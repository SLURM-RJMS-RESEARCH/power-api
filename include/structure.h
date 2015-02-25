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
 *  The file contains all the functions related to hardware structure queries.
 */

#ifndef __STRUCTURE_H__
#define __STRUCTURE_H__

#ifndef __POWER_API_H__
    #error "Never directly include this file, rather use power_api.h"
#endif

//====-------------------------------------------------------------------------
// Public Functions
//-----------------------------------------------------------------------------

/**
  * The number of actual CPU controlled by the Power API.
  *
  * @param ctx The current library context
  *
  * @return How many CPUs are controlled by the library.
  */
unsigned long pwr_num_phys_cpus(pwr_ctx_t *ctx);

/**
 * Get the number of voltage islands controlled by the Power API. The islands
 * can then be addressed using a number in [0, nb islands).
 * 
 * @param ctx The current library context.
 *
 * @return The number of physical islands available on the machine.
 */
unsigned long pwr_num_phys_islands(pwr_ctx_t *ctx);

/**
 * Returns the id of the island that contains the given CPU.
 *
 * @param ctx The current API context.
 * @param cpu The Linux id of the CPU whose island is searched for.
 *
 * @return The identifier of the island that contains the given CPU core.
 */
unsigned long pwr_island_of_cpu(pwr_ctx_t *ctx, unsigned long cpu);

#endif

