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
 *  The file contains all the high level functions, mostly not implemented.
 */

#ifndef __HIGH_LEVEL_H__
#define __HIGH_LEVEL_H__

#ifndef __POWER_API_H__
    #error "Never directly include this file, rather use power_api.h"
#endif

/**
  * Current energy efficiency of an island (Joules / Flop)
  *  
  * @todo IMPLEMENT
  * @todo What time period to sample power over? 
  * @todo Is Joules / Flop the best unit?
  * @todo Corresponding function for power efficiency?
  *
  * @param ctx The current library context.
  * @param island  The voltage island to calculate efficiency for
  * @param efficiency[out]  The power efficiency of the island in Joules / Watt
  */
void pwr_efficiency(pwr_ctx_t *ctx, unsigned long island,
                    efficiency_t* efficiency);

/**
  * Sets the importance of power efficiency for the given task
  * 
  * @todo IMPLEMENT
  * @todo How to represent tasks?
  * @todo Create a corresponding register_task() method that returns a task id?
  * @todo Take in a task_id for setting power and performance efficiency 
  *       priorities?
  * @todo Enforce <code>power_priority + performance_priority = 100</code>?
  *
  * @param ctx The current library context.
  * @param task  A task managed by the Power API
  * @param priority  An integer indicating the importance of power efficiency 
  *                  for the given task. Possible values are between 0 
  *                  (power efficiency is lowest priority) and 100 (power 
  *                  efficiciency is highest priority) inclusive.
  */
void pwr_set_power_priority(pwr_ctx_t *ctx, void* task, int priority);

/**
  * Sets the importance of performance for the given task
  * 
  * @todo IMPLEMENT
  *
  * @param ctx The current library context.
  * @param task  A task managed by the Power API
  * @param priority  An integer indicating the importance of performance
  *                  for the given task. Possible values are between 0 
  *                  (power efficiency is lowest priority) and 100 (power 
  *                  efficiciency is highest priority) inclusive.
  */
void pwr_set_speed_priority(pwr_ctx_t *ctx,void* task, int priority);


#endif

