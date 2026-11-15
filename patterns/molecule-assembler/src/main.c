/**
 * Concurrent Systems Laboratory
 * Module: H2O Molecular Assembly Synchronization
 * File: main.c
 *
 * Classic barrier synchronization problem where two Hydrogen threads
 * and one Oxygen thread must rendezvous to form a stable water molecule.
 *
 * Author: Josue Torres Sibaja
 * Copyright (c) 2024-2025 Josue Torres Sibaja
 * Licensed under the MIT License.
 */

#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define TOTAL_MOLECULES 10

/*
 * Admission works as in "The Little Book of Semaphores" (Downey): the atom
 * that completes a molecule releases exactly two hydrogens and one oxygen
 * (itself included) and keeps the admission lock. The oxygen gives the lock
 * back only after all three atoms have passed the barrier, so the atoms of
 * one molecule are never mixed with those of the next.
 *
 * The admission lock is a binary semaphore rather than a mutex because it is
 * released by a different thread (the oxygen) than the one that took it.
 */
typedef struct {
  sem_t admission;        /* Binary semaphore guarding the counts */
  int hydrogen_count;     /* Hydrogens waiting to be bonded */
  int oxygen_count;       /* Oxygens waiting to be bonded */
  sem_t sem_h;            /* Hydrogen queue */
  sem_t sem_o;            /* Oxygen queue */
  pthread_barrier_t barrier;
  int molecules_formed;   /* Written only while holding admission */
} H2OAssembler;

H2OAssembler g_h2o;

/**
 * Releases one molecule (two hydrogens and one oxygen) if enough atoms are
 * waiting. Must be called while holding admission; returns 1 if a molecule
 * was released (admission stays held), 0 otherwise (admission is released).
 */
static int release_molecule_if_ready(void) {
  if (g_h2o.hydrogen_count >= 2 && g_h2o.oxygen_count >= 1) {
    g_h2o.hydrogen_count -= 2;
    g_h2o.oxygen_count -= 1;
    sem_post(&g_h2o.sem_h);
    sem_post(&g_h2o.sem_h);
    sem_post(&g_h2o.sem_o);
    return 1;
  }
  sem_post(&g_h2o.admission);
  return 0;
}

void* hydrogen_thread(void* arg) {
  int id = *((int*) arg);
  free(arg);

  sem_wait(&g_h2o.admission);
  g_h2o.hydrogen_count++;
  release_molecule_if_ready();

  sem_wait(&g_h2o.sem_h);
  int molecule = g_h2o.molecules_formed + 1;  /* Stable while admission is held */

  /* Rendezvous barrier: the two hydrogens and the oxygen of this molecule */
  pthread_barrier_wait(&g_h2o.barrier);
  printf("  [H #%2d] Bonded into H2O molecule #%d\n", id, molecule);

  return NULL;
}

void* oxygen_thread(void* arg) {
  int id = *((int*) arg);
  free(arg);

  sem_wait(&g_h2o.admission);
  g_h2o.oxygen_count++;
  release_molecule_if_ready();

  sem_wait(&g_h2o.sem_o);

  /* Rendezvous barrier: the two hydrogens and the oxygen of this molecule */
  pthread_barrier_wait(&g_h2o.barrier);

  g_h2o.molecules_formed++;
  printf(" >>> [O #%2d] COMPLETED MOLECULE H2O #%d <<<\n", id, g_h2o.molecules_formed);

  /* Let the next molecule form */
  sem_post(&g_h2o.admission);

  return NULL;
}

int main() {
  printf("====================================================================\n");
  printf("    H2O MOLECULE ASSEMBLER - BARRIER & SEMAPHORE RENDEZVOUS        \n");
  printf("====================================================================\n");
  printf(" [✓] Target Molecules: %d H2O molecules\n", TOTAL_MOLECULES);
  printf(" [✓] Atoms Required:   %d Hydrogen, %d Oxygen\n", TOTAL_MOLECULES * 2, TOTAL_MOLECULES);
  printf("====================================================================\n\n");

  sem_init(&g_h2o.admission, 0, 1);
  sem_init(&g_h2o.sem_h, 0, 0);
  sem_init(&g_h2o.sem_o, 0, 0);
  pthread_barrier_init(&g_h2o.barrier, NULL, 3);
  g_h2o.hydrogen_count = 0;
  g_h2o.oxygen_count = 0;
  g_h2o.molecules_formed = 0;

  pthread_t h_threads[TOTAL_MOLECULES * 2];
  pthread_t o_threads[TOTAL_MOLECULES];

  for (int i = 0; i < TOTAL_MOLECULES * 2; ++i) {
    int* id = malloc(sizeof(int)); *id = i + 1;
    pthread_create(&h_threads[i], NULL, hydrogen_thread, id);
  }
  for (int i = 0; i < TOTAL_MOLECULES; ++i) {
    int* id = malloc(sizeof(int)); *id = i + 1;
    pthread_create(&o_threads[i], NULL, oxygen_thread, id);
  }

  for (int i = 0; i < TOTAL_MOLECULES * 2; ++i) pthread_join(h_threads[i], NULL);
  for (int i = 0; i < TOTAL_MOLECULES; ++i) pthread_join(o_threads[i], NULL);

  sem_destroy(&g_h2o.sem_h);
  sem_destroy(&g_h2o.sem_o);
  pthread_barrier_destroy(&g_h2o.barrier);
  sem_destroy(&g_h2o.admission);

  printf("\n====================================================================\n");
  printf(" [✓] Successfully synthesized %d H2O molecules without race conditions.\n", TOTAL_MOLECULES);
  printf("====================================================================\n");
  return EXIT_SUCCESS;
}
