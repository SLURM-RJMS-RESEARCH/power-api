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
 *  The file contains all the functions related to frequencies.
 */

#ifndef __DVFS_H__
#define __DVFS_H__

#ifndef __POWER_API_H__
    #error "Never directly include this file, rather use power_api.h"
#endif

//====-------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

/**
  * Number of discrete speed levels supported by a voltage island 
  * 
  * The slowest speed level is '<code>0</code>' and speed levels increase 
  * monotonically until the fastest speed level at 
  * '<code>num_speed_levels - 1</code>'.
  *
  * @todo Map speed level to integer between 0 and 100 inclusive?
  * 
  * @param ctx The current library context.
  * @param island  The ID of the island to get speed level count for
  *
  * @return The number of speed levels for that island.
  */
unsigned int pwr_num_speed_levels(pwr_ctx_t *ctx, unsigned long island);
  

/**
  * The current speed level of a voltage island
  * 
  * @param ctx The current library context.
  * @param island  The island to check speed level on.
  *
  * @return The current speed level. The value returned is in range [0, num speed).
  */
unsigned int pwr_current_speed_level(pwr_ctx_t *ctx, unsigned long island);


/**
  * Requests speed level change on a voltage island 
  * 
  * Requires <code>0 <= new_level < num_speed_levels</code>.
  * 
  * @param ctx The current library context.
  * @param island  The island to change speed level on
  * @param new_level  The requested speed level
  */
void pwr_request_speed_level(pwr_ctx_t *ctx, unsigned long island,
    unsigned int new_level);


/**
  * Request a speed level modification of the given island 
  *
  * <code>delta</code> can be positive or negative.
  *
  * @param ctx The current library context.
  * @param island  The island to speed up or slow down
  * @param delta  The number of speed levels to increment by
  */
void pwr_increase_speed_level(pwr_ctx_t *ctx, unsigned long island, int delta);

/**
  * Calculates the cost of switching speed levels
  *
  * @todo Decide on unit for agility. Currently ns because of cpufreq default.
  *       If agility in cycles, at what speed level? Full speed? to_speed?
  *
  * @param ctx The current library context.
  * @param island  The island of interest
  * @param from_level  Starting speed level
  * @param to_level  Finishing speed level
  *
  * @return The agility value.
  */
long pwr_agility(pwr_ctx_t *ctx, unsigned long island, unsigned int from_level,
    unsigned int to_level);

/**
  * Requests a voltage level modification of the given island 
  *
  * <code>delta</code> can be positive or negative.
  *
  * @todo IMPLEMENT
  * @todo Most architectures don't make this adjustment available. How to deal 
  *       with this fact?
  *
  * @param ctx The current library context.
  * @param island  The island to change voltage on
  * @param delta  The number of voltage levels to increment by
  */
void pwr_increase_voltage(pwr_ctx_t *ctx, unsigned long island, int delta);

#endif
