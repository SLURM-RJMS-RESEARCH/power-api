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
 *  The file contains all the fields and functions used internally. 
 *  Never directly use them unless you really know what you are doing.
 */

#include <glib.h> 
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "power-api.h"

//====-------------------------------------------------------------------------
// Typedefs
//-----------------------------------------------------------------------------

/** Error codes returned by API functions */
typedef int pwr_err_t;

//====-------------------------------------------------------------------------
// Internal types for the modules
//-----------------------------------------------------------------------------

/** You are not supposed to directly access any of those fields */
/* Holds information related to a physical voltage island */
typedef struct phys_island {  
    /* The count of CPU */
    unsigned long num_cpu;

    /* IDs of the CPU */
    unsigned long *cpus;

    /* Total number of speed levels supported */
    unsigned int num_speed_levels;

    /* 
      * Nominal speed level set by Power API clients, may be overridden by 
      * hardware PMU
      */
    speed_level_t current_speed_level;

    /* Minimum available speed level */
    speed_level_t min_speed_level;

    /* Maximum available speed level */
    speed_level_t max_speed_level;

    /* 
      * Maps speed levels to physical frequencies, i.e. 
      * <code>freqs[speed_level] = frequency</code> 
      */
    freq_t* freqs;

    /* 
      * If supported by hardware, the total number of voltages this island can 
      * be set at
      */
    long num_voltages;
    
    /* Voltages supported by this island */
    voltage_t* voltages;

    /* 
      * Nominal voltage as set by Power API client, may be overridden by 
      * hardware PMU
      */
    voltage_t current_voltage;

    /*
      * Worst case time to transition from on frequency / voltage to another
      */
    agility_t agility;
} phys_island_t;


//====-------------------------------------------------------------------------
// Private structures shared across all modules
//-----------------------------------------------------------------------------

/** You are not supposed to directly access any of those fields */
/* Generic structure to store the current library status */
typedef struct pwr_ctx {
    /* bitfield to determine if a module was initialized */
    unsigned int module_init;

    /* The last error that occurred */
    pwr_err_t error;

    /* Where to write the error messages. Can be NULL to print no message. */
    FILE *err_fd;

    /* --- structure module --- */

    /* How many physical CPU are in the system? */
    unsigned long num_phys_cpu;

    /* How many physical voltage islands are in the system? */
    unsigned long num_phys_islands;

    /* Physical power islands on the system */
    phys_island_t **phys_islands;

    /* --- DVFS module --- */
    
    /* File pointers fpr sysfs frequency control files, one per CPU */
    FILE** island_throttle_files;

    /* --- Power measurements --- */

    /* Are we measuring energy right now? */
    bool emeas_running;

    /* Last measurement */
    pwr_emeas_t *emeas;

    /* Event set identifier (used by PAPI) */
    int event_set;
} pwr_ctx_t;


//====-------------------------------------------------------------------------
// Private functions shared across all modules
//-----------------------------------------------------------------------------


// ###### General functions ######


/*
  * Builds a filename of the form 
  * <code>/sys/devices/system/cpu/cpu_id/cpufreq/filename</code>
  *
  * @param cpu_id The unique identifier of the cpu to build a filename for
  * @param filename The cpufreq file to use in the filename
  *
  * @return A sysfs filename as a <code>GString</code> 
  */
GString* sysfs_filename(unsigned long cpu_id, const char* filename); 


// ###### Structure functions ######


/*
  * Sets the number of physical voltage islands in the system and creates a 
  * struct for each.
  *
  * @param ctx The current library context.
  */
void init_struct_module(pwr_ctx_t *ctx);

/*
 * Frees the memory used to store structural information.
 */
void free_structure_data(pwr_ctx_t *ctx);


// ###### Frequencies-related functions ######


/*
  * Sets the speed levels and frequencies for each voltage island
  *
  * @param ctx The current library context.
  */
void init_speed_levels(pwr_ctx_t *ctx);

/*
 * Frees the memory used to store speed information.
 *
 * @return PWR_OK.
 */
void free_speed_data(pwr_ctx_t *ctx);

// ###### Energy-related functions ######


/*
  * Allocate resources used to collect energy counter data
  *
  * @param ctx The current library context.
  */
void init_energy(pwr_ctx_t *ctx);

/*
  * Release resources used to gather energy counter data
  */
void free_energy_data(pwr_ctx_t *ctx);

