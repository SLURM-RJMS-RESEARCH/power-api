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
  * @file power_api.h Interface of the Power API for compiler-assisted 
  * energy-proportional scheduling.
  *
  * \mainpage Reservoir Labs Power API
  *
  * \section over Overview
  * The Power API is divided into high-level and low-level interfaces. The 
  * high-level interface allows a programmer or compiler to specify power and 
  * energy management goals and choose strategies to achieve 
  * these goals. The implementation of strategies and the means to track and 
  * enforce power management goals is the responsibility of the implementor of
  * the API on a platform.
  *
  * The low-level interface of the Power API is close to the hardware and 
  * provides direct control over DVFS settings for voltage islands. It also
  * provides access to energy probes, whenever they are available on the
  * platform.
  *
  * The high-level goals of the API are as follows:
  *   - Provide a cross-platform interface for compilers and programmers to
  *     control and measure power and energy consumption
  *   - Be concise, intuitive and make minimal assumptions about the 
  *     underlying hardware
  *   - Maintain a level of abstraction high enough to allow implementation
  *     in terms of DARPA PERFECT team APIs. 
  *   - Take advantage of features provided by leading edge task-based runtime 
  *     environments 
  *
  * The API assumes that any system components bound to the same voltage and 
  * frequency settings are grouped together in an island. An island is
  * the atomic unit for which frequency and voltage can be modified through 
  * the Power API. In the current library state, the energy is also measured
  * at the granularity of the island, although this may change in the future.
  *
  * The library is not multithread safe.
  * 
  * \section hl_desc High-Level Interface
  * The high-level interface of the Power API is accessed through an 
  * initialization function and the data structures passed to this function.
  *
  * The programmer (or compiler) must set up three things at Power API 
  * initialization:   

  *   - A model of hardware behavior
  *   - A speed adjustment policy
  *   - A scheduling policy
  *
  * @see pwr_initialize()
  *
  * Once configured, the combination of these 3 elements guides power and 
  * energy management decisions at program execution time. The 3 elements 
  * and associated data structures are described in the following sections.
  * Power API implementations must define a default for each element on the 
  * targeted architecture.
  *
  * \subsection hw_behav Hardware Behavior
  * This defines the valid combinations of voltage and speed / speed level, 
  * possibly as functions of external factors such as the current temperature.
  * Hardware behavior may be changed by hardware, software, or a combination of 
  * both.
  *
  * Consider a near-threshold voltage architecture that may trade accuracy for power
  * savings when voltage drops near threshold. An application that is resillient 
  * to errors would use a hardware behavior that allowed to use all voltage /
  * frequency combinations supported by the architecture. An application that
  * demands accurate results would use a hardware behavior that limited available 
  * voltage-frequency combinations to those well above threshold voltage.
  * 
  * @todo Clarify hardware behavior relationship with temperature / external 
  *       factors.
  * 
  *  
  * \subsection speed_policy Speed Policy
  * This defines a blanket policy for determining the speed level at which 
  * voltage islands are set.
  *
  * Rather than exposing a notion of frequency, we expose a notion of speed 
  * level. This decision addresses the following:
  *  - Permissible frequencies at discrete values
  *  - Heterogeneity of architectures. Two different chips may have the same 
  *    frequency but they won't have the same speed level (i.e., they won't 
  *    execute the same piece of code in the same amount of time).
  *
  * In the Power API, we define frequency, speed level, and speed:
  *
  *  - Frequency: The clock rate of a voltage island, in KHz
  *  - Speed: A real number that corresponds to the absolute performance of a
  *           voltage island
  *  - Speed Level: An integer greater than or equal to -1 that identifies a 
  *                 legal combination of voltage and frequency for a voltage 
  *                 island
  *
  * @see speed_policies
  * @see pwr_num_speed_levels()
  * @see pwr_request_speed_level()
  * @see pwr_current_speed_level()
  * @see speed_policy_t
  *
  * 
  * 
  * \subsection scheduling Scheduling Policy
  * This defines a blanket policy for the run-time assignment of tasks to 
  * processing elements. 
  *
  * @see scheduling_policies
  * @see scheduling_policy_t
  *
  * @todo More options for scheduling policy. Specifically, space and time 
  *       mapping.
  * @todo Clarify definition of task, processing element.
  *
  *
  * 
  * \section ll_desc Low-Level Interface
  * \subsection speedlevel Speed Level
  * This interface allows the user to modify the speed level and voltage of an 
  * island. Speed level and voltage are not necessarily independent, so 
  * modifying one may modify the other.
  *
  * A user is expected to be interested in upping the speed level or lowering 
  * the voltage knowing that the corresponding power consumption and speed will 
  * be negatively affected. An advanced user or compiler familiar with both 
  * island and application characteristics could modify speed and voltage in 
  * the same direction and still achieve power or performance gains. The goal 
  * of the low-level interface is to enable these modifications. 
  *
  * \subsection energy Energy Measurement
  * The low-level interface also exposes energy measurement capabilities. That
  * capability allows energy profiling of programs and provides feedback to the
  * user whenever frequency or voltage has been changed.
  *
  * @see pwr_start_energy_count()
  * @see pwr_stop_energy_count()
  *
  * 
  * 
  * \subsection agility Agility
  * Agility is defined as the best and worst case amount of time it takes to 
  * switch from one speed level to another on a voltage island.
  * 
  * @see pwr_agility()
  * 
  * \section err_check Error checking
  * There are numerous situations in which hardware access is not possible. To
  * detect those situations and help the user fixing potential configuration
  * issues, the API provides convenient error checking. The last error that 
  * occured is stored in the library context and can be accessed either as
  * a numeric error code or as a string for printing.
  *
  * @see pwr_error()
  * @see pwr_strerror()
  * 
  *
  *
  * 
  * 
  * \section example Example Code
  * \code
#include <power_api.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    long num_islands;
    pwr_ctx_t *ctx;

    // Initialize with default values
    ctx = pwr_initialize(NULL, NULL, NULL);

    // Get the number of islands
    num_islands = pwr_num_phys_islands(ctx);

    // Set each island to max speed
    for (long i = 0; i < num_islands; ++i) {
        unsigned int num_speed_lvls = pwr_num_speed_levels(ctx, i);

        // set maximum frequency
        pwr_request_speed_level(ctx, i, num_speed_lvls - 1);

        // check for errors
        if (pwr_error(ctx) != PWR_OK) {
            printf("Error while setting the frequency: %s\n",
                pwr_strerror(ctx));
        }
    }

    // Finalize
    pwr_finalize(ctx);

    return 0;    
}
\endcode
  * @author M. Deneroff <deneroff@reservoir.com>
  * @author Benoit Meister <meister@reservoir.com>
  * @author Tom Henretty <henretty@reservoir.com>
  * @author Athanasios Konstantinidis <konstantinidis@reservoir.com>
  * @author Benoit Pradelle <pradelle@reservoir.com>
  */

#ifndef __POWER_API_H__
#define __POWER_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

//====-------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

/** \addtogroup limits Limitations
  * Describes various limitations imposed by the API.
  * @{
  */
/** The maximum number of physical CPU in a system supported by the Power API */
#define PWR_MAX_PHYS_CPU (1024*1024)

/** The maximum number of physical voltage islands in a system supported by the Power API */
#define PWR_MAX_PHYS_ISLANDS (PWR_MAX_PHYS_CPU)

/** The maximum number of virtual voltage islands in a system supported by the Power API */
#define PWR_MAX_VIRT_ISLANDS (PWR_MAX_PHYS_ISLANDS) // reserved for future

/** The maximum number of CPU in a physical voltage island supported by the Power API */
#define PWR_MAX_CPU_PER_PHYS_ISLAND (PWR_MAX_PHYS_CPU)

/** The maximum number of CPU in a virtual voltage island supported by the Power API */
#define PWR_MAX_CPU_PER_VIRT_ISLAND (PWR_MAX_PHYS_CPU)

/** The maximum number of speed levels supported by the Power API */
#define PWR_MAX_SPEED_LEVELS (1024*1024)

/** @} */

//====-------------------------------------------------------------------------
// Error codes
//-----------------------------------------------------------------------------

/** \addtogroup errors Error Codes
  * List of all the possible error codes.
  * The context contains the last error that occurred. It can be retrived by
  * pwr_error() or pwr_strerror().
  * @{  
  */

/** Error: Feature unsupported by hardware */
#define PWR_ARCH_UNSUPPORTED (-3)

/** Error: Feature not implemented */
#define PWR_UNIMPLEMENTED (-2)

/** Error: Power API has not been initialized */
#define PWR_UNINITIALIZED (-1)

/** Command executed successfully  */
#define PWR_OK (0)

/** Error: Unspecified error */
#define PWR_ERR (1)

/** Error: Feature temporarily unavailable */
#define PWR_UNAVAILABLE (2)

/** Error: Request was denied */
#define PWR_REQUEST_DENIED (4)

/** Error: Unspecified error during initialization */
#define PWR_INIT_ERR (5)

/** Error: Unspecified error during finalization */
#define PWR_FINAL_ERR (6)

/** Error: Attempt to initialize API after it has been initialized */
#define PWR_ALREADY_INITIALIZED (7)

/** Error: Unspecified input / output error */
#define PWR_IO_ERR (8)

/** Error: Speed level not supported by hardware or API */
#define PWR_UNSUPPORTED_SPEED_LEVEL (9)

/** Error: Voltage not supported by hardware or API */
#define PWR_UNSUPPORTED_VOLTAGE (10)

/** Error: Feature is already set to minimum or maximum value */
#define PWR_ALREADY_MINMAX (11)

/** Error: Request denied, over energy budget */
#define PWR_OVER_E_BUDGET (12) // reserved for future

/** Error: Request denied, over power budget */
#define PWR_OVER_P_BUDGET (13) // reserved for future

/** Error: Request denied, over thermal budget */
#define PWR_OVER_T_BUDGET (14) // reserved for future

/** Error: Specified island does not exist */
#define PWR_INVALID_ISLAND (15)

/** Error: Unspecified error when changing voltage and/or frequency */
#define PWR_DVFS_ERR (16)

/** @} */

/** \addtogroup units Units
  * @{  
  */

/** Efficiency in Joules/Flop */
typedef double efficiency_t;

/** Voltage in Volts */
typedef double voltage_t;

/** Speed in UNDEFINED units */
typedef long speed_t;

/** Unit-less speed level */
typedef long speed_level_t;

/** Frequency in KHz */
typedef long freq_t;

/** Agility in UNDEFINED units */
typedef long agility_t;

/** Power in Watts */
typedef long power_t;

/** Energy in Joules */
typedef double energy_t;

/** @} */

 
//====-------------------------------------------------------------------------
// Modules
//-----------------------------------------------------------------------------

/**
 * \addtogroup modules Modules
 * Modules-related features. The functionalities in the API are provided by
 * different modules. Every module has an id which uniquely identifies it. A
 * module works independently from the others and some of them may be available
 * at runtime while others may not be loaded.
 *
 * @see pwr_is_initialized()
 *
 * @{
 */

/**
 * A module identifier
 */
typedef unsigned int pwr_module_id_t;

/** All the module ids */
enum pwr_module_id_t {
    PWR_MODULE_STRUCT = 0,  /**< Hardware structure discovery */
    PWR_MODULE_DVFS,        /**< DVFS functions */
    PWR_MODULE_ENERGY,      /**< Energy measurement */
    PWR_MODULE_HIGH_LEVEL,  /**< High level interface */
    PWR_NB_MODULES          /**< Number of existing modules */ 
};

/** @} */

/** An opaque library context structure */
typedef struct pwr_ctx pwr_ctx_t;

#include "structure.h"
#include "dvfs.h"
#include "energy.h"
#include "high-level.h"

//====-------------------------------------------------------------------------
// Setup / shutdown
//-----------------------------------------------------------------------------

/** \addtogroup init Initialization code
  * @{  
  */

/**
  * Checks if the Power API has been initialized
  *
  * @param ctx The current library context
  * @param module The id of the module to test
  *
  * @return True if the given module has been correctly initialized, false
  *  otherwise.
  */
bool pwr_is_initialized(const pwr_ctx_t *ctx, const pwr_module_id_t module);


/**
  * Allocates resources used by the Power API 
  *
  * Must be called before any other function in the Power API can be used.
  *
  * @param hw_behavior Reserved for future use
  * @param speed_policy Reserved for future use
  * @param scheduling_policy Reserved for future use
  *
  * @return A new context valid to be used by other Power API calls. 
  */
pwr_ctx_t *pwr_initialize(void* hw_behavior, void* speed_policy, 
    void* scheduling_policy);


/**
  * Frees resources used by the Power API.
  * 
  * Must be called after all Power API functions have been called. Reusing the
  * context after calling that method leads to undefined results.
  *
  * @param ctx The current library context.
  */
void pwr_finalize(pwr_ctx_t *ctx);

/**
 * Returns the last error code that was set on the given context.
 *
 * @param ctx The current library context.
 *
 * @return The code of the last error that occured.
 */
int pwr_error(const pwr_ctx_t *ctx);


/**
 * Returns a string describing the last error occuring in the library.
 *
 * @param ctx The current library context
 *
 * @return A string describing the last error that occured.
 */
const char *pwr_strerror(const pwr_ctx_t *ctx);

/** @} */

#ifdef __cplusplus
}
#endif

#endif

