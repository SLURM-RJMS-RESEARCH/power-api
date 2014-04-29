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
  * @file power_api.c Power API implementation for the Linux userspace CPU governor.
  *  
  * @authors M. Deneroff <deneroff@reservoir.com> <br>
  * @authors Benoit Meister <meister@reservoir.com<br>
  * @authors Tom Henretty <henretty@reservoir.com><br>
  * @authors Athanasios Konstantinidis <konstantinidis@reservoir.com><br>
  *
  * @copyright Copyright (C) 2013-2014 Reservoir Labs, Inc.
  */

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

/** Feature test macro for enabling clock functions */
#define _POSIX_C_SOURCE 199912L

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "power_api.h"
#include "ecount.h"

/** The maximum number of physical CPU in a system supported by the Power API */
#define PWR_MAX_CPU (PWR_MAX_ISLANDS)  
                         
/** The maximum number of CPU in a physical voltage island supported by the Power API */
#define PWR_MAX_CPU_PER_ISLAND (PWR_MAX_CPU) 

/** Integral unique identifier of a physical CPU */                             
typedef long cpu_id_t;

/** Holds information related to a physical voltage island */
typedef struct island {
    /** Uniquely identifies a voltage island in a given system */
    island_id_t id;
    
    /** The count of CPU */
    long num_cpu;

    /** IDs of the CPU */
    cpu_id_t* cpus;

    /** Total number of speed levels supported */
    long num_speed_levels;

    /** 
      * Nominal speed level set by Power API clients, may be overridden by 
      * hardware PMU
      */
    speed_level_t current_speed_level;

    /** Minimum available speed level */
    speed_level_t min_speed_level;

    /** Maximum available speed level */
    speed_level_t max_speed_level;

    /** 
      * Maps speed levels to physical frequencies, i.e. 
      * <code>freqs[speed_level] = frequency</code> 
      */
    freq_t* freqs;

    /** 
      * If supported by hardware, the total number of voltages this island can 
      * be set at
      */
    long num_voltages;
    
    /** Voltages supported by this island */
    voltage_t* voltages;

    /** 
      * Nominal voltage as set by Power API client, may be overridden by 
      * hardware PMU
      */
    voltage_t current_voltage;

    /**
      * Worst case time to transition from on frequency / voltage to another
      */
    agility_t agility;
} island_t;


//====-------------------------------------------------------------------------
// Forward declare private functions
//-----------------------------------------------------------------------------
retval_t init_cpu();
retval_t init_islands();
retval_t init_speed_levels();
GString* sysfs_filename(const cpu_id_t cpu_id, const char* filename); 
gboolean island_deep_eq(const island_t* i0, const island_t* i1);
retval_t sort_and_cast_freqs(gchar** freqs, freq_t* sorted, long num_freqs);
gint compare_freq(freq_t f0, freq_t f1);
gint compare_cpu_id(const cpu_id_t i0, const cpu_id_t i1); 
retval_t pwr_num_cpu(long* num_cpu);

//====-------------------------------------------------------------------------
// Power API state
//-----------------------------------------------------------------------------

/** Has the <code>pwr_initialize()</code> function been successfully called? */
int initialized = FALSE;

/** How many physical CPU are in the system? */
long num_cpu = 0;

/** How many physical voltage islands are in the system? */
long num_islands = 0;

/** Array for physical island info */
island_t** islands = NULL;

/** Energy counter */
energy_t energy_counter = 0;


//====-------------------------------------------------------------------------
// CPUFreq file interface
//-----------------------------------------------------------------------------
/** File pointers for sysfs frequency control files, one per CPU */
FILE** island_throttle_files = NULL;

/** File pointers for sysfs frequency state files, one per CPU */
FILE** island_speed_files = NULL;



//====-------------------------------------------------------------------------
// High-Level Interface
//-----------------------------------------------------------------------------
retval_t pwr_is_initialized(int* is_initialized) {
    *is_initialized = initialized;
    return PWR_OK;
}

retval_t pwr_initialize(const hw_behavior_t* hw_behavior,
                        const speed_policy_t* speed_policy,
                        const scheduling_policy_t* scheduling_policy) {
    if (initialized) {
        return PWR_ALREADY_INITIALIZED;
    }

    // Initialize physical CPU info
    if (PWR_OK != init_cpu()) {
        return PWR_INIT_ERR;            
    }

    // Initialize physical islands info
    if (PWR_OK != init_islands()) {
        return PWR_INIT_ERR;  
    }

    // Initialize physical speeds info
    if (PWR_OK != init_speed_levels()) {
        return PWR_INIT_ERR;
    }

    // Initialize throttle and speedometer files for each island
    island_throttle_files = (FILE**)malloc(num_islands*sizeof(FILE*));
    island_speed_files = (FILE**)malloc(num_islands*sizeof(FILE*));

    for (long island_id=0; island_id<num_islands; ++island_id) {
        // Use first CPU in island to throttle and read speed from
        long cpu_id = islands[island_id]->cpus[0];

        // Build path to sysfs entries
        GString* throttle_filename = sysfs_filename(cpu_id, "scaling_setspeed");
        GString* cur_speed_filename = sysfs_filename(cpu_id, "scaling_cur_freq");

        // Get FILE*
        island_throttle_files[island_id] = fopen(throttle_filename->str, "w");
        island_speed_files[island_id] = fopen(cur_speed_filename->str, "r");

        // Sanity check
        if (NULL == island_throttle_files[island_id]) {
            return PWR_INIT_ERR;
        }
        if (NULL == island_speed_files[island_id]) {
            return PWR_INIT_ERR;
        }
    }

	// Initialize energy counters
	ec_initialize();
	ec_start_e_counters();

    initialized = TRUE;
    // Set each island to low speed
    for (long i=0; i<num_islands; ++i) {
        pwr_request_speed_level(i, 1);    
    }

    return PWR_OK;
}

retval_t pwr_finalize() {
    if (FALSE == initialized) {
        return PWR_UNINITIALIZED;
    }

    for (int i=0; i < num_islands; ++i) {
        free(islands[i]->freqs);
        //free(islands[i]->voltages);
        free(islands[i]);
        fclose(island_throttle_files[i]);
        fclose(island_speed_files[i]);
    }

    free(islands);

    free(island_throttle_files);
    free(island_speed_files);

    initialized = FALSE;
    return PWR_OK;
}

retval_t pwr_ecount_finalize()
{
	ec_finalize();
	return PWR_OK;
}

retval_t pwr_hw_behavior(hw_behavior_t* hw_behavior) {
    return PWR_UNIMPLEMENTED;
}

retval_t pwr_change_hw_behavior(hw_behavior_t* hw_behavior) {
    return PWR_UNIMPLEMENTED;
}

//====-------------------------------------------------------------------------
// Low-Level Interface
//-----------------------------------------------------------------------------



retval_t pwr_num_islands(long* num_islands_p) {
    if (FALSE == initialized) {
        return PWR_UNINITIALIZED;
    }
    *num_islands_p = num_islands;
    return PWR_OK;
}

retval_t pwr_islands(island_id_t* island_ids) {
    if (FALSE == initialized) {
        return PWR_UNINITIALIZED;
    }
    for (int i=0; i<num_islands; ++i) {
        island_ids[i] = islands[i]->id;
    }   
    return PWR_OK;
}

retval_t pwr_num_speed_levels(const island_id_t island, long* num_speed_levels) {
    if (FALSE == initialized) {
        return PWR_UNINITIALIZED;
    }
    if (island < 0 || island >= num_islands) {
        return PWR_INVALID_ISLAND;
    }
    *num_speed_levels = islands[island]->num_speed_levels;
    return PWR_OK;
}

retval_t pwr_current_speed_level(const island_id_t island, speed_level_t* current_level) {
    if (FALSE == initialized) {
        return PWR_UNINITIALIZED;
    }
    if (island < 0 || island >= num_islands) {
        return PWR_INVALID_ISLAND;
    }
    (*current_level) = islands[island]->current_speed_level;
    return PWR_OK;
}

retval_t pwr_request_speed_level(const island_id_t island, const speed_level_t new_level) {
    if (FALSE == initialized) {
        return PWR_UNINITIALIZED;
    }
    if (island < 0 || island >= num_islands) {
        return PWR_INVALID_ISLAND;
    }
    if (new_level < islands[island]->min_speed_level ||
        new_level > islands[island]->max_speed_level) {
        return PWR_UNSUPPORTED_SPEED_LEVEL;
    }
    if ((new_level == islands[island]->min_speed_level  ||
         new_level == islands[island]->max_speed_level) && 
         new_level == islands[island]->current_speed_level) {
        return PWR_ALREADY_MINMAX;
    }

    // Write the speed to the throttle file, rewind
    fprintf(island_throttle_files[island], "%ld", islands[island]->freqs[new_level]);
    if (ferror(island_throttle_files[island])) {
        return PWR_DVFS_ERR;
    }

    rewind(island_throttle_files[island]);
    if (ferror(island_throttle_files[island])) {
        return PWR_DVFS_ERR;
    }
   
    // Set the new level
    islands[island]->current_speed_level = new_level;

    return PWR_OK;
}

retval_t pwr_modify_speed_level(const island_id_t island, 
                                const int              delta, 
                                const speed_level_t    bottom) {
    if (FALSE == initialized) {
        return PWR_UNINITIALIZED;
    }
    speed_level_t current_level;
    pwr_current_speed_level(island, &current_level);
    return pwr_request_speed_level(island, current_level + delta);
}

retval_t pwr_agility(const island_id_t island, 
                     const speed_t          from_level,
                     const speed_t          to_level,
                           agility_t*       best_case,                          
                           agility_t*       worst_case) {
    if (FALSE == initialized) {
        return PWR_UNINITIALIZED;
    }
    *best_case = islands[island]->agility;
    *worst_case = islands[island]->agility;

    return PWR_OK;
}

retval_t pwr_modify_voltage(const island_id_t island, const int delta) {
    if (FALSE == initialized) {
        return PWR_UNINITIALIZED;
    }

    return PWR_UNIMPLEMENTED;
}

retval_t pwr_energy_counter(const island_id_t island, 
                                  energy_t*        e_j, 
                                  energy_t*        e_mj, 
                                  timestamp_t*     t_sec,
                                  timestamp_t*     t_nsec) {

	energy_t billion = 1000000000;
	ec_stop_e_counters();
	e_data * data = ec_read_e_counters();

	*e_j = data->values[0]/billion;
	energy_t temp = data->values[0]%billion;
	*e_mj = temp/1000;
	if ( *e_j == 0 )
		return PWR_ERR;
	// printf("Output value(%lld) Joules(%ld) micro-joules(%ld) \n", data->values[0], *e_j, *e_mj);

    // Get clock values
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    *t_sec = ts.tv_sec;
    *t_nsec = ts.tv_nsec;

    return PWR_OK;
}


//====-------------------------------------------------------------------------
// Private functions (for internal use only)
//-----------------------------------------------------------------------------

/**                                                                             
  * Sets the number of physical CPU in the system                            
  *
  * @retval PWR_OK  CPU were initialized
  * @retval PWR_ERR  CPU could not be initialized
  */   
retval_t init_cpu() {
    num_cpu = 0;

    // This is ~= a C version of `ls -1 /sys/devices/system/cpu/cpu[0-9]* | wc -l`
    GError* dir_error = NULL;
    GDir* cpu_dir = g_dir_open("/sys/devices/system/cpu", 0, &dir_error);
    if (NULL != dir_error) {
        fputs("Error opening dir...\n", stderr);
        fputs(dir_error->message, stderr);
        g_error_free(dir_error);
        return PWR_ERR;
    }

    GError* regex_error = NULL;
    GRegex* cpu_regex = g_regex_new("cpu[0-9]+", 0, 0, &regex_error);
    if (NULL != regex_error) {
        fputs("Error building regex...\n", stderr);
        fputs(regex_error->message, stderr);
        g_error_free(regex_error);
        return PWR_ERR;
    }

    const gchar* filename;
    while (NULL != (filename = g_dir_read_name(cpu_dir))) {
        if (g_regex_match(cpu_regex, filename, 0, NULL)) {
            num_cpu++;
        }
    }

    g_dir_close(cpu_dir);
    g_regex_unref(cpu_regex);
    return PWR_OK;
}

/**
  * Sets the number of physical voltage islands in the system and creates a 
  * struct for each

  * @retval PWR_OK  Voltage islands were initialized
  * @retval PWR_ERR  Voltage islands could not be initialized
  */
retval_t init_islands() {
    num_islands = 0;
    GPtrArray* islands_gpa = g_ptr_array_new();

    // Check /sys/devices/system/cpu/cpu*/cpufreq/affected_cpus to determine 
    // voltage island membership
    //
    // A comma separated list of CPU on the same island should be returned
    // and parsed to determine island membership
    for (long cpu=0; cpu<num_cpu; ++cpu) {


        // Get affected CPU from sysfs
        GString* affected_filename = sysfs_filename(cpu, "affected_cpus");
        GError* file_error = NULL;
        gchar* affected_char = NULL;
        if (!g_file_get_contents(affected_filename->str, &affected_char, NULL, &file_error)) {
            fprintf(stderr, "Error opening affected cpu file for cpu %ld...\n",cpu);
            fputs(file_error->message, stderr);
            g_error_free(file_error);
            g_string_free(affected_filename, TRUE);
            g_ptr_array_free(islands_gpa, TRUE);
            return PWR_ERR;
        }
        gchar** affected_cpu_vstr = g_strsplit(affected_char, 
                                               " ", 
                                               PWR_MAX_CPU_PER_ISLAND);
        gchar** ptr;

        // Count CPU on same island
        long num_affected_cpu = 0;
        for (ptr=affected_cpu_vstr; *ptr; ptr++) {
            num_affected_cpu++;
        }

        // Build the island struct
        island_t* new_pi = (island_t*)malloc(sizeof(island_t));
        new_pi->num_cpu = num_affected_cpu;
        new_pi->cpus = (cpu_id_t*)malloc(num_affected_cpu*sizeof(cpu_id_t));

        // Sort CPU in island
        GArray* tmp_sorted = g_array_new(FALSE, FALSE, sizeof(cpu_id_t));
        for (int i=0; i<num_affected_cpu; ++i) {
            cpu_id_t cpu_id = atoi(affected_cpu_vstr[i]);
            g_array_append_val(tmp_sorted, cpu_id);
        }

        g_array_sort(tmp_sorted, (GCompareFunc)&compare_cpu_id);

        for (int i=0; i<num_affected_cpu; ++i) {
            new_pi->cpus[i] = g_array_index(tmp_sorted, cpu_id_t, i);
        }

        // Compare to existing islands, save if it is new
        gboolean pi_exists = FALSE;
        for (long i=0; i<num_islands; ++i) {
            island_t* existing_pi = g_ptr_array_index(islands_gpa, i);
            if (island_deep_eq(new_pi, existing_pi)) {
                pi_exists = TRUE;
                free(new_pi->cpus);
                free(new_pi);
                break;
            }
        }
        if (!pi_exists) {
            new_pi->id = num_islands;
            num_islands++;
            g_ptr_array_add(islands_gpa, new_pi);
        } 

        g_array_free(tmp_sorted, TRUE);
        g_strfreev(affected_cpu_vstr);
        g_free(affected_char);
        g_string_free(affected_filename, TRUE);
    }

    islands = (island_t**)malloc(num_islands*sizeof(island_t*));
    for (int i=0; i<num_islands; ++i) {
        islands[i] = g_ptr_array_remove_index(islands_gpa,0);
        
        // Set agility
        cpu_id_t cpu_id = islands[i]->cpus[0];
        
        GString* agility_filename = sysfs_filename(cpu_id, "cpuinfo_transition_latency");
        GError* file_error = NULL;
        gchar* agility_char = NULL;

        if (!g_file_get_contents(agility_filename->str, &agility_char, NULL, &file_error)) {
            fprintf(stderr, "Error opening agility file for cpu %ld...\n", cpu_id);
            fputs(file_error->message, stderr);
            g_error_free(file_error);
            g_string_free(agility_filename, TRUE);
            g_ptr_array_free(islands_gpa, TRUE);
            return PWR_ERR;
        }
        agility_t* island_agility = &(islands[i]->agility);
        sscanf(agility_char, "%ld", island_agility);
        g_free(agility_char);
        g_string_free(agility_filename, TRUE);
    }

    g_ptr_array_free(islands_gpa, FALSE);
    return PWR_OK;
}

/**
  * Sets the speed levels and frequencies for each voltage island
  *
  * @retval PWR_OK  Speed levels and frequencies initialized
  * @retval PWR_ERR  Speed levels and frequencies could not be initialized
  */
retval_t init_speed_levels() {
    // Each island may have a different number of speed levels
    for (long island_id=0; island_id<num_islands; ++island_id) {
        //===------------------------------------------------------------------
        // Get the speeds from the first CPU in the island

        // Build path to sysfs entry for available frequencies
        long cpu_id = islands[island_id]->cpus[0];
        GString* freq_filename = sysfs_filename(cpu_id, "scaling_available_frequencies");
        
        // Get the frequencies
        GError* file_error = NULL;                                              
        gchar* freq_char = NULL;                                             
        if (!g_file_get_contents(freq_filename->str, &freq_char, NULL, &file_error)) {
            fprintf(stderr, "Error opening speeds file for cpu %ld...\n", cpu_id);
            fputs(file_error->message, stderr);                                 
            g_error_free(file_error);
            g_string_free(freq_filename, TRUE);
            return PWR_ERR;                                                     
        }
        gchar** freq_vstr = g_strsplit(freq_char, " ", PWR_MAX_SPEED_LEVELS);

        // Calculate the number of speed levels
        gchar** ptr;
        long num_speed_levels = -1;

        for (ptr=freq_vstr; *ptr; ptr++) {
            num_speed_levels++;
        }

        // Set the number of speed levels / frequencies in island struct
        islands[island_id]->num_speed_levels = num_speed_levels;
        islands[island_id]->freqs = 
            (freq_t*)malloc(num_speed_levels*sizeof(freq_t*));
        
        // Sort frequencies low to high, set speed levels and min/max
        sort_and_cast_freqs(freq_vstr, 
                            islands[island_id]->freqs, 
                            num_speed_levels);
        islands[island_id]->min_speed_level = 0;
        islands[island_id]->max_speed_level = num_speed_levels - 1;

        g_strfreev(freq_vstr);
        g_free(freq_char);
        g_string_free(freq_filename, TRUE);
    }
    return PWR_OK;
}

/**
  * Builds a filename of the form 
  * <code>/sys/devices/system/cpu/cpu_id/cpufreq/filename</code>
  *
  * @param cpu_id The unique identifier of the cpu to build a filename for
  * @param filename The cpufreq file to use in the filename
  *
  * @return A sysfs filename as a <code>GString</code> 
  */
GString* sysfs_filename(const cpu_id_t cpu_id, const char* filename) {
    GString* new_filename = g_string_new("/sys/devices/system/cpu/cpu");
    char cpu_str[21];// Maximum chars in base 10 long value is 20 + 1 (\0)
    sprintf(cpu_str, "%ld", cpu_id);

    g_string_append(new_filename, cpu_str);
    g_string_append(new_filename, "/cpufreq/");
    g_string_append(new_filename, filename);

    return new_filename;
}

/**
  * Deep equality test of two voltage islands
  *
  * Tests if two voltage islands are equal by comparing their component CPU
  * IDs. Assumes CPU IDs are sorted.
  * 
  * @param i0  Voltage island to compare
  * @param i1  Voltage island to compare
  * 
  * @retval TRUE  If and only if all corresponding CPU in each island have the 
  *               same id
  * @retval FALSE  Otherwise
  */
gboolean island_deep_eq(const island_t* i0, const island_t* i1) {
    if (i0->num_cpu != i1->num_cpu) {
        return FALSE;
    }
    for (int i=0; i<i0->num_cpu; ++i) {
        if (i0->cpus[i] != i1->cpus[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

/**
  * Sorted list of frequencies constructor
  *
  * Builds a sorted array of <code>freq_t</code> frequencies from an array of 
  * <code>char*</code>.
  *
  * @param freqs  An array of strings representing frequencies. These strings
  *               should be null-terminated and numbers only 
  *               (<code>^[0-9]+\0$</code>).
  * @param sorted  A sorted version of <code>freqs</code>
  * @param num_freqs  The number of elements in <code>freqs</code> and 
  *                   <code>sorted</code>
  *
  * @retval PWR_OK
  */
retval_t sort_and_cast_freqs(gchar** freqs, freq_t* sorted, long num_freqs) {
    GArray* tmp_sorted = g_array_new(FALSE, FALSE, sizeof(freq_t));
    for (long i=0; i<num_freqs; ++i) {
        freq_t freq = atoi(freqs[i]);
        g_array_append_val(tmp_sorted, freq);
    }
    g_array_sort(tmp_sorted, (GCompareFunc)(&compare_freq));
    for (long i=0; i<num_freqs; ++i) {
        sorted[i] = g_array_index(tmp_sorted, freq_t, i);
    }
    g_array_free(tmp_sorted, FALSE);
    return PWR_OK;
}

/**
  * Compares two frequencies
  *
  * @param f0  Frequency to compare
  * @param f1  Frequency to compare
  * 
  * @retval -1  <code>(f0 > f1)</code>
  * @retval 0  <code>(f0 == f1)</code>
  * @retval 1  <code>(f0 < f1)</code>
  */
gint compare_freq(const freq_t f0, const freq_t f1) {
    if (f0 > f1) {
        return -1;
    } else if (f0 < f1) {
        return 1;
    }
    return 0;
}

/**
  * Compares two CPU ID
  *
  * @param i0  CPU ID to compare
  * @param i1  CPU ID to compare
  * 
  * @retval -1  <code>(i0 < i1)</code>
  * @retval 0  <code>(i0 == i1)</code>
  * @retval 1  <code>(i0 > i1)</code>
  */
gint compare_cpu_id(const cpu_id_t i0, const cpu_id_t i1) {
    if (i0 < i1) {
        return -1;
    } else if (i0 > i1) {
        return 1;
    }
    return 0;
}

/**                                                                             
  * The number of actual CPU controlled by the Power API                        
  *                                                                             
  * @param num_cpu  Total CPU controlled by the Power API                  
  *                                                                             
  * @retval #PWR_OK  Number of CPU has been returned                            
  * @retval #PWR_UNINITIALIZED  Power API has not been initialized              
  */
retval_t pwr_num_cpu(long* num_of_cpus) {
    if (FALSE == initialized) {
        return PWR_UNINITIALIZED;
    }
    *num_of_cpus = num_cpu;
    return PWR_OK;
}
