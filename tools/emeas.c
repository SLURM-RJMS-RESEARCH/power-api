/**
  * Copyright 2014 Reservoir Labs, Inc.
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

/*
 * This program is able to measure energy consumption while a program runs. The
 * measurement stops immediately after the command execution ends. Execution
 * time as well as energy are reported for every available energy probes.
 *
 * Usage:
 *  emeas command
 *
 * Typical output with RAPL:
 *  time: 1.002 s.
 *  PACKAGE_ENERGY:PACKAGE0: 4137512207 nJ
 *  DRAM_ENERGY:PACKAGE0: 881835937 nJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "power-api.h"

int main(int argc, char **argv) {
    pid_t son;
    const pwr_emeas_t *res;

    if (argc < 2) {
        printf("Usage: %s commandline\n", argv[0]);
        return EXIT_FAILURE;
    }

    pwr_ctx_t *ctx = pwr_initialize(NULL, NULL, NULL);

    if (!pwr_is_initialized(ctx, PWR_MODULE_ENERGY)) {
        fprintf(stderr, "Failed to initialize the energy module\n");
        return EXIT_FAILURE;
    }

    pwr_start_energy_count(ctx);

    if (!(son = fork())) {
        execvp(argv[1], argv + 1);
        perror("Failed to run the command");
        return EXIT_FAILURE;
    }

    waitpid(son, NULL, 0);
    res = pwr_stop_energy_count(ctx);

    printf("time: %.3f s.\n", res->duration);
    for (unsigned int i = 0; i < res->nbValues; ++i) {
        printf("%s: %lld %s\n", res->names[i], res->values[i], res->units[i]);
    }

    pwr_finalize(ctx);
}
