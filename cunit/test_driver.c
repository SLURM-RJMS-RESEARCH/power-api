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



#include "power_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "CUnit/Basic.h"


int initialize() {
    void* hw_behavior;
    void* speed_policy;
    void* scheduling_policy;
    return pwr_initialize(hw_behavior, speed_policy, scheduling_policy);
}

int finalize() {
    return pwr_finalize();
}

int init_suite1(void) {
    return PWR_OK;
}

int clean_suite1(void) {
    return PWR_OK;
}

void test_initialize(void) {
    int initialized = TRUE;
    retval_t ret;
    
    ret = pwr_is_initialized(&initialized);
    CU_ASSERT(PWR_OK == ret);
    CU_ASSERT(FALSE == initialized);

    ret = initialize();
    CU_ASSERT(PWR_OK == ret);

    ret = pwr_is_initialized(&initialized);
    CU_ASSERT(PWR_OK == ret);
    CU_ASSERT(TRUE == initialized);

    ret = finalize();
}

void test_finalize(void) {
    retval_t ret;
    initialize();
    
    int initialized;
    pwr_is_initialized(&initialized);
    ret = pwr_finalize();
    CU_ASSERT(PWR_OK == ret);
    CU_ASSERT(TRUE == initialized);

    ret = pwr_is_initialized(&initialized);
    CU_ASSERT(PWR_OK == ret);
    CU_ASSERT(FALSE == initialized);
}

void test_num_islands(void) {
    retval_t ret;
    initialize();

    long num_islands = -1;
    ret = pwr_num_islands(&num_islands);
    CU_ASSERT(PWR_OK == ret);
    CU_ASSERT(num_islands > 0);
    CU_ASSERT(num_islands < PWR_MAX_ISLANDS);
    //fprintf(stderr, "\nFound %ld voltage islands\n", num_islands);

    finalize();
}

void test_phys_islands(void) {
    retval_t ret;
    initialize();

    long num_islands;
    pwr_num_islands(&num_islands);

    island_id_t* islands = (island_id_t*)malloc(num_islands*sizeof(island_id_t));
    pwr_islands(islands);
    CU_ASSERT(PWR_OK == ret);
    CU_ASSERT(NULL != islands);

    for (int i=0; i<num_islands; ++i) {
        island_id_t id = islands[i];
        CU_ASSERT(id >= 0);
        CU_ASSERT(id < num_islands);
    }

    free(islands);
    finalize();
}

void test_num_speed_levels(void) {
    retval_t ret;
    initialize();

    long num_islands;
    pwr_num_islands(&num_islands);

    long num_speed_levels = -1;
    
    for (int i=0; i<num_islands; ++i) {
        ret = pwr_num_speed_levels(i, &num_speed_levels);
        CU_ASSERT(PWR_OK == ret); 
        CU_ASSERT(num_speed_levels > 0);
        CU_ASSERT(num_speed_levels < PWR_MAX_SPEED_LEVELS);
        //fprintf(stderr, "\nFound %ld speed levels (island %d)\n", num_speed_levels, i);
    }

    finalize();
}

void test_current_speed_level(void) {
    retval_t ret;
    initialize();

    long num_islands;
    pwr_num_islands(&num_islands);

    for (int i=0; i<num_islands; ++i) {
        speed_level_t level = -1;
        ret = pwr_current_speed_level(i, &level);
        CU_ASSERT(PWR_OK == ret);
        CU_ASSERT(level >= 0);
    }

    finalize();
}

void test_request_speed_level(void) {
    retval_t ret;
    initialize();

    long num_islands;
    pwr_num_islands(&num_islands);

    island_id_t* islands = (island_id_t*)malloc(num_islands*sizeof(island_id_t));
    pwr_islands(islands);

    long num_speed_levels;
    for (int i=0; i<num_islands; ++i) {
        pwr_num_speed_levels(i, &num_speed_levels);
        for (speed_level_t level=0; level<num_speed_levels; ++level) {
            ret = pwr_request_speed_level(i, level);
            CU_ASSERT(PWR_OK == ret); 
        }
    }

    free(islands);
    finalize();
}

void test_increase_speed_level(void) {
    retval_t ret;
    initialize();

    long num_islands;
    pwr_num_islands(&num_islands);

    island_id_t* islands = (island_id_t*)malloc(num_islands*sizeof(island_id_t));
    pwr_islands(islands);

    long num_speed_levels;
    for (int i=0; i<num_islands; ++i) {
        pwr_num_speed_levels(i, &num_speed_levels);
        for (speed_level_t base_level=0; base_level<num_speed_levels; ++base_level) {
            pwr_request_speed_level(i, base_level);
            for (int delta=-base_level+1; delta<num_speed_levels-base_level-1; ++delta) {
                ret = pwr_modify_speed_level(i, delta, base_level);
                CU_ASSERT(PWR_OK == ret); 
                pwr_request_speed_level(i, base_level);
            }
        }
    }

    free(islands);
    finalize();
}

void test_agility(void) {
    retval_t ret;
    initialize();

    long num_islands;
    pwr_num_islands(&num_islands);

    island_id_t* islands = (island_id_t*)malloc(num_islands*sizeof(island_id_t));
    pwr_islands(islands);

    long num_speed_levels;
    agility_t best_agility;
	agility_t worst_agility;
    for (int i=0; i<num_islands; ++i) {
        ret = pwr_agility(i, 0, 1, &best_agility, &worst_agility);
        CU_ASSERT(PWR_OK == ret);
        // CU_ASSERT(agility > 0);
        //fprintf(stderr, "\nFound agility %ld (island %d)\n", agility, i);
    }

    finalize();
}

void test_power_energy_counters(void) {
	retval_t ret;
	energy_t e_j, e_uj;
	timestamp_t t_sec, t_nsec;
	initialize();
	
	long num_islands;
	ret = pwr_num_islands(&num_islands);
	island_id_t * islands = (island_id_t*)malloc(num_islands*sizeof(island_id_t));
    ret = pwr_islands(islands);
	CU_ASSERT(PWR_OK == ret);
	
	for ( int i = 0 ; i < num_islands ; i++ ) {
		ret = pwr_energy_counter( islands[i], &e_j, &e_uj, &t_sec, &t_nsec);
		CU_ASSERT(PWR_OK == ret);
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
	/* Quick fix to a strange problem that causes energy measurments to return zero always.
		The underlying cause of this problem is currently unknown. With this method we avoid finalizing
		and initializing the ecount environment multiple times which appears to be the trigger for
		this problem. */
    pwr_ecount_finalize(); 
    // Run all tests
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    
    // Clean up and exit
    CU_cleanup_registry();
    return CU_get_error();
}
