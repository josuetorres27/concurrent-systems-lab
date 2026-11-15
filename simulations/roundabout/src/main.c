/**
 * Concurrent Systems Laboratory
 * Module: Roundabout Traffic Concurrency Simulator
 * File: main.c
 *
 * Entry point for the roundabout concurrency simulator.
 * Supports piped inputs or automated synthetic traffic generation.
 *
 * Author: Josue Torres Sibaja
 * Copyright (c) 2024-2025 Josue Torres Sibaja
 * Licensed under the MIT License.
 */

#include "roundabout.h"

int main(int argc, char* argv[]) {
  int min_time = 5;
  int max_time = 20;
  int segment_capacity = 3;
  int verbose_mode = 0;
  int auto_vehicle_count = 15;

  const char* input_file = NULL;
  int positional = 0;

  /* Flags may appear anywhere; the remaining arguments are, in order,
     min_ms, max_ms and capacity */
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
      verbose_mode = 1;
    } else if (strcmp(argv[i], "-f") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Error: -f needs a file name.\n");
        return EXIT_FAILURE;
      }
      input_file = argv[++i];
    } else {
      int value = atoi(argv[i]);
      switch (positional++) {
        case 0:  min_time = value; break;
        case 1:  max_time = value; break;
        case 2:  segment_capacity = value; break;
        default:
          fprintf(stderr, "Error: Unexpected argument: %s\n", argv[i]);
          return EXIT_FAILURE;
      }
    }
  }

  if (min_time < 0 || max_time < 0 || segment_capacity < 1) {
    fprintf(stderr, "Error: Delays must be 0 or more and the capacity at least 1.\n");
    return EXIT_FAILURE;
  }
  if (positional == 1 && max_time < min_time) {
    max_time = min_time;  /* Only min_ms given above the default max_ms */
  }
  if (max_time < min_time) {
    fprintf(stderr, "Error: max_ms (%d) is smaller than min_ms (%d).\n", max_time, min_time);
    return EXIT_FAILURE;
  }

  printf("====================================================================\n");
  printf("      ROUNDABOUT TRAFFIC CONCURRENCY SIMULATOR - POSIX THREADS      \n");
  printf("====================================================================\n");
  printf(" [✓] Segment Capacity: %d vehicles per quadrant\n", segment_capacity);
  printf(" [✓] Transit Delay:    %d - %d ms\n", min_time, max_time);
  printf(" [✓] Verbose Mode:     %s\n", verbose_mode ? "ENABLED" : "DISABLED");
  printf("====================================================================\n\n");

  SimulationState* sim_state = init_simulation(min_time, max_time, verbose_mode, segment_capacity);
  if (!sim_state) {
    fprintf(stderr, "Error: Failed to allocate simulation state.\n");
    return EXIT_FAILURE;
  }

  static pthread_t threads[MAX_VEHICLES];
  int count = 0;

  if (input_file) {
    FILE* f = fopen(input_file, "r");
    if (!f) {
      perror("Error opening input file");
      cleanup_simulation(sim_state);
      return EXIT_FAILURE;
    }
    char entry, exit;
    while (fscanf(f, " %c %c", &entry, &exit) == 2 && count < MAX_VEHICLES) {
      sim_state->vehicles[count].id = count;
      sim_state->vehicles[count].entry = entry;
      sim_state->vehicles[count].exit = exit;
      count++;
    }
    fclose(f);
    printf(" [✓] Loaded %d vehicles from input file: %s\n\n", count, input_file);
  } else {
    /* Synthetic automated traffic stream */
    char directions[] = {'N', 'E', 'S', 'W'};
    srand((unsigned int) time(NULL));
    for (int i = 0; i < auto_vehicle_count; ++i) {
      sim_state->vehicles[i].id = i;
      int e_idx = rand() % 4;
      int x_idx = rand() % 4;
      while (x_idx == e_idx) {
        x_idx = rand() % 4;
      }
      sim_state->vehicles[i].entry = directions[e_idx];
      sim_state->vehicles[i].exit = directions[x_idx];
      count++;
    }
    printf(" [✓] Generated %d synthetic vehicles for traffic flow.\n\n", count);
  }

  sim_state->num_vehicles = count;

  /* Spawn vehicle threads concurrently */
  int started = 0;
  for (int i = 0; i < count; ++i) {
    ThreadArgs* args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
    if (!args) break;
    args->sim_state = sim_state;
    args->vehicle = &sim_state->vehicles[i];
    if (pthread_create(&threads[i], NULL, vehicle_thread, args) != 0) {
      free(args);
      break;
    }
    started++;
  }

  if (started < count) {
    /* Vehicles that were not started would never take their print turn,
       so the started ones could wait forever: stop here */
    fprintf(stderr, "Error: Could only start %d of %d vehicle threads.\n", started, count);
    exit(EXIT_FAILURE);
  }

  /* Wait for all vehicle threads to reach their destination */
  for (int i = 0; i < count; ++i) {
    pthread_join(threads[i], NULL);
  }

  printf("\n====================================================================\n");
  printf(" [✓] Simulation completed successfully with 0 deadlocks.\n");
  printf("====================================================================\n");

  cleanup_simulation(sim_state);
  return EXIT_SUCCESS;
}
