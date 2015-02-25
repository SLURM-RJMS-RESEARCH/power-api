/** 	Copyright 2013-14 Reservoir Labs, Inc.
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



#include "power-api.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "CUnit/Basic.h"

static pwr_ctx_t *ctx;

void initialize() {
    void* hw_behavior = NULL;
    void* speed_policy = NULL;
    void* scheduling_policy = NULL;

    ctx = pwr_initialize(hw_behavior, speed_policy, scheduling_policy);
    CU_ASSERT(ctx != NULL);
}

void finalize() {
    pwr_finalize(ctx);
}

int init_suite1(void) {
    return PWR_OK;
}

int clean_suite1(void) {
    return PWR_OK;
}

void test_initialize(void) {
    
    bool initialized = pwr_is_initialized(NULL, PWR_MODULE_STRUCT);
    CU_ASSERT(false == initialized);

    initialize();
    CU_ASSERT(ctx != NULL);
    CU_ASSERT(PWR_OK == pwr_error(ctx));

    initialized = pwr_is_initialized(ctx, PWR_MODULE_STRUCT);
    CU_ASSERT(PWR_OK == pwr_error(ctx));
    CU_ASSERT(true == initialized);

    initialized = pwr_is_initialized(ctx, PWR_MODULE_DVFS);
    CU_ASSERT(PWR_OK == pwr_error(ctx));
    CU_ASSERT(true == initialized);

    initialized = pwr_is_initialized(ctx, PWR_MODULE_ENERGY);
    CU_ASSERT(PWR_OK == pwr_error(ctx));
    CU_ASSERT(true == initialized);

    finalize(ctx);
}

void test_finalize(void) {
    initialize();
    CU_ASSERT(ctx != NULL);
    CU_ASSERT(PWR_OK == pwr_error(ctx));
    
    pwr_finalize(ctx);
}

void test_num_islands(void) {
    initialize();

    unsigned long num_islands = pwr_num_phys_islands(ctx);
    CU_ASSERT(PWR_OK == pwr_error(ctx));
    CU_ASSERT(num_islands > 0);
    CU_ASSERT(num_islands < PWR_MAX_PHYS_ISLANDS);
    //fprintf(stderr, "\nFound %ld voltage islands\n", num_islands);

    finalize();
}

void test_phys_islands(void) {
    initialize();
    unsigned long num_islands = pwr_num_phys_islands(ctx);

    unsigned long num_cpus = pwr_num_phys_cpus(ctx);
    CU_ASSERT(pwr_error(ctx) == PWR_OK);
    CU_ASSERT(num_cpus > 0);
    CU_ASSERT(num_cpus < PWR_MAX_PHYS_CPU);

    for (unsigned long i = 0; i < num_cpus; ++i) {
        unsigned long cpu_island = pwr_island_of_cpu(ctx, i);
        CU_ASSERT(pwr_error(ctx) == PWR_OK);
        CU_ASSERT(cpu_island < num_islands);
    }

    finalize();
}

void test_num_speed_levels(void) {
    initialize();
    unsigned long num_islands = pwr_num_phys_islands(ctx);

    unsigned int num_speed_levels = 0;
    
    for (unsigned long i = 0; i < num_islands; ++i) {
        num_speed_levels = pwr_num_speed_levels(ctx, i);
        CU_ASSERT(PWR_OK == pwr_error(ctx)); 
        CU_ASSERT(num_speed_levels > 0);
        CU_ASSERT(num_speed_levels < PWR_MAX_SPEED_LEVELS);
        //fprintf(stderr, "\nFound %ld speed levels (island %d)\n", num_speed_levels, i);
    }

    finalize();
}

void test_current_speed_level(void) {
    initialize();
    unsigned long num_islands = pwr_num_phys_islands(ctx);

    for (unsigned long i = 0; i < num_islands; ++i) {
        unsigned int nb_levels = pwr_num_speed_levels(ctx, i);
        unsigned int level = pwr_current_speed_level(ctx, i);
        CU_ASSERT(PWR_OK == pwr_error(ctx));
        CU_ASSERT(level < nb_levels);
    }

    finalize();
}

void test_request_speed_level(void) {
    initialize();
    unsigned long num_islands = pwr_num_phys_islands(ctx);

    for (unsigned long i = 0; i < num_islands; ++i) {
        unsigned int num_speed_levels = pwr_num_speed_levels(ctx, i);
        for (unsigned int level = 0; level < num_speed_levels; ++level) {
            pwr_request_speed_level(ctx, i, level);
            CU_ASSERT(PWR_OK == pwr_error(ctx)); 
        }
    }

    finalize();
}

void test_increase_speed_level(void) {
    initialize();
    unsigned long num_islands = pwr_num_phys_islands(ctx);

    for (unsigned long i = 0; i < num_islands; ++i) {
        unsigned int num_speed_levels = pwr_num_speed_levels(ctx, i);
        for (unsigned int base_level = 0; base_level < num_speed_levels; ++base_level) {
            pwr_request_speed_level(ctx, i, base_level);
            for (int delta= -base_level + 1;
                delta < (int) num_speed_levels - (int) base_level - 1;
                ++delta)
            {
                pwr_increase_speed_level(ctx, i, delta);
                CU_ASSERT(PWR_OK == pwr_error(ctx)); 
                pwr_request_speed_level(ctx, i, base_level);
            }
        }
    }

    finalize();
}

void test_agility(void) {
    initialize();
    unsigned long num_islands = pwr_num_phys_islands(ctx);

    for (unsigned long i = 0; i < num_islands; ++i) {
        agility_t agility = pwr_agility(ctx, i, 0, 1);
        CU_ASSERT(PWR_OK == pwr_error(ctx));
        CU_ASSERT(agility > 0);
    }

    finalize();
}

void test_power_energy_counters(void) {
    initialize();

    pwr_start_energy_count(ctx);
    CU_ASSERT(pwr_error(ctx) == PWR_OK);
    sleep(1);
    const pwr_emeas_t *res = pwr_stop_energy_count(ctx);
    CU_ASSERT(pwr_error(ctx) == PWR_OK);
    CU_ASSERT(res != NULL);
    CU_ASSERT(res->nbValues > 0);

    for (unsigned int i = 0; i < res->nbValues; ++i) {
        printf("i = %u\n", i);
        printf("%s %s\n", res->names[i], res->units[i]);
        CU_ASSERT(res->values[i] > 0);

        CU_ASSERT(res->names[i] != NULL);
        CU_ASSERT(strlen(res->names[i]) > 0);

        CU_ASSERT(res->units[i] != NULL);
        CU_ASSERT(strlen(res->units[i]) > 0);
    }

	finalize();
}

void test_increase_voltage(void) {
    CU_ASSERT(!PWR_UNIMPLEMENTED);
}

void test_efficiency(void) {
    CU_ASSERT(!PWR_UNIMPLEMENTED);
}

void test_set_power_priority(void) {
    CU_ASSERT(!PWR_UNIMPLEMENTED);
}

void test_set_speed_priority(void) {
    CU_ASSERT(!PWR_UNIMPLEMENTED);
}

int main() {
    // Set up CUnit test suite
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    pSuite = CU_add_suite("PowerAPI Black Box Unit Test Suite", init_suite1, clean_suite1);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Add tests to suite
    if (NULL == CU_add_test(pSuite, 
                            "pwr_initialize()",           
                            test_initialize)             ||
        NULL == CU_add_test(pSuite, 
                            "pwr_finalize()",             
                            test_finalize)               ||
        NULL == CU_add_test(pSuite, 
                            "pwr_num_islands()", 
                            test_num_islands)       ||
        NULL == CU_add_test(pSuite, 
                            "pwr_islands()",     
                            test_phys_islands)           ||
        NULL == CU_add_test(pSuite, 
                            "pwr_num_speed_levels()", 
                            test_num_speed_levels)       ||
        NULL == CU_add_test(pSuite, 
                            "pwr_current_speed_level()",    
                            test_current_speed_level)    ||
        NULL == CU_add_test(pSuite, 
                            "pwr_request_speed_level()",        
                            test_request_speed_level)    ||
        NULL == CU_add_test(pSuite, 
                            "pwr_modify_speed_level()",       
                            test_increase_speed_level)   ||
        NULL == CU_add_test(pSuite, 
                            "pwr_agility()",           
                            test_agility)                ||
        NULL == CU_add_test(pSuite, 
                            "pwr_increase_voltage()",     
                            test_increase_voltage)       ||
        NULL == CU_add_test(pSuite, 
                            "pwr_efficiency()",           
                            test_efficiency)             ||
        NULL == CU_add_test(pSuite, 
                            "pwr_set_power_priority()",   
                            test_set_power_priority)     ||
        NULL == CU_add_test(pSuite, 
                            "pwr_set_speed_priority()",   
                            test_set_speed_priority)	 ||
		NULL == CU_add_test(pSuite,
							"pwr_energy_counters()",
							test_power_energy_counters)) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    // Run all tests
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    
    // Clean up and exit
    CU_cleanup_registry();
    return CU_get_error();
}
