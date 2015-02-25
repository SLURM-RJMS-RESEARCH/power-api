/**
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
  *
  * @authors M. Deneroff <deneroff@reservoir.com> <br>
  * @authors Benoit Meister <meister@reservoir.com<br>
  * @authors Tom Henretty <henretty@reservoir.com><br>
  * @authors Benoit Pradelle <pradelle@reservoir.com><br>
  */

#include <assert.h>
#include <glib.h>
#include <stdlib.h>

#include "internals.h"


//====-------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

bool pwr_is_initialized(const pwr_ctx_t *ctx, const pwr_module_id_t module) {
    if (ctx == NULL || module >= PWR_NB_MODULES) {
        return false;
    }

    return (ctx->module_init & (1U << module)) != 0;
}


pwr_ctx_t *pwr_initialize(void* hw_behavior, void* speed_policy,
    void* scheduling_policy)
{
    (void) hw_behavior;
    (void) speed_policy;
    (void) scheduling_policy;

    // create a new context
    pwr_ctx_t *ctx = malloc(sizeof(*ctx));

    ctx->module_init = 0;
    ctx->error = PWR_OK;
    ctx->err_fd = stderr;

    // Initialize physical islands info
    init_struct_module(ctx);

    // only attempt to initialize the rest if the hardware hierarchy has been
    // resolved.
    if (ctx->error == PWR_OK) {

        // Initialize physical speeds info
        init_speed_levels(ctx);

        // Initialize energy-related features
        init_energy(ctx);
    }

    return ctx;
}

void pwr_finalize(pwr_ctx_t *ctx) {
    assert (ctx != NULL);

    if (pwr_is_initialized(ctx, PWR_MODULE_ENERGY)) {
        free_energy_data(ctx);
    }

    if (pwr_is_initialized(ctx, PWR_MODULE_DVFS)) {
        free_speed_data(ctx);
    }

    if (pwr_is_initialized(ctx, PWR_MODULE_STRUCT)) {
        free_structure_data(ctx);
    }

    free(ctx);
}

int pwr_error(const pwr_ctx_t *ctx) {
    if (ctx == NULL) {
        return PWR_UNINITIALIZED;
    }

    return ctx->error;
}

const char *pwr_strerror(const pwr_ctx_t *ctx) {
    if (ctx == NULL) {
        return "Invalid context";
    }

    switch (ctx->error) {
        case PWR_ARCH_UNSUPPORTED: return "Unsupported architecture";
        case PWR_UNIMPLEMENTED: return "Feature not implemented";
        case PWR_UNINITIALIZED: return "Non-initialized context";
        case PWR_OK: return "Success";
        case PWR_ERR: return "General error";
        case PWR_UNAVAILABLE: return "The requested feature is not available";
        case PWR_REQUEST_DENIED: return "The last request was denied";
        case PWR_INIT_ERR: return "Initialization error";
        case PWR_FINAL_ERR: return "Finalization error";
        case PWR_ALREADY_INITIALIZED: return "Already initialized";
        case PWR_IO_ERR: return "I/O error";
        case PWR_UNSUPPORTED_SPEED_LEVEL: return "Unsupported speed level";
        case PWR_UNSUPPORTED_VOLTAGE: return "Unsupported voltage";
        case PWR_ALREADY_MINMAX: return "Already at min/max speed";
        case PWR_OVER_E_BUDGET: return "Over energy budget";
        case PWR_OVER_P_BUDGET: return "Over power budget";
        case PWR_OVER_T_BUDGET: return "Over thermal budget";
        case PWR_INVALID_ISLAND: return "Invalid island identifier";
        case PWR_DVFS_ERR: return "Generic DVFS error";
        default: return "Unknown error";
    }

    return "Unknown error";
}

//====-------------------------------------------------------------------------
// Library internal functions
//-----------------------------------------------------------------------------

GString* sysfs_filename(unsigned long cpu_id, const char* filename) {
    GString* new_filename = g_string_new("/sys/devices/system/cpu/cpu");
    char cpu_str[21];// Maximum chars in base 10 long value is 20 + 1 (\0)
    sprintf(cpu_str, "%ld", cpu_id);

    g_string_append(new_filename, cpu_str);
    g_string_append(new_filename, "/cpufreq/");
    g_string_append(new_filename, filename);

    return new_filename;
}

