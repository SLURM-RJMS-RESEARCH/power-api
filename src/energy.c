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
  */

#ifdef HAS_PAPI

#include <assert.h>
#include <papi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internals.h"

/** Internal null, constant measurement results */
static pwr_emeas_t emeas_zero = { 0, 0, NULL, NULL, NULL };

//====-------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

void pwr_start_energy_count(pwr_ctx_t *ctx) {
    if (ctx == NULL) {
        ctx->error = PWR_INIT_ERR;
        return;
    }
    
    if (!pwr_is_initialized(ctx, PWR_MODULE_ENERGY)) {
        ctx->error = PWR_UNINITIALIZED;
        return;
    }

    ctx->error = PWR_OK;

    if (ctx->emeas_running) {
        pwr_stop_energy_count(ctx);
    }

    PAPI_reset(ctx->event_set);
    ctx->emeas->duration = PAPI_get_real_nsec();
    ctx->emeas_running = true;
    PAPI_start(ctx->event_set);
}

const pwr_emeas_t *pwr_stop_energy_count(pwr_ctx_t *ctx) {
    // fast status check here
    if (ctx == NULL || !(ctx->module_init & (1U << PWR_MODULE_ENERGY))) {
        return &emeas_zero;
    }

    if (!ctx->emeas_running) {
        ctx->error = PWR_UNAVAILABLE;
        return &emeas_zero;
    }

    PAPI_stop(ctx->event_set, ctx->emeas->values);
    ctx->emeas->duration = (PAPI_get_real_nsec() - ctx->emeas->duration) / 1e9;
    ctx->emeas_running = false;

    ctx->error = PWR_OK;
    return ctx->emeas;
}


//====-------------------------------------------------------------------------
// Library internal functions
//-----------------------------------------------------------------------------

void init_energy(pwr_ctx_t *ctx) {
    char buf[1024] = { '\0' };
    int ret, num_comp, cid, code = 0;
    const PAPI_component_info_t *comp_info = NULL;
	PAPI_event_info_t evinfo;

    assert(ctx != NULL);
    assert(pwr_is_initialized(ctx, PWR_MODULE_STRUCT));
    assert(!pwr_is_initialized(ctx, PWR_MODULE_ENERGY));

    // Initialize PAPI
	if ( !PAPI_is_initialized() ) {
    	ret = PAPI_library_init(PAPI_VER_CURRENT);
    	if (PAPI_VER_CURRENT != ret ) {
            PAPI_shutdown();
            if (ctx->err_fd) {
                fprintf(ctx->err_fd, "Unexpected PAPI version\n");
            }
            ctx->error = PWR_ARCH_UNSUPPORTED;
            return;
    	}
	}

    // Search PAPI components for RAPL (Intel energy counting) events
    num_comp = PAPI_num_components();

    for (cid = 0; cid < num_comp; ++cid) {
        comp_info = PAPI_get_component_info(cid);
        if (NULL == comp_info) {
            PAPI_shutdown();
            if (ctx->err_fd) {
                fprintf(ctx->err_fd, "Invalid PAPI module found\n");
            }
            ctx->error = PWR_INIT_ERR;
            return;
        }
        if (strstr(comp_info->name, "rapl")) {
            if (0 == comp_info->num_native_events) {
                PAPI_shutdown();
                if (ctx->err_fd) {
                    fprintf(ctx->err_fd, "RAPL counters not available\n");
                }
                ctx->error = PWR_UNAVAILABLE;
                return;
            }
            break;
        }
    }

    if (cid == num_comp) {
        if (ctx->err_fd) {
            fprintf(ctx->err_fd, "Cannot find RAPL module in PAPI\n");
        }
        ctx->error = PWR_UNAVAILABLE;
        return;
    }

    // Create PAPI event set
    ctx->event_set = PAPI_NULL;
    ret = PAPI_create_eventset(&ctx->event_set);
    if (PAPI_OK != ret) {
        PAPI_shutdown();
        if (ctx->err_fd) {
            fprintf(ctx->err_fd, "Failed to create a PAPI event set\n");
        }
        ctx->error = PWR_INIT_ERR;
        return;
    }

    // count how many counters can be added
    ctx->emeas = malloc(sizeof(*ctx->emeas));
    ctx->emeas->nbValues = 0;
    for (int i = 0; i < PWR_MAX_PHYS_CPU; ++i) {
        snprintf(buf, 1024, "PACKAGE_ENERGY:PACKAGE%d", i);
        ret = PAPI_query_named_event(buf);
        if (PAPI_OK != ret) {
            break;
        }
        ++ctx->emeas->nbValues;
    }    
    for (int i = 0; i < PWR_MAX_PHYS_CPU; ++i) {
        snprintf(buf, 1024, "DRAM_ENERGY:PACKAGE%d", i);
        ret = PAPI_query_named_event(buf);
        if (PAPI_OK != ret) {
            break;
        }
        ++ctx->emeas->nbValues;
    }

    ctx->emeas->values = malloc(ctx->emeas->nbValues * sizeof(*ctx->emeas->values));
    ctx->emeas->units = malloc(ctx->emeas->nbValues * sizeof(*ctx->emeas->units));
    ctx->emeas->names = malloc(ctx->emeas->nbValues * sizeof(*ctx->emeas->names));

    // fetch the counter information
    unsigned int cpt_i = 0;
    for (int i = 0; i < PWR_MAX_PHYS_CPU; ++i) {
        snprintf(buf, 1024, "PACKAGE_ENERGY:PACKAGE%d", i);
        ret = PAPI_query_named_event(buf);
        if (PAPI_OK != ret) {
            break;
        }

        ret = PAPI_add_named_event(ctx->event_set, buf);
        PAPI_event_name_to_code(buf, &code);
        PAPI_get_event_info(code, &evinfo);
        ctx->emeas->units[cpt_i] = strdup(evinfo.units);
        ctx->emeas->names[cpt_i] = strdup(buf);
        ++cpt_i;
    }

    // same for DRAM
    for (int i = 0; i < PWR_MAX_PHYS_CPU; ++i) {
        snprintf(buf, 1024, "DRAM_ENERGY:PACKAGE%d", i);
        ret = PAPI_query_named_event(buf);
        if (PAPI_OK != ret) {
            break;
        }

        ret = PAPI_add_named_event(ctx->event_set, buf);
        PAPI_event_name_to_code(buf, &code);
        PAPI_get_event_info(code, &evinfo);
        ctx->emeas->units[cpt_i] = strdup(evinfo.units);
        ctx->emeas->names[cpt_i] = strdup(buf);
        ++cpt_i;
    }

    ctx->emeas_running = false;
    ctx->error = PWR_OK;
    ctx->module_init |= (1U << PWR_MODULE_ENERGY);

    return;
}

void free_energy_data(pwr_ctx_t *ctx) {
    if (ctx == NULL) {
        return;
    }

    if (!pwr_is_initialized(ctx, PWR_MODULE_ENERGY)) {
        ctx->error = PWR_UNINITIALIZED;
        return;
    }

    
    if (ctx->emeas_running) {
        pwr_stop_energy_count(ctx);
    }

    PAPI_cleanup_eventset(ctx->event_set);
    PAPI_destroy_eventset(&ctx->event_set);
    PAPI_shutdown();

    for (unsigned int i = 0; i < ctx->emeas->nbValues; ++i) {
        free(ctx->emeas->units[i]);
        free(ctx->emeas->names[i]);
    }
    free(ctx->emeas->values);
    free(ctx->emeas->units);
    free(ctx->emeas->names);
    free(ctx->emeas);

    ctx->error = PWR_OK;
}

// PAPI is missing? provide failsafe implementation
#else

#include <assert.h>
#include <stdlib.h>

#include "internals.h"

static pwr_emeas_t emeas_zero = { 0, 0, NULL, NULL, NULL };

void pwr_start_energy_count(pwr_ctx_t *ctx) {
    assert(ctx != NULL);

    ctx->error = PWR_ARCH_UNSUPPORTED;
}

const pwr_emeas_t *pwr_stop_energy_count(pwr_ctx_t *ctx) {
    assert(ctx != NULL);

    ctx->error = PWR_ARCH_UNSUPPORTED;
    return &emeas_zero;
}

void init_energy(pwr_ctx_t *ctx) {
    assert(ctx != NULL);

    ctx->error = PWR_ARCH_UNSUPPORTED;
}

void free_energy_data(pwr_ctx_t *ctx) {
    if (ctx == NULL) {
        return;
    }

    ctx->error = PWR_ARCH_UNSUPPORTED;
}

#endif
