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
  * provides direct control over DVFS settings for voltage islands. 
  *
  * The high-level goals of the API are as follows:
  *   - Provide a cross-platform interface for compilers and programmers to
  *     control power and energy consumption
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
  * the Power API.
  *
  * 
  * \section hl_desc High-Level Interface
  *
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
  * 
  * 
  * \subsection hw_behav Hardware Behavior
  * 
  * This defines the valid combinations of voltage and speed / speed level, 
  * possibly as functions of external factors such as the current temperature.
  * Hardware behavior may be changed by hardware, software, or a combination of 
  * both.
  * 
  * @see hw_behavior_t
  * @see pwr_hw_behavior()
  * @see pwr_change_hw_behavior()
  *
  * @todo Clarify hardware behavior relationship with temperature / external 
  *       factors.
  *
  * Consider a near-threshold voltage architecture that may trade accuracy for power
  * savings when voltage drops near threshold. An application that is resillient 
  * to errors would use a hardware behavior that allowed to use all voltage /
  * frequency combinations supported by the architecture. An application that
  * demands accurate results would use a hardware behavior that limited available 
  * voltage-frequency combinations to those well above threshold voltage.
  * 
  * 
  *  
  * \subsection speed_policy Speed Policy
  * 
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
  * @see pwr_request_speed_level()
  * @see pwr_modify_speed_level()
  * @see pwr_current_speed_level()
  * @see speed_policy_t
  *
  * 
  * 
  * \subsection scheduling Scheduling Policy
  *
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
  *
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
  * 
  * \subsection speedlevel Speed Level
  * 
  * \subsection agility Agility
  * Agility is defined as the best and worst case amount of time it takes to 
  * switch from one speed level to another on a voltage island.
  * 
  * @see pwr_agility()
  * 
  * 
  * 
  * 
  * 
  * \section example Example Code
  *
  * \code
#include <power_api.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    retval_t retval;
    long num_islands;
    island_id_t islands;

    // Initialize with default values
    retval = pwr_initialize(NULL, NULL, NULL);

    // Get island ids
    retval = pwr_num_islands(&num_islands);
    islands = (island_id_t*)malloc(num_islands*sizeof(island_id_t));
    retval = pwr_islands(islands);

    // Set each island to max speed
    for (long i = 0; i < num_islands; ++i) {
        retval = pwr_request_speed_level(islands[i], PWR_MAX_SPEED_LEVEL);
    }

    // Finalize
    retval = pwr_finalize();
    return 0;    
}
\endcode
  * 
  * 
  * 
  * 
  * 
  * 
  *
  */
#ifndef POWERAPI_H
#define POWERAPI_H
#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>

//====-------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
/** \addtogroup limits Limitations
  * @{  
  */

/** The maximum number of voltage islands in a system supported by the Power API */
#define PWR_MAX_ISLANDS (1024L*1024L*1024L)

/** The maximum number of speed levels supported by the Power API */
#define PWR_MAX_SPEED_LEVELS (1024L*1024L)
/** @} */



/** \addtogroup speed_control Speed Levels
  * @{
  */
/** Power to voltage island is off */
#define PWR_POWER_GATE (-1)

/** Clock signal to voltage island is off */
#define PWR_CLOCK_GATE ( 0)

/** Voltage island minimum speed level */
#define PWR_MIN_SPEED_LEVEL (1)

/** Voltage island maximum speed level */
#define PWR_MAX_SPEED_LEVEL (LONG_MAX)
/** @} */



/** \addtogroup speed_policies Speed Policies
  * @{  
  */
/** Pin all voltage islands to minimum speed level */
#define PWR_SPEED_FIXED_MIN (1)

/** Pin all voltage islands to maximum speed level */
#define PWR_SPEED_FIXED_MAX (2)

/** Dynamically adjust speed level of voltage island to meet demand */
#define PWR_SPEED_AS_NEEDED (3)

/** Dynamically adjust speed level with Power API calls only */
#define PWR_SPEED_LOWLEVEL_API (4)
/** @} */



/** \addtogroup scheduling_policies Scheduling Policies
  * @{  
  */
/** Distribute tasks across processing elements such that hardware resource 
    sharing between tasks is minimized */
#define PWR_SCHED_DISTRIBUTE (1)

/** Distribute tasks across processing elements such that hardware resource 
    sharing between tasks is maximized */
#define PWR_SCHED_CONCENTRATE (2)

/** Distribute tasks across processing elements randomly */
#define PWR_SCHED_RANDOM (3)

/** Use energy proportional scheduling for space-time task mapping */
#define PWR_SCHED_EPS (4)
/** @} */

//====-------------------------------------------------------------------------
// Typedefs
//-----------------------------------------------------------------------------

/** \addtogroup id Unique Identifiers
  * @{  
  */
/** Integral unique identifier of a voltage island */
typedef long island_id_t;

/** @} */

/** \addtogroup units Units
  * @{  
  */
/** Voltage in volts */
typedef double voltage_t;

/** Frequency in KHz */
typedef long freq_t;

/** Speed */
typedef long speed_t;

/** Unit-less speed level */
typedef long speed_level_t;

/** Agility in ns */  
typedef long agility_t;

/** Power in watts */
typedef long power_t;

/** Energy in joules or microjoules (see function documentation) */
typedef long energy_t;

/** Time in seconds or nanoseconds (see function documentation) */
typedef long timestamp_t;
/** @} */

//====-------------------------------------------------------------------------
// High-level interface data structures
//-----------------------------------------------------------------------------
/** \addtogroup hli High-Level Interface Data Structures
  * @{  
  */

/**
  * Defines legal (island_id, voltage, frequency, speed, speed_level) tuples
  */
typedef struct hw_behavior {
    /** Total number of (island_id, voltage, frequency, speed, speed_level) tuples */
    long num_tuples;
    /** Island that this tuple applies to (required) */
    island_id_t* island;
    /** Voltage value of the tuple (required) */
    voltage_t* volts;
    /** Frequency value of the tuple (required) */
    freq_t* freq;
    /** Speed value of the tuple (optional) */
    speed_t* speed;
    /** Speed level value of the tuple (required) */
    speed_level_t* speed_level;
} hw_behavior_t;

/**
  * Defines policy for determining speed.
  */
typedef long speed_policy_t;
    

/**
  * Defines policy for scheduling.
  */
typedef long scheduling_policy_t; 

/** @} */


//====-------------------------------------------------------------------------
// Error codes
//-----------------------------------------------------------------------------

/** \addtogroup errors Error Codes
  * @{  
  */

/** Error codes returned by API functions */
typedef int  retval_t;

/** Feature unsupported by hardware */
#define PWR_ARCH_UNSUPPORTED (-3)

/** Feature not implemented */
#define PWR_UNIMPLEMENTED (-2)

/** Power API has not been initialized */
#define PWR_UNINITIALIZED (-1)

/** Command executed successfully  */
#define PWR_OK (0)

/** Unspecified error */
#define PWR_ERR (1)

/** Feature temporarily unavailable */
#define PWR_UNAVAILABLE (2)

/** Request was denied */
#define PWR_REQUEST_DENIED (4)

/** Unspecified error during initialization */
#define PWR_INIT_ERR (5)

/** Unspecified error during finalization */
#define PWR_FINAL_ERR (6)

/** Attempt to initialize API after it has been initialized */
#define PWR_ALREADY_INITIALIZED (7)

/** Unspecified input / output error */
#define PWR_IO_ERR (8)

/** Speed level not supported by hardware or API */
#define PWR_UNSUPPORTED_SPEED_LEVEL (9)

/** Voltage not supported by hardware or API */
#define PWR_UNSUPPORTED_VOLTAGE (10)

/** Feature is already set to minimum or maximum value */
#define PWR_ALREADY_MINMAX (11)

/** Request denied, over energy budget */
#define PWR_OVER_E_BUDGET (12) // reserved for future

/** Request denied, over power budget */
#define PWR_OVER_P_BUDGET (13) // reserved for future

/** Request denied, over thermal budget */
#define PWR_OVER_T_BUDGET (14) // reserved for future

/** Specified island does not exist */
#define PWR_INVALID_ISLAND (15)

/** Unspecified error when changing voltage and/or frequency */
#define PWR_DVFS_ERR (16)

/** Requested value overflow */
#define PWR_OVERFLOW_ERR (17)
/** @} */

//====-------------------------------------------------------------------------
// High-Level Interface
//-----------------------------------------------------------------------------

/** \addtogroup highlevel High-Level Interface
  * @{  
  */

/**
  * Checks if the Power API has been initialized
  *
  * @param is_initialized  <code>TRUE</code> if initialized, <code>FALSE</code> otherwise
  *
  * @retval #PWR_OK  All cases
  */
retval_t pwr_is_initialized(int* is_initialized);


/**
  * Allocates resources used by the Power API 
  *
  * Must be called before any other function in the Power API can be used.
  *
  * @param hw_behavior Relationship between voltage and speed on all voltage 
                       islands
  * @param speed_policy  Blanket policy for controlling voltage island speed 
                         levels
  * @param scheduling_policy Blanket policy for runtime task scheduling 
  *
  * @retval #PWR_OK  Initialized with no errors
  * @retval #PWR_INIT_ERR  Initialization failed
  * @retval #PWR_ALREADY_INITIALIZED  API is initialized
  */
retval_t pwr_initialize(const hw_behavior_t* hw_behavior, 
                        const speed_policy_t* speed_policy, 
                        const scheduling_policy_t* scheduling_policy);


/**
  * Frees resources used by the Power API
  * 
  * Must be called after all Power API functions have been called.
  *
  * @retval #PWR_OK  All resources freed successfully
  * @retval #PWR_UNINITIALIZED  Power API has not been initialized thus cannot 
  *                            be finalized
  */
retval_t pwr_finalize();

/**
  *	This method is a quick fix to a strange problem caused by re-initializing
  *	and finalizing the ecount environment multiple times. In particular,
  *	if the ec_finalize method is called in-between measurments then for some
  *	unknown reason measurments after ec_finalize always return zero.
  */
retval_t pwr_ecount_finalize();

/**
  * Retrieve the currently active hardware behavior
  *
  * @param hw_behavior  Pointer to active hardware behavior struct
  * 
  * @retval #PWR_OK  Active hardware behavior returned successfully
  * @retval #PWR_UNINITIALIZED  Power API has not been initialized thus hardware 
  *                            behavior is unavailable
  */
retval_t pwr_hw_behavior(hw_behavior_t* hw_behavior);

/**
  * Change currently active hardware behavior
  *
  * @param hw_behavior Pointer to new active hardware behavior struct
  * 
  * @retval #PWR_OK Active hardware behavior successfully changed
  * @retval #PWR_UNINITIALIZED  Power API has not been initialized thus hardware 
  *                             behavior cannot be changed
  */
retval_t pwr_change_hw_behavior(hw_behavior_t* hw_behavior);


/** @} */

//====-------------------------------------------------------------------------
// Low-Level Interface
//-----------------------------------------------------------------------------

/** \addtogroup lowlevel Low-Level Interface
  * @{  
  */
/**
  * The number of voltage islands controlled by the Power API
  * 
  * @param num_islands  Total voltage islands controlled by the Power API
  *
  * @retval #PWR_OK  Number of voltage islands has been returned
  * @retval #PWR_UNINITIALIZED  Power API has not been initialized
  */
retval_t pwr_num_islands(long* num_islands);


/**                                                                             
  * Retrieve unique ID for all voltage islands
  *
  * Requires that <code>islands</code> points to at least 
  * <code>num_islands*sizeof(island_id_t)</code> bytes of memory.
  *
  * @param islands An array of <code>island_id_t</code>
  *
  * @retval #PWR_OK  Voltage island IDs have been returned
  * @retval #PWR_UNINITIALIZED  Power API has not been initialized
  */                                                                             
retval_t pwr_islands(island_id_t* islands);


/**
  * Number of discrete speed levels, not including power and clock gated states,
  * supported by a voltage island 
  * 
  * The slowest speed level is `<code>1</code>' and speed levels increase 
  * monotonically until the fastest speed level at 
  * `<code>num_speed_levels</code>'. 
  *
  * @todo Map speed level to integer between 0 and 100 inclusive?
  * 
  * @param island  The ID of the island to get speed level count for
  * @param num_speed_levels  The number of speed levels supported by <code>island</code>
  *
  * @retval #PWR_OK  Number of speed levels has been returned
  * @retval #PWR_INVALID_ISLAND  The given ID does not correspond to a voltage island.
  * @retval #PWR_UNINITIALIZED  Power API has not been initialized
  */
retval_t pwr_num_speed_levels(const island_id_t island, long* num_speed_levels);  


/**
  * The current speed level of a voltage island
  * 
  * @param island  The island to check speed level on
  * @param current_level  The current speed level of <code>island</code>
  *
  * @retval #PWR_OK  Speed level has been returned
  * @retval #PWR_INVALID_ISLAND  The given ID does not correspond to a voltage island.
  * @retval #PWR_UNINITIALIZED  Power API has not been initialized
  */
retval_t pwr_current_speed_level(const island_id_t island, speed_level_t* current_level);


/**
  * Requests speed level on the given voltage island 
  * 
  * Requires <code>-1 <= new_level <= num_speed_levels</code>.
  * `<code>new_level == -1</code>' requests power gating.
  * `<code>new_level == O</code>' requests clock gating.
  * 
  * @param island  The island to request speed level on
  * @param new_level  The requested speed level
  * 
  * @retval #PWR_OK  Speed level has been changed
  * @retval #PWR_UNSUPPORTED_SPEED_LEVEL  The requested speed level is not supported 
  * @retval #PWR_ALREADY_MINMAX  Voltage island already at requested minimum or maximum speed level 
  * @retval #PWR_DVFS_ERR  DVFS failed
  * @retval #PWR_INVALID_ISLAND  The given ID does not correspond to a voltage island.
  * @retval #PWR_OVER_E_BUDGET Request denied, over energy budget (reserved for future use)
  * @retval #PWR_OVER_P_BUDGET Request denied, over power budget (reserved for future use)
  * @retval #PWR_OVER_T_BUDGET Request denied, over thermal budget (reserved for future use)
  * @retval #PWR_UNINITIALIZED  Power API has not been initialized
  */
retval_t pwr_request_speed_level(const island_id_t island, const speed_level_t new_level);


/**
  * Requests a speed level modification on the given island 
  *
  * <code>delta</code> can be positive or negative.
  *
  * @param island  The island to speed up or slow down
  * @param delta  The number of speed levels to increment by
  * @param bottom  The minimum legal speed level, can be one of #PWR_POWER_GATE,
  *                #PWR_CLOCK_GATE or #PWR_MIN_SPEED_LEVEL
  *
  * @todo Clarify behavior when an illegal speed level is requested
  * @todo Clarify semantics of speed levels when multiple hardware behaviors 
  *       are used  
  *
  * @return #PWR_OK  Speed level was successfully modified
  * @retval #PWR_UNSUPPORTED_SPEED_LEVEL  The requested speed level is not supported 
  * @retval #PWR_ALREADY_MINMAX  Voltage island already at requested minimum or maximum speed level 
  * @retval #PWR_DVFS_ERR  DVFS failed
  * @retval #PWR_INVALID_ISLAND  The given ID does not correspond to a voltage island.
  * @retval #PWR_OVER_E_BUDGET Request denied, over energy budget (reserved for future use)
  * @retval #PWR_OVER_P_BUDGET Request denied, over power budget (reserved for future use)
  * @retval #PWR_OVER_T_BUDGET Request denied, over thermal budget (reserved for future use)
  * @retval #PWR_UNINITIALIZED  Power API has not been initialized
  */
retval_t pwr_modify_speed_level(const island_id_t      island, 
                                const int              delta, 
                                const speed_level_t    bottom);

/**
  * Calculates the cost of switching speed levels
  *
  * @todo Decide on unit for agility. Currently ns because of cpufreq default.
  *       If agility in cycles, at what speed level? Full speed? 
  *       <code>to_level</code>?
  *
  * @param island  The island of interest
  * @param from_level  Starting speed level
  * @param to_level  Finishing speed level
  * @param best_case  Minimum cost in ns
  * @param worst_case Maximum cost in ns
  *
  * @retval #PWR_OK Agility successfully returned
  * @retval #PWR_ERR Agility not returned
  */
retval_t pwr_agility(const island_id_t      island, 
                     const speed_t          from_level, 
                     const speed_t          to_level,
                           agility_t*       best_case,
                           agility_t*       worst_case);

/**
  * Requests a voltage level modification of the given island 
  *
  * <code>delta</code> can be positive or negative.
  *
  * @todo Should we have a corresponding function to directly set voltage as
  *       we do with speed level?
  *
  * @param island  The island to change voltage on
  * @param delta  The number of voltage levels to modify by
  *
  * @retval #PWR_OK  Voltage level successfully modified
  * @retval #PWR_ARCH_UNSUPPORTED  Voltage level cannot be modified
  * @retval #PWR_UNSUPPORTED_SPEED_LEVEL  The requested speed level is not supported 
  * @retval #PWR_ALREADY_MINMAX  Voltage island already at requested minimum or maximum voltage 
  * @retval #PWR_DVFS_ERR  DVFS failed
  * @retval #PWR_INVALID_ISLAND  The given ID does not correspond to a voltage island.
  * @retval #PWR_OVER_E_BUDGET Request denied, over energy budget (reserved for future use)
  * @retval #PWR_OVER_P_BUDGET Request denied, over power budget (reserved for future use)
  * @retval #PWR_OVER_T_BUDGET Request denied, over thermal budget (reserved for future use)
  * @retval #PWR_UNINITIALIZED  Power API has not been initialized
  * @retval #PWR_ERR Voltage level not successfully modified
  */
retval_t pwr_modify_voltage(const island_id_t island, const int delta);

/**
  * Value of monotonically increasing energy counter on the given island.
  * Total energy consumed (J) since some undefined starting point is 
  * <code>e_j + e_uj*10E6</code>. Elapsed time (sec) since some undefined 
  * starting point is <code>t_sec + t_nsec*10E9</code>.
  *
  * @param island  The island to measure energy consumption on 
  * @param e_j  Energy consumed since some unspecified starting point on 
                <code>island</code>, joule component
  * @param e_uj  Energy consumed since some undefined starting point on
                 <code>island</code>, microjoule component
  * @param t_sec  Time since some unspecified starting point, seconds component
  * @param t_nsec  Time since some unspecified starting point, nanoseconds component
  *
  * @retval #PWR_OK  Energy counter successfuly returned
  * @retval #PWR_ERR  Energy counter not returned
  * @retval #PWR_OVERFLOW_ERR  Energy counter overflowed, inaccurate results provided
  */
retval_t pwr_energy_counter(const island_id_t      island, 
                                  energy_t*        e_j, 
                                  energy_t*        e_uj, 
                                  timestamp_t*     t_sec,
                                  timestamp_t*     t_nsec);
/** @} */
#ifdef __cplusplus
}
#endif
#endif
