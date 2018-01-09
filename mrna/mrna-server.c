/**
 * @file mrna-server.c
 * @author Koray Koska <e1528624@student.tuwien.ac.at>
 * @date 2018-01-07
 *
 * @brief Server for OSUE exercise 3 'mrna'.
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

// Signals
#include <signal.h>


/// Name of this program
static const char *programName = "mrna-server";

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

/// Signal handling
static volatile sig_atomic_t wantsQuit = 0;

/// All connected mrnas from clients and the count
static uint8_t mrnaCount = 0;
static uint8_t **mrnas;
static int *mrnaPointers;

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

/**
 * Submit command
 */
static void submit(void);

/**
 * Next sequence command
 */
static void nextSequence(void);

/**
 * Reset command
 */
static void reset(void);

/**
 * Quit command
 */
static void quit(void);

/**
 * Reset shared memory data
 */
static void resetSharedMemory(void);

/**
 * Catch signals
 */
static void catchSignals(int signo);


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

  // Signal handlers
  if (signal(SIGINT, catchSignals) == SIG_ERR) {
    fprintf(stderr, "Signal handling impossible.\n");
    exit(EXIT_FAILURE);
  }
  if (signal(SIGTERM, catchSignals) == SIG_ERR) {
    fprintf(stderr, "Signal handling impossible.\n");
    exit(EXIT_FAILURE);
  }

  // Create shared memory object
  createSharedMemory();

  // Create semaphores
  createSemaphores();

  // Test
  /*
  sharedMemory->data[1] = 0xA;
  int something = 0;
  while (!something) {
    if (sharedMemory->data[0] == 0xF) {
      something = 1;
    } else {
      sleep(1);
    }
  }*/

  // Wait for the first client with sem2
  while (!wantsQuit) {
    // Wait until a client wants to send a request.
    // printf("Waiting for 2...\n");
    if (sem_wait(sem2) != 0) {
      // Let signal handler and atexit handle this
      if (errno == EINTR) {
        continue;
      }
      fprintf(stderr, "sem_wait failed\n");
      exit(EXIT_FAILURE);
    }

    // Process the request. Read from shared memory.
    uint8_t command = sharedMemory->data[0];
    switch (command) {
      case 's':
        submit();
        break;
      case 'n':
        nextSequence();
        // printf("Sequenzo n\n");
        break;
      case 'r':
        reset();
        break;
      case 'q':
        quit();
        break;
      default:
        break;
    }

    // printf("Posting 4\n");

    // Tell client to start reading
    if (sem_post(sem4) != 0) {
      fprintf(stderr, "sem_wait failed\n");
      exit(EXIT_FAILURE);
    }

    // printf("Waiting for 3\n");

    // Wait for client to finish reading
    if (sem_wait(sem3) != 0) {
      // Let signal handler and atexit handle this
      if (errno == EINTR) {
        continue;
      }
      fprintf(stderr, "sem_wait failed\n");
      exit(EXIT_FAILURE);
    }

    // printf("Posting 1\n");

    // We are done. Tell clients that they can send new requests now.
    if (sem_post(sem1) != 0) {
      fprintf(stderr, "sem_post failed\n");
      exit(EXIT_FAILURE);
    }
  }

  return EXIT_SUCCESS;
}

static void createSharedMemory(void) {
  int shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, SHM_PERMISSION);
  if (shmfd == -1) {
    fprintf(stderr, "Shared memory allocation failed.\n");
    exit(EXIT_FAILURE);
  }
  if (ftruncate(shmfd, sizeof *sharedMemory) == -1) {
    fprintf(stderr, "ftrunctate failed\n");
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
  // Don't block so the first client can connect
  sem1 = sem_open(SEM_1, O_CREAT | O_EXCL, 0600, 1);
  if (sem1 == SEM_FAILED) {
    fprintf(stderr, "sem_open failed\n");
    exit(EXIT_FAILURE);
  }
  // Start with 0 so we block immediately while waiting for clients
  sem2 = sem_open(SEM_2, O_CREAT | O_EXCL, 0600, 0);
  if (sem2 == SEM_FAILED) {
    fprintf(stderr, "sem_open failed\n");
    exit(EXIT_FAILURE);
  }

  sem3 = sem_open(SEM_3, O_CREAT | O_EXCL, 0600, 0);
  if (sem3 == SEM_FAILED) {
    fprintf(stderr, "sem_open failed\n");
    exit(EXIT_FAILURE);
  }

  sem4 = sem_open(SEM_4, O_CREAT | O_EXCL, 0600, 0);
  if (sem4 == SEM_FAILED) {
    fprintf(stderr, "sem_open failed\n");
    exit(EXIT_FAILURE);
  }}

static void cleanup(void) {
  // printf("Cleaning up\n");
  // Cleanup
  if (munmap(sharedMemory, sizeof *sharedMemory) == -1) {
    fprintf(stderr, "munmap failed\n");
  }
  if (shm_unlink(SHM_NAME) == -1) {
    fprintf(stderr, "shm_unlink failed\n");
  }
  if (sem_close(sem1) == -1) {
    fprintf(stderr, "sem_close failed\n");
  }
  if (sem_close(sem2) == -1) {
    fprintf(stderr, "sem_close failed\n");
  }
  if (sem_close(sem3) == -1) {
    fprintf(stderr, "sem_close failed\n");
  }
  if (sem_close(sem4) == -1) {
    fprintf(stderr, "sem_close failed\n");
  }
  if (sem_unlink(SEM_1) == -1) {
    fprintf(stderr, "sem_unlink failed\n");
  }
  if (sem_unlink(SEM_2) == -1) {
    fprintf(stderr, "sem_unlink failed\n");
  }
  if (sem_unlink(SEM_3) == -1) {
    fprintf(stderr, "sem_unlink failed\n");
  }
  if (sem_unlink(SEM_4) == -1) {
    fprintf(stderr, "sem_unlink failed\n");
  }

  // Free everything
  free(mrnas);
  free(mrnaPointers);
}

static void submit(void) {
  int finished = 0;
  int i = 0;
  while (!finished) {
    if (i >= SHM_MAX_DATA) {
      // Failure
      resetSharedMemory();
      sharedMemory->data[0] = SHM_ERROR_BYTE;
      sharedMemory->data[1] = SHM_END_BYTE;
      return;
    }
    if (sharedMemory->data[i + 1] == SHM_END_BYTE) {
      finished = 1;
    }

    i++;
  }

  uint8_t *mrna = malloc((sizeof(uint8_t)) * i);
  for (int j = 0; j < i; j++) {
    mrna[j] = sharedMemory->data[j + 1];
  }

  // Save globally
  mrnaCount++;
  if (mrnaCount == 0) {
    mrnas = malloc(sizeof(uint8_t*));
    mrnaPointers = malloc(sizeof(int));
  } else {
    mrnas = realloc(mrnas, (sizeof(uint8_t*)) * mrnaCount);
    mrnaPointers = realloc(mrnaPointers, (sizeof(int)) * mrnaCount);
  }
  mrnas[mrnaCount - 1] = mrna;
  mrnaPointers[mrnaCount - 1] = 0;

  // Send client id and success
  resetSharedMemory();
  sharedMemory->data[0] = SHM_SUCCESS_BYTE;
  sharedMemory->data[1] = mrnaCount - 1;
  sharedMemory->data[2] = SHM_END_BYTE;
}

static void nextSequence(void) {
  uint8_t clientId = sharedMemory->data[1];
  if (clientId >= mrnaCount) {
    // Error
    resetSharedMemory();
    sharedMemory->data[0] = SHM_ERROR_BYTE;
    sharedMemory->data[1] = SHM_END_BYTE;
    return;
  }

  uint8_t start = 0;
  uint8_t end = 0;

  uint8_t *mrna = mrnas[clientId];
  if (mrna == 0) {
    // Error, client requested quit before.
    resetSharedMemory();
    sharedMemory->data[0] = SHM_ERROR_BYTE;
    sharedMemory->data[1] = SHM_END_BYTE;
    return;
  }
  int pointer = mrnaPointers[clientId];
  int running = 1;
  while (running) {
    uint8_t c = mrna[pointer];
    if (c == SHM_END_BYTE) {
      resetSharedMemory();
      sharedMemory->data[0] = SHM_END_REACHED_BYTE;
      sharedMemory->data[1] = SHM_END_BYTE;
      return;
    }

    if (c == 'A') {
      if (mrna[pointer + 1] != SHM_END_BYTE && mrna[pointer + 1] == 'U') {
        if (mrna[pointer + 2] != SHM_END_BYTE && mrna[pointer + 2] == 'G') {
          // Start!!!
          pointer++;
          pointer++;
          start = pointer + 1;
          running = false;
        }
      }
    }

    pointer++;
  }

  int aminosCount = 0;
  uint8_t *aminos;

  running = 1;
  while (running) {
    if (mrna[pointer] == SHM_END_BYTE || mrna[pointer + 1] == SHM_END_BYTE || mrna[pointer + 2] == SHM_END_BYTE) {
        resetSharedMemory();
        sharedMemory->data[0] = SHM_END_REACHED_BYTE;
        sharedMemory->data[1] = SHM_END_BYTE;
        return;
    }
    uint8_t f = mrna[pointer];
    uint8_t s = mrna[pointer + 1];
    uint8_t t = mrna[pointer + 2];

    uint8_t amino = getAminoAcid(f, s, t);
    if (amino == 0) {
        resetSharedMemory();
        sharedMemory->data[0] = SHM_ERROR_BYTE;
        sharedMemory->data[1] = SHM_END_BYTE;
        return;
    }

    if (amino == AMINO_STOP) {
      // We reached the end. Lol.
      end = pointer;
      running = 0;
    }

    // Amino is something. Save it.
    if (aminosCount == 0) {
      aminos = malloc(sizeof(uint8_t));
    } else {
      aminos = realloc(aminos, sizeof(uint8_t) * aminosCount + 1);
    }
    aminos[aminosCount] = amino;
    aminosCount++;

    pointer++;
    pointer++;
    pointer++;
  }

  // We are finished. Send the response.
  resetSharedMemory();
  sharedMemory->data[0] = SHM_SUCCESS_BYTE;
  sharedMemory->data[1] = start;
  sharedMemory->data[2] = end;
  // Always starts with AMINO_START
  sharedMemory->data[3] = AMINO_START;
  for (int i = 0; i < aminosCount; i++) {
    sharedMemory->data[i + 4] = aminos[i];
  }
  sharedMemory->data[aminosCount - 1 + 4] = SHM_END_BYTE;

  // Save new pointer
  mrnaPointers[clientId] = pointer;
}

static void reset(void) {
  uint8_t clientId = sharedMemory->data[1];
  if (clientId >= mrnaCount) {
    // Error
    resetSharedMemory();
    sharedMemory->data[0] = SHM_ERROR_BYTE;
    sharedMemory->data[1] = SHM_END_BYTE;
    return;
  }

  // Reset the pointer
  mrnaPointers[clientId] = 0;
}

static void quit(void) {
  uint8_t clientId = sharedMemory->data[1];
  if (clientId >= mrnaCount) {
    // Error
    resetSharedMemory();
    sharedMemory->data[0] = SHM_ERROR_BYTE;
    sharedMemory->data[1] = SHM_END_BYTE;
    return;
  }

  // Reset the pointer
  mrnaPointers[clientId] = 0;
  // Reset mrna
  uint8_t *mrna = mrnas[clientId];
  if (mrna != 0) {
    free(mrna);
  }
  mrnas[clientId] = 0;
}

static void resetSharedMemory(void) {
  // Debugging. Should be irrelevant because of our
  // SHM_END_BYTE constant.
  for (int i = 0; i < SHM_MAX_DATA; i++) {
    sharedMemory->data[i] = 0x00;
  }
}

static void usage(void) {
  fprintf(stderr, "Usage: %s\n", programName);
  exit(EXIT_FAILURE);
}

static void catchSignals(int signo) {
  wantsQuit = 1;
}
