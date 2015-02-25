/**
  * Copyright 2014-15 Reservoir Labs, Inc.
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "internals.h"

//====-------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

static gboolean phys_island_deep_eq(const phys_island_t* i0, const phys_island_t* i1);
static gint compare_phys_cpu_id(unsigned long i0, unsigned long i1); 

//====-------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

unsigned long pwr_num_phys_cpus(pwr_ctx_t *ctx) {
    if (ctx == NULL) {
        ctx->error = PWR_INIT_ERR;
        return 0;
    }

    if (!pwr_is_initialized(ctx, PWR_MODULE_STRUCT)) {
        ctx->error = PWR_UNINITIALIZED;
        return 0;
    }

    ctx->error = PWR_OK;
    return ctx->num_phys_cpu;
}

unsigned long pwr_num_phys_islands(pwr_ctx_t *ctx) {
    if (ctx == NULL) {
        ctx->error = PWR_INIT_ERR;
        return 0;
    }

    if (!pwr_is_initialized(ctx, PWR_MODULE_STRUCT)) {
        ctx->error = PWR_UNINITIALIZED;
        return 0;
    }
    
    ctx->error = PWR_OK;
    return ctx->num_phys_islands;
}

unsigned long pwr_island_of_cpu(pwr_ctx_t *ctx, unsigned long cpu) {
    if (ctx == NULL) {
        ctx->error = PWR_INIT_ERR;
        return 0;
    }

    if (cpu >= ctx->num_phys_cpu) {
        ctx->error = PWR_REQUEST_DENIED;
        return ctx->num_phys_islands;
    }

    for (unsigned long i = 0; i < ctx->num_phys_islands; ++i) {
        phys_island_t *island = ctx->phys_islands[i];
        for (unsigned long j = 0; j < island->num_cpu; ++j) {
            if (island->cpus[j] == cpu) {
                ctx->error = PWR_OK;
                return i;
            }
        }
    }

    ctx->error = PWR_ERR;
    if (ctx->err_fd != NULL) {
        fprintf(ctx->err_fd, "Cannot find the island for CPU %lu\n", cpu);
    }
    return ctx->num_phys_islands;
}

//====-------------------------------------------------------------------------
// Library internal functions
//-----------------------------------------------------------------------------

void init_struct_module(pwr_ctx_t *ctx) {
    assert(ctx != NULL);
    assert(!pwr_is_initialized(ctx, PWR_MODULE_STRUCT));

    ctx->num_phys_cpu = sysconf(_SC_NPROCESSORS_ONLN);
    ctx->num_phys_islands = 0;

    //===----------------------------------------------------------------------
    // Build the island for every CPU 

    GPtrArray* phys_islands_gpa = g_ptr_array_new();

    // Check /sys/devices/system/cpu/cpu*/cpufreq/freqdomain_cpus to determine 
    // voltage island membership if it exists, affected_cpus otherwise.
    //
    // A space separated list of CPU on the same island should be returned
    // and parsed to determine island membership
    for (unsigned long cpu = 0; cpu < ctx->num_phys_cpu; ++cpu) {

        // Get affected CPU from sysfs
        GError* file_error = NULL;
        gchar* affected_char = NULL;
        GString *affected_filename = sysfs_filename(cpu, "freqdomain_cpus");

        if (!g_file_get_contents(affected_filename->str, &affected_char,
                                 NULL, &file_error))
        {
            g_string_free(affected_filename, TRUE);
            g_error_free(file_error);
            file_error = NULL;

            // try the affected_cpus file
            affected_filename = sysfs_filename(cpu, "affected_cpus");
            if (!g_file_get_contents(affected_filename->str, &affected_char,
                                     NULL, &file_error))
            {
                if (ctx->err_fd != NULL) {
                    fprintf(ctx->err_fd,
                        "Error opening affected cpu file for cpu %lu...\n",
                        cpu);
                    fputs(file_error->message, ctx->err_fd);
                    fprintf(ctx->err_fd, "\n");
                }
                g_error_free(file_error);
                ctx->error = PWR_ARCH_UNSUPPORTED;

                g_string_free(affected_filename, TRUE);
                g_ptr_array_free(phys_islands_gpa, TRUE);
                return;
            }
        }
        g_string_free(affected_filename, TRUE);
        
        gchar** affected_cpu_vstr = g_strsplit(affected_char, " ",
            PWR_MAX_CPU_PER_PHYS_ISLAND);
        gchar** ptr;

        // Count CPU on same island
        unsigned int num_affected_cpu = 0;
        for (ptr = affected_cpu_vstr; *ptr; ptr++) {
            num_affected_cpu++;
        }

        // Build the island struct
        phys_island_t* new_pi = malloc(sizeof(*new_pi));
        new_pi->num_cpu = num_affected_cpu;
        new_pi->cpus = malloc(num_affected_cpu * sizeof(*new_pi->cpus));

        // Sort CPU in island
        GArray *tmp_sorted = g_array_new(FALSE, FALSE, sizeof(*affected_cpu_vstr));
        for (unsigned int i = 0; i < num_affected_cpu; ++i) {
            unsigned long cpu_id = atoi(affected_cpu_vstr[i]);
            g_array_append_val(tmp_sorted, cpu_id);
        }

        g_array_sort(tmp_sorted, (GCompareFunc) &compare_phys_cpu_id);

        for (unsigned int i = 0; i < num_affected_cpu; ++i) {
            new_pi->cpus[i] = g_array_index(tmp_sorted, unsigned long, i);
        }

        // Compare to existing islands, save if it is new
        gboolean pi_exists = FALSE;
        for (unsigned long i = 0; i < ctx->num_phys_islands; ++i) {
            phys_island_t* existing_pi = g_ptr_array_index(phys_islands_gpa, i);
            if (phys_island_deep_eq(new_pi, existing_pi)) {
                pi_exists = TRUE;
                free(new_pi->cpus);
                free(new_pi);
                break;
            }
        }
        if (!pi_exists) {
            ctx->num_phys_islands++;
            g_ptr_array_add(phys_islands_gpa, new_pi);
        } 

        g_array_free(tmp_sorted, TRUE);
        g_strfreev(affected_cpu_vstr);
        g_free(affected_char);
    }

    //===----------------------------------------------------------------------
    // Convert the island to their internal form and setup the agility 

    ctx->phys_islands = malloc(ctx->num_phys_islands * sizeof(*ctx->phys_islands));
    for (unsigned long i = 0; i < ctx->num_phys_islands; ++i) {
        ctx->phys_islands[i] = g_ptr_array_remove_index(phys_islands_gpa, 0);
        
        // Set agility
        unsigned long cpu_id = ctx->phys_islands[i]->cpus[0];
        
        GString* agility_filename = sysfs_filename(cpu_id, "cpuinfo_transition_latency");
        GError* file_error = NULL;
        gchar* agility_char = NULL;

        if (!g_file_get_contents(agility_filename->str, &agility_char, NULL,
            &file_error)) 
        {
            if (ctx->err_fd != NULL) {
                fprintf(ctx->err_fd,
                    "Error opening agility file for cpu %ld...\n",
                    cpu_id);
                fputs(file_error->message, ctx->err_fd);
            }
            g_error_free(file_error);
            ctx->error = PWR_ARCH_UNSUPPORTED;

            // free the island memory
            for (unsigned long j = 0; j < i; ++j) {
                free(ctx->phys_islands[j]->cpus);
                free(ctx->phys_islands[j]);
            }
            free(ctx->phys_islands);
            for (; i < ctx->num_phys_islands; ++i) {
                ctx->phys_islands[0] =
                    g_ptr_array_remove_index(phys_islands_gpa, 0);
                free(ctx->phys_islands[0]->cpus);
                free(ctx->phys_islands[0]);
            }

            g_string_free(agility_filename, TRUE);
            g_ptr_array_free(phys_islands_gpa, TRUE);
            return;
        }
        agility_t* island_agility = &(ctx->phys_islands[i]->agility);
        sscanf(agility_char, "%ld", island_agility);
        g_free(agility_char);
        g_string_free(agility_filename, TRUE);
    }

    g_ptr_array_free(phys_islands_gpa, TRUE);
    
    // set the initialized flag
    ctx->error = PWR_OK;
    ctx->module_init |= (1U << PWR_MODULE_STRUCT);
}

void free_structure_data(pwr_ctx_t *ctx) {
    if (ctx == NULL) {
        return;
    }

    if (!pwr_is_initialized(ctx, PWR_MODULE_STRUCT)) {
        ctx->error = PWR_UNINITIALIZED;
        return;
    }

    for (unsigned int i = 0; i < ctx->num_phys_islands; ++i) {
        free(ctx->phys_islands[i]->cpus);
        free(ctx->phys_islands[i]);
    }
    free(ctx->phys_islands);
    ctx->error = PWR_OK;
}


//====-------------------------------------------------------------------------
// Local functions
//-----------------------------------------------------------------------------

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
gboolean phys_island_deep_eq(const phys_island_t* i0, const phys_island_t* i1) {
    if (i0->num_cpu != i1->num_cpu) {
        return FALSE;
    }
    for (unsigned long i = 0; i < i0->num_cpu; ++i) {
        if (i0->cpus[i] != i1->cpus[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

/**
  * Compares two CPU ID
  *
  * @param i0  CPU ID to compare
  * @param i1  CPU ID to compare
  * 
  * @return A negative value if i0 is before i1, 0 if they are equal, or a
  *  positive value otherwise.
  */
gint compare_phys_cpu_id(unsigned long i0, unsigned long i1) {
    return i0 - i1;
}
