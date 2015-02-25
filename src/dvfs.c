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
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internals.h"

//====-------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

static void sort_and_cast_freqs(gchar** freqs, freq_t* sorted, long num_freqs);
static gint compare_freq(const freq_t f0, const freq_t f1);

//====-------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

unsigned int pwr_num_speed_levels(pwr_ctx_t *ctx, unsigned long island) {
    if (ctx == NULL) {
        ctx->error = PWR_INIT_ERR;
        return 0;
    }
    
    if (!pwr_is_initialized(ctx, PWR_MODULE_DVFS)) {
        ctx->error = PWR_UNINITIALIZED;
        return 0;
    }

    if (island >= ctx->num_phys_islands) {
        ctx->error = PWR_INVALID_ISLAND;
        return 0;
    }

    ctx->error = PWR_OK;
    return ctx->phys_islands[island]->num_speed_levels;
}

unsigned int pwr_current_speed_level(pwr_ctx_t *ctx, unsigned long island) {
    if (ctx == NULL) {
        ctx->error = PWR_INIT_ERR;
        return 0;
    }
    
    if (!pwr_is_initialized(ctx, PWR_MODULE_DVFS)) {
        ctx->error = PWR_UNINITIALIZED;
        return 0;
    }

    if (island >= ctx->num_phys_islands) {
        ctx->error = PWR_INVALID_ISLAND;
        return 0;
    }

    ctx->error = PWR_OK;
    return ctx->phys_islands[island]->current_speed_level;
}

void pwr_request_speed_level(pwr_ctx_t *ctx, unsigned long island,
    unsigned int new_level)
{
    if (ctx == NULL) {
        ctx->error = PWR_INIT_ERR;
        return;
    }
    
    if (!pwr_is_initialized(ctx, PWR_MODULE_DVFS)) {
        ctx->error = PWR_UNINITIALIZED;
        return;
    }

    if (island >= ctx->num_phys_islands) {
        ctx->error = PWR_INVALID_ISLAND;
        return;
    }

    if (new_level < ctx->phys_islands[island]->min_speed_level ||
        new_level > ctx->phys_islands[island]->max_speed_level)
    {
        ctx->error = PWR_UNSUPPORTED_SPEED_LEVEL;
        return;
    }

    if ((new_level == ctx->phys_islands[island]->min_speed_level  ||
         new_level == ctx->phys_islands[island]->max_speed_level) && 
         new_level == ctx->phys_islands[island]->current_speed_level)
    {
        ctx->error = PWR_ALREADY_MINMAX;
        return;
    }

    // Write the speed to the throttle file, rewind
    fprintf(ctx->island_throttle_files[island], "%ld",
        ctx->phys_islands[island]->freqs[new_level]);
    if (ferror(ctx->island_throttle_files[island])) {
        ctx->error = PWR_DVFS_ERR;
        return;
    }

    if (fflush(ctx->island_throttle_files[island])) {
        ctx->error = PWR_DVFS_ERR;
        return;
    }

    // This is not really required on Linux
    //rewind(island_throttle_files[island]);
    //if (ferror(ctx->island_throttle_files[island])) {
    //    ctx->error = PWR_DVFS_ERR;
    //    return;
    //}
   
    // Set the new level
    ctx->phys_islands[island]->current_speed_level = new_level;

    ctx->error = PWR_OK;
    return;
}

void pwr_increase_speed_level(pwr_ctx_t *ctx, unsigned long island, int delta)
{
    if (ctx == NULL) {
        ctx->error = PWR_INIT_ERR;
        return;
    }
    
    if (!pwr_is_initialized(ctx, PWR_MODULE_DVFS)) {
        ctx->error = PWR_UNINITIALIZED;
        return;
    }

    unsigned int current_level = pwr_current_speed_level(ctx, island);
    pwr_request_speed_level(ctx, island, current_level + delta);
}

long pwr_agility(pwr_ctx_t *ctx, unsigned long island, unsigned int from_level,
    unsigned int to_level)
{
    (void) from_level;
    (void) to_level;

    if (ctx == NULL) {
        ctx->error = PWR_INIT_ERR;
        return 0;
    }
    
    if (!pwr_is_initialized(ctx, PWR_MODULE_DVFS)) {
        ctx->error = PWR_UNINITIALIZED;
        return 0;
    }

    if (from_level < ctx->phys_islands[island]->min_speed_level ||
        from_level > ctx->phys_islands[island]->max_speed_level)
    {
        ctx->error = PWR_UNSUPPORTED_SPEED_LEVEL;
        return 0;
    }

    if (to_level < ctx->phys_islands[island]->min_speed_level ||
        to_level > ctx->phys_islands[island]->max_speed_level)
    {
        ctx->error = PWR_UNSUPPORTED_SPEED_LEVEL;
        return 0;
    }

    ctx->error = PWR_OK;
    return ctx->phys_islands[island]->agility;
}

void pwr_increase_voltage(pwr_ctx_t *ctx, unsigned long island, int delta) {
    (void) island;
    (void) delta;

    if (ctx != NULL) {
        ctx->error = PWR_UNIMPLEMENTED;
    }
}

//====-------------------------------------------------------------------------
// Library internal functions
//-----------------------------------------------------------------------------

void init_speed_levels(pwr_ctx_t *ctx) {
    assert(ctx != NULL);
    assert(pwr_is_initialized(ctx, PWR_MODULE_STRUCT));
    assert(!pwr_is_initialized(ctx, PWR_MODULE_DVFS));

    //===----------------------------------------------------------------------
    // Make sure we are using the userspace governor
    for (unsigned long island_id = 0;
         island_id < ctx->num_phys_islands;
         ++island_id)
    {
        for (unsigned long c = 0;
            c < ctx->phys_islands[island_id]->num_cpu;
            ++c)
        {
            GString *gov_filename = sysfs_filename(c, "scaling_governor");
            GError* file_error = NULL;
            gchar* gov_char = NULL;

            if (!g_file_get_contents(gov_filename->str, &gov_char, NULL,
                &file_error)) 
            {
                if (ctx->err_fd) {
                    fprintf(ctx->err_fd,
                        "Error opening governor file for cpu %ld...\n", c);
                    fputs(file_error->message, ctx->err_fd);
                }
                g_error_free(file_error);
                ctx->error = PWR_ARCH_UNSUPPORTED;

                g_string_free(gov_filename, TRUE);
                return;
            }
        
            bool error = strncmp("userspace", gov_char, 9) != 0;
            
            g_free(gov_char);
            g_string_free(gov_filename, TRUE);

            if (error) {
                if (ctx->err_fd) {
                    fprintf(ctx->err_fd, "Invalid governor set on core %ld\n",
                        c);
                }
                ctx->error = PWR_UNAVAILABLE;
                return;
            }
        }
    }

    //===----------------------------------------------------------------------
    // Get the speeds from the first CPU in the island

    // Each island may have a different number of speed levels
    for (unsigned long island_id = 0;
         island_id < ctx->num_phys_islands;
         ++island_id)
    {
        // Build path to sysfs entry for available frequencies
        unsigned long cpu_id = ctx->phys_islands[island_id]->cpus[0];
        GString* freq_filename = sysfs_filename(cpu_id, "scaling_available_frequencies");
        
        // Get the frequencies
        GError* file_error = NULL;                                              
        gchar* freq_char = NULL;                                             
        if (!g_file_get_contents(freq_filename->str, &freq_char, NULL, &file_error)) {
            if (ctx->err_fd) {
                fprintf(ctx->err_fd,
                    "Error opening speeds file for cpu %ld...\n", cpu_id);
                fputs(file_error->message, ctx->err_fd);
            }
            g_error_free(file_error);
            ctx->error = PWR_ARCH_UNSUPPORTED;
            g_string_free(freq_filename, TRUE);
            return;                                                     
        }
        gchar** freq_vstr = g_strsplit(freq_char, " ", PWR_MAX_SPEED_LEVELS);

        // Calculate the number of speed levels
        gchar** ptr;
        long num_speed_levels = -1;

        for (ptr = freq_vstr; *ptr; ptr++) {
            num_speed_levels++;
        }

        // Set the number of speed levels / frequencies in island struct
        ctx->phys_islands[island_id]->num_speed_levels = num_speed_levels;
        ctx->phys_islands[island_id]->freqs = 
            malloc(num_speed_levels * sizeof(*ctx->phys_islands[island_id]->freqs));
        
        // Sort frequencies low to high, set speed levels and min/max
        sort_and_cast_freqs(freq_vstr, 
                            ctx->phys_islands[island_id]->freqs, 
                            num_speed_levels);
        ctx->phys_islands[island_id]->min_speed_level = 0;
        ctx->phys_islands[island_id]->max_speed_level = num_speed_levels - 1;

        g_strfreev(freq_vstr);
        g_free(freq_char);
        g_string_free(freq_filename, TRUE);

        // fetch the former frequency used
        speed_t cur_freq = 0;
        for (unsigned long cpu_id = 0;
             cpu_id < ctx->phys_islands[island_id]->num_cpu;
             ++cpu_id)
        {
            GString *curfreq_filename = 
                sysfs_filename(cpu_id, "scaling_cur_freq");
            GError* file_error = NULL;
            gchar* curfreq_char = NULL;

            if (!g_file_get_contents(curfreq_filename->str, &curfreq_char,
                    NULL, &file_error)) 
            {
                if (ctx->err_fd) {
                    fprintf(ctx->err_fd,
                        "Error opening curfreq file for cpu %ld...\n", cpu_id);
                    fputs(file_error->message, ctx->err_fd);
                }
                g_error_free(file_error);
                ctx->error = PWR_ARCH_UNSUPPORTED;
                g_string_free(curfreq_filename, TRUE);
                return;
            }
        
            speed_t cpu_freq;
            sscanf(curfreq_char, "%ld", &cpu_freq);
            g_free(curfreq_char);
            g_string_free(curfreq_filename, TRUE);

            if (cpu_freq > cur_freq) {
                cur_freq = cpu_freq;
            }
        }

        speed_level_t cur_freq_lvl;
        for (cur_freq_lvl = 0;
             cur_freq_lvl < ctx->phys_islands[island_id]->num_speed_levels;
             ++cur_freq_lvl)
        {
            if (cur_freq == ctx->phys_islands[island_id]->freqs[cur_freq_lvl]) {
                break;
            }
        }

        if (cur_freq_lvl == ctx->phys_islands[island_id]->num_speed_levels) {
            if (ctx->err_fd) {
                fprintf(ctx->err_fd, "Incoherent curfreq file content\n");
            }
            ctx->error = PWR_INIT_ERR;
            return;
        }

        ctx->phys_islands[island_id]->current_speed_level = cur_freq_lvl;
    }

    //===----------------------------------------------------------------------
    // Initialize throttle and speedometer files for each island
    ctx->island_throttle_files = malloc(ctx->num_phys_islands * sizeof(FILE*));

    for (unsigned long island_id = 0;
         island_id < ctx->num_phys_islands;
         ++island_id)
    {
        // Use first CPU in island to throttle and read speed from
        unsigned long cpu_id = ctx->phys_islands[island_id]->cpus[0];

        // Build path to sysfs entries
        GString* throttle_filename = sysfs_filename(cpu_id, "scaling_setspeed");

        // Get FILE*
        ctx->island_throttle_files[island_id] = fopen(throttle_filename->str, "w");
        g_string_free(throttle_filename, TRUE);

        // Sanity check
        if (NULL == ctx->island_throttle_files[island_id]) {
            if (ctx->err_fd) {
                fprintf(ctx->err_fd,
                    "Failed to open DVFS throttle file for cpu %lu\n", cpu_id);
            }
            ctx->error = PWR_INIT_ERR;
            return;
        }

        // Initialize the speed on that island
        // Set the controlling core to max frequency and ignore the rest by
        // setting the lowest speed on them
        for (unsigned long c = 1;
             c < ctx->phys_islands[island_id]->num_cpu;
             ++c)
        {
            GString* tmp_filename = sysfs_filename(c, "scaling_setspeed");
            FILE *setter_fd = fopen(tmp_filename->str, "w");
            g_string_free(tmp_filename, TRUE);
            
            fprintf(setter_fd, "%ld", 
                ctx->phys_islands[island_id]->freqs[ctx->phys_islands[island_id]->min_speed_level]);
            if (ferror(setter_fd)) {
                fclose(setter_fd);
                if (ctx->err_fd) {
                    fprintf(ctx->err_fd,
                        "Failed to set the frequency on cpu %lu\n", c);
                }
                ctx->error = PWR_INIT_ERR;
                return;
            }

            if (fflush(setter_fd)) {
                if (ctx->err_fd) {
                    fprintf(ctx->err_fd,
                        "Failed to set the frequency on cpu %lu\n", c);
                }
                ctx->error = PWR_INIT_ERR;
                fclose(setter_fd);
                return;
            }

            fclose(setter_fd);
        }

        fprintf(ctx->island_throttle_files[island_id], "%ld", 
            ctx->phys_islands[island_id]->freqs[ctx->phys_islands[island_id]->max_speed_level]);
        if (ferror(ctx->island_throttle_files[island_id])) {
            if (ctx->err_fd) {
                fprintf(ctx->err_fd,
                    "Failed to set the frequency on island %lu\n", island_id);
            }
            ctx->error = PWR_INIT_ERR;
            return;
        }

        if (fflush(ctx->island_throttle_files[island_id])) {
            if (ctx->err_fd) {
                fprintf(ctx->err_fd,
                    "Failed to set the frequency on island %lu\n", island_id);
            }
            ctx->error = PWR_INIT_ERR;
            return;
        }

    }

    ctx->module_init |= (1U << PWR_MODULE_DVFS);
    ctx->error = PWR_OK;

    return;
}

void free_speed_data(pwr_ctx_t *ctx) {
    if (ctx == NULL) {
        return;
    }

    if (!pwr_is_initialized(ctx, PWR_MODULE_DVFS)) {
        ctx->error = PWR_UNINITIALIZED;
        return;
    }

    for (unsigned int i = 0; i < ctx->num_phys_islands; ++i) {
        fclose(ctx->island_throttle_files[i]);
        free(ctx->phys_islands[i]->freqs);
    }
    free(ctx->island_throttle_files);

    ctx->error = PWR_OK;
}


//====-------------------------------------------------------------------------
// Local functions
//-----------------------------------------------------------------------------

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
void sort_and_cast_freqs(gchar** freqs, freq_t* sorted, long num_freqs) {
    GArray* tmp_sorted = g_array_new(FALSE, FALSE, sizeof(freq_t));
    for (long i=0; i<num_freqs; ++i) {
        freq_t freq = atoi(freqs[i]);
        g_array_append_val(tmp_sorted, freq);
    }
    g_array_sort(tmp_sorted, (GCompareFunc)(&compare_freq));
    for (long i=0; i<num_freqs; ++i) {
        sorted[i] = g_array_index(tmp_sorted, freq_t, i);
    }
    g_array_free(tmp_sorted, TRUE);
}


/**
  * Compares two frequencies
  *
  * @param f0  Frequency to compare
  * @param f1  Frequency to compare
  * 
  * @return A negative value if f0 is higher than f1, 0 if they are equal or a
  *  positive value otherwise.
  */
gint compare_freq(const freq_t f0, const freq_t f1) {
    return f1 - f0;
}

