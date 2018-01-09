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

// Signals
#include <signal.h>


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

/// Signal handling
static volatile sig_atomic_t wantsQuit = 0;

/// Current client id for server
static uint8_t clientId = 0;

/// Current mrna count
static int currentMrnaCount = 0;

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
 * Cleanup resources
 */
static void cleanup(void);

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
  // printf("The number %u was read!\n", sharedMemory->data[1]);
  // sharedMemory->data[0] = 0xF;

  /*
  // ---- Submit
  printf("Waiting for 1\n");
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

  printf("Posting 2\n");
  // Nice
  sem_post(sem2);

  printf("Waiting for 4\n");
  sem_wait(sem4);

  // Read
  printf("%u\n", sharedMemory->data[0]);
  printf("%u\n", sharedMemory->data[1]);

  // VERY IMPORTANT FOR ALL FOLLOWING REQUESTS
  uint8_t clientId = sharedMemory->data[1];

  printf("Posting 3\n");
  sem_post(sem3);

  // ---- Done

  printf("Done\n");

  // ---- Next Sequence

  sem_wait(sem1);
  sharedMemory->data[0] = 'n';
  sharedMemory->data[1] = clientId;
  sharedMemory->data[2] = SHM_END_BYTE;

  sem_post(sem2);

  sem_wait(sem4);

  printf("%u\n", sharedMemory->data[0]);
  printf("%u\n", sharedMemory->data[1]);
  printf("%u\n", sharedMemory->data[2]);
  int i = 3;
  while (true) {
    uint8_t d = sharedMemory->data[i];
    if (d == SHM_END_BYTE) {
      break;
    }
    printf("%c", d);
    i++;
  }
  printf("\n");
  sem_post(sem3);

  // ---- Done
  */

  // Process user input

  printf("Available commands:\n");
  printf("%s\n", "  s - submit a new mRNA sequence");
  printf("%s\n", "  n - show next protein sequence in active mRNA sequence");
  printf("%s\n", "  r - reset active mRNA sequence");
  printf("%s\n", "  q - close this client");

  while (!wantsQuit) {
    printf("%s", "Enter new command. (Enter to end input): ");
    char c = getchar();
    // Remove newline from stdin
    getchar();
    // printf("\n");

    switch (c) {
      case 's':
        submit();
        break;
      case 'n':
        nextSequence();
        break;
      case 'r':
        reset();
        break;
      case 'q':
        quit();
        printf("%s\n", "Close client.");
        wantsQuit = 1;
        break;
      default:
        quit();
        printf("%s\n", "This is not a command. Bye.");
        wantsQuit = 1;
        break;
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
  // printf("Cleaning up\n");
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

static void submit(void) {
  // Get user input
  printf("%s\n", "Enter new mRNA sequence. (Newline to end input):");

  int running = 1;

  int mrnaCount = 0;
  char *mrna;

  int newLineCount = 0;

  while (running) {
    char c = getchar();
    if (c == '\n') {
      newLineCount++;
      if (newLineCount >= 2) {
        running = 0;
        continue;
      }
    } else {
      newLineCount = 0;
    }

    if (c == 'U' || c == 'C' || c == 'A' || c == 'G') {
      mrnaCount++;
      if (mrnaCount == 0) {
        mrna = malloc(sizeof(char));
      } else {
        mrna = realloc(mrna, sizeof(char) * mrnaCount);
      }
      mrna[mrnaCount - 1] = c;
    }

    // We collect only valid characters.
    // After two newlines we set running to 0.
  }

  // Hard check for now
  if (mrnaCount >= 250) {
    printf("%s\n", "Please don't type in more that 249 characters.");
    return;
  }
  currentMrnaCount = mrnaCount;

  // Submit mrna to server

  if (sem_wait(sem1) != 0) {
    fprintf(stderr, "sem_wait failed\n");
    exit(EXIT_FAILURE);
  }
  sharedMemory->data[0] = 's';
  for (int i = 0; i < mrnaCount; i++) {
    sharedMemory->data[i + 1] = mrna[i];
  }
  sharedMemory->data[mrnaCount] = SHM_END_BYTE;

  // Nice, We are finished requesting. Ask server to answer.
  if (sem_post(sem2) != 0) {
    fprintf(stderr, "sem_wait failed\n");
    exit(EXIT_FAILURE);
  }

  // Wait for server response.
  if (sem_wait(sem4) != 0) {
    fprintf(stderr, "sem_wait failed\n");
    exit(EXIT_FAILURE);
  }

  // Read
  // printf("%u\n", sharedMemory->data[0]);
  // printf("%u\n", sharedMemory->data[1]);

  int resp = sharedMemory->data[0];

  // VERY IMPORTANT FOR ALL FOLLOWING REQUESTS
  clientId = sharedMemory->data[1];

  // Tell server we are finished reading.
  if (sem_post(sem3) != 0) {
    fprintf(stderr, "sem_wait failed\n");
    exit(EXIT_FAILURE);
  }

  if (resp == SHM_ERROR_BYTE) {
    fprintf(stderr, "%s\n", "Something went wrong while requesting from the server.");
    exit(EXIT_FAILURE);
  }
}

static void nextSequence(void) {
  // Wait until we can start the request.
  if (sem_wait(sem1) != 0) {
    fprintf(stderr, "sem_wait failed\n");
    exit(EXIT_FAILURE);
  }
  sharedMemory->data[0] = 'n';
  sharedMemory->data[1] = clientId;
  sharedMemory->data[2] = SHM_END_BYTE;

  // Tell the server we are finished requesting.
  if (sem_post(sem2) != 0) {
    fprintf(stderr, "sem_wait failed\n");
    exit(EXIT_FAILURE);
  }

  // Wait until we can read the response.
  if (sem_wait(sem4) != 0) {
    fprintf(stderr, "sem_wait failed\n");
    exit(EXIT_FAILURE);
  }

  int resp = sharedMemory->data[0];
  if (resp == SHM_ERROR_BYTE) {
    // We need to tell the server that we are finished reading
    // or other clients will be stuck.
    sem_post(sem3);
    fprintf(stderr, "%s\n", "Something went wrong while requesting from the server.");
    exit(EXIT_FAILURE);
  }

  if (resp == SHM_END_REACHED_BYTE) {
    printf("End reached [%i/%i], send r to reset.\n", currentMrnaCount, currentMrnaCount);
  } else {
    uint8_t start = sharedMemory->data[1];
    uint8_t end = sharedMemory->data[2];
    printf("Protein sequence found [%i/%i] to [%i/%i]: ", start, currentMrnaCount, end, currentMrnaCount);

    int i = 3;
    while (true) {
      if (i >= SHM_MAX_DATA) {
        fprintf(stderr, "%s\n", "The server seems malicious.");
        exit(EXIT_FAILURE);
      }
      uint8_t d = sharedMemory->data[i];
      if (d == SHM_END_BYTE) {
        break;
      }
      printf("%c", d);
      i++;
    }
    printf("\n");
  }

  // printf("%u\n", sharedMemory->data[0]);
  // printf("%u\n", sharedMemory->data[1]);
  // printf("%u\n", sharedMemory->data[2]);

  // We are done reading. Tell the server about it.
  if (sem_post(sem3) != 0) {
    fprintf(stderr, "sem_wait failed\n");
    exit(EXIT_FAILURE);
  }
}

static void reset(void) {
  // Wait until we can start the request.
  if (sem_wait(sem1) != 0) {
    fprintf(stderr, "sem_wait failed\n");
    exit(EXIT_FAILURE);
  }
  sharedMemory->data[0] = 'r';
  sharedMemory->data[1] = clientId;
  sharedMemory->data[2] = SHM_END_BYTE;

  // Tell the server we are finished requesting.
  if (sem_post(sem2) != 0) {
    fprintf(stderr, "sem_wait failed\n");
    exit(EXIT_FAILURE);
  }

  // Wait until we can read the response.
  if (sem_wait(sem4) != 0) {
    fprintf(stderr, "sem_wait failed\n");
    exit(EXIT_FAILURE);
  }

  int resp = sharedMemory->data[0];
  if (resp == SHM_ERROR_BYTE) {
    // We need to tell the server that we are finished reading
    // or other clients will be stuck.
    sem_post(sem3);
    fprintf(stderr, "%s\n", "Something went wrong while requesting from the server.");
    exit(EXIT_FAILURE);
  }

  printf("Reset. [0/%i]\n", currentMrnaCount);

  // We are done reading. Tell the server about it.
  if (sem_post(sem3) != 0) {
    fprintf(stderr, "sem_wait failed\n");
    exit(EXIT_FAILURE);
  }
}

static void quit(void) {
  // Wait until we can start the request.
  if (sem_wait(sem1) != 0) {
    fprintf(stderr, "sem_wait failed\n");
    exit(EXIT_FAILURE);
  }
  sharedMemory->data[0] = 'q';
  sharedMemory->data[1] = clientId;
  sharedMemory->data[2] = SHM_END_BYTE;

  // Tell the server we are finished requesting.
  if (sem_post(sem2) != 0) {
    fprintf(stderr, "sem_wait failed\n");
    exit(EXIT_FAILURE);
  }

  // Wait until we can read the response.
  if (sem_wait(sem4) != 0) {
    fprintf(stderr, "sem_wait failed\n");
    exit(EXIT_FAILURE);
  }

  int resp = sharedMemory->data[0];
  if (resp == SHM_ERROR_BYTE) {
    // We need to tell the server that we are finished reading
    // or other clients will be stuck.
    sem_post(sem3);
    fprintf(stderr, "%s\n", "Something went wrong while requesting from the server.");
    exit(EXIT_FAILURE);
  }

  // We are done reading. Tell the server about it.
  if (sem_post(sem3) != 0) {
    fprintf(stderr, "sem_wait failed\n");
    exit(EXIT_FAILURE);
  }
}

static void usage(void) {
  fprintf(stderr, "Usage: %s\n", programName);
  exit(EXIT_FAILURE);
}

static void catchSignals(int signo) {
  wantsQuit = 1;
}
