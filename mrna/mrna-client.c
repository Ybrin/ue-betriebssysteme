/**
 * @file mrna-client.c
 * @author Koray Koska <e1528624@student.tuwien.ac.at>
 * @date 2018-01-07
 *
 * @brief Client for OSUE exercise 3 'mrna'.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#include <assert.h>
#include <ctype.h>
#include <math.h>

#include <assert.h>
#include <errno.h>
#include <signal.h>

#include <time.h>

#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

// Commons
#include "common.h"

// Shared memory etc
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

// Semaphores
#include <semaphore.h>


/// Name of this program
static const char *programName = "mrna-client";

/// The SharedMemory object
static SharedMemory *sharedMemory;

/// The first semaphore
static sem_t *sem1;

/// The semaphore for waiting for clients
static sem_t *sem2;

/// The semaphore for waiting for client to finish reading
static sem_t *sem3;

/// The semaphore for clients to wait
static sem_t *sem4;

/**
 * Prints the usage and exits.
 */
static void usage(void);

/**
 * Creates the shared memory
 */
static void createSharedMemory(void);

/**
 * Creates the needed semaphores
 */
static void createSemaphores(void);

/**
 * Cleanup resources
 */
static void cleanup(void);


int main(int argc, char *argv[]) {
  if (argc > 0) {
    programName = argv[0];
  }
  if (argc > 1) {
    usage();
  }
  // Cleanup on exit
  if (atexit(cleanup) != 0) {
    fprintf(stderr, "Could not set atexit cleanup function.\n");
    exit(EXIT_FAILURE);
  }

  // Create shared memory object
  createSharedMemory();

  // Create semaphores
  createSemaphores();

  // Test
  // printf("The number %u was read!\n", sharedMemory->data[1]);
  // sharedMemory->data[0] = 0xF;

  // Submit
  if (sem_wait(sem1) != 0) {
    fprintf(stderr, "sem_wait failed\n");
    exit(EXIT_FAILURE);
  }
  sharedMemory->data[0] = 's';
  char *test = "CACAGCCUAUGGUAGGGUCUGCUUAACGCCGCUAUGUAUCAUAAGGAGACAUGACGCCGC";
  for (int i = 0; i < 60; i++) {
    sharedMemory->data[i + 1] = test[i];
  }
  sharedMemory->data[61] = SHM_END_BYTE;

  // Nice
  sem_post(sem2);

  sem_wait(sem4);

  // Read
  printf("%u\n", sharedMemory->data[0]);
  printf("%u\n", sharedMemory->data[1]);
  sem_post(sem3);


  return EXIT_SUCCESS;
}

static void createSharedMemory(void) {
  int shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, SHM_PERMISSION);
  if (shmfd == -1) {
    fprintf(stderr, "Shared memory allocation failed.\n");
    exit(EXIT_FAILURE);
  }

  sharedMemory = mmap(NULL, sizeof *sharedMemory, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

  if (sharedMemory == MAP_FAILED) {
    fprintf(stderr, "mmap failed\n");
    exit(EXIT_FAILURE);
  }

  if (close(shmfd) == -1) {
    fprintf(stderr, "Closing the shared memory file failed.\n");
    exit(EXIT_FAILURE);
  }
}

static void createSemaphores(void) {
  sem1 = sem_open(SEM_1, 0);
  if (sem1 == SEM_FAILED) {
    fprintf(stderr, "sem_open failed\n");
    exit(EXIT_FAILURE);
  }
  sem2 = sem_open(SEM_2, 0);
  if (sem2 == SEM_FAILED) {
    fprintf(stderr, "sem_open failed\n");
    exit(EXIT_FAILURE);
  }
  sem3 = sem_open(SEM_3, 0);
  if (sem3 == SEM_FAILED) {
    fprintf(stderr, "sem_open failed\n");
    exit(EXIT_FAILURE);
  }

  sem4 = sem_open(SEM_4, 0);
  if (sem4 == SEM_FAILED) {
    fprintf(stderr, "sem_open failed\n");
    exit(EXIT_FAILURE);
  }
}

static void cleanup(void) {
  printf("Cleaning up\n");
  // Cleanup
  if (munmap(sharedMemory, sizeof *sharedMemory) == -1) {
    fprintf(stderr, "munmap failed\n");
  }
  if (sem_close(sem1) == -1) {
    fprintf(stderr, "sem_unlink failed\n");
  }
  if (sem_close(sem2) == -1) {
    fprintf(stderr, "sem_unlink failed\n");
  }
  if (sem_close(sem3) == -1) {
    fprintf(stderr, "sem_unlink failed\n");
  }
  if (sem_close(sem4) == -1) {
    fprintf(stderr, "sem_unlink failed\n");
  }
}

static void usage(void) {
  fprintf(stderr, "Usage: %s\n", programName);
  exit(EXIT_FAILURE);
}

