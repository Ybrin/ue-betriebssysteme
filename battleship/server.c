/**
 * @file server.c
 * @author OSUE Team <osue-team@cps.tuwien.ac.at>
 * @date 2017-10-06
 *
 * @brief Server for OSUE exercise 1B `Battleship'.
 */

// IO, C standard library, POSIX API, data types:
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#include <assert.h>
#include <ctype.h>

// Assertions, errors, signals:
#include <assert.h>
#include <errno.h>
#include <signal.h>

// Time:
#include <time.h>

// Sockets, TCP, ... :
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

// stuff shared by client and server:
#include "common.h"

// Static variables for things you might want to access from several functions:
static const char *port = DEFAULT_PORT; // the port to bind to

// Static variables for resources that should be freed before exiting:
static struct addrinfo *ai = NULL;      // addrinfo struct
static int sockfd = -1;                 // socket file descriptor
static int connfd = -1;                 // connection file descriptor

/// Name of this program
static const char *programName = "server";

/* TODO
 * You might want to add further static variables here, for instance to save
 * the programe name (argv[0]) since you should include it in all messages.
 *
 * You should also have some kind of a list which saves information about your
 * ships. For this purpose you might want to define a struct. Bear in mind that
 * your server must keep record about which ships have been hit (and also where
 * they have been hit) in order to know when a ship was sunk.
 *
 * You might also find it convenient to add a few functions, for instance:
 *  - a function which cleans up all resources and exits the program
 *  - a function which parses the arguments from the command line
 *  - a function which adds a new ship to your list of ships
 *  - a function which checks whether a client's shot hit any of your ships
 */

/**
 * A ship with start and end coordinates
 */
typedef struct {
  uint8_t startVertical;
  uint8_t startHorizontal;
  uint8_t endVertical;
  uint8_t endHorizontal;
} Ship;

/**
 * An array of ships
 */
Ship **ships = NULL;

/**
 * Prints the usage and exits.
 */
static void usage(void);

/**
 * Parses command line arguments
 */
static void parseArgs(int argc, char *argv[]);

int main(int argc, char *argv[]) {
  /* TODO
   * Add code to parse the command line arguments, maybe as a separate
   * function.
   */

  parseArgs(argc, argv);

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  int res = getaddrinfo(NULL, port, &hints, &ai);
  /* TODO
   * check for errors
   */

  sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
  /* TODO
   * check for errors
   */

  int val = 1;
  res = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof val);
  /* TODO
   * check for errors
   */

  res = bind(sockfd, ai->ai_addr, ai->ai_addrlen);
  /* TODO
   * check for errors
   */

  res = listen(sockfd, 1);
  /* TODO
   * check for errors
   */

  connfd = accept(sockfd, NULL, NULL);
  /* TODO
   * check for errors
   */

  /* TODO
   * Here you might want to add variables to keep track of the game status,
   * for instance the number of rounds that have been played.
   */

  while (false) {
      /* TODO
       * add code to:
       *  - wait for a request from the client
       *  - check for a parity error or invalid coordinates in the request
       *  - check whether a ship was hit and determine the status to return
       *  - send an according response to the client
       */
  }

  /* TODO
   * cleanup
   */
}

/**
 * Prints the usage of this program and terminates with EXIT_FAILURE
 */
static void usage(void) {
  (void) fprintf(stderr, "Usage: %s [-p PORT] SHIP1...\n",
                 programName);
  exit(EXIT_FAILURE);
}

static void parseArgs(int argc, char *argv[]) {
  if (argc > 0) {
    programName = argv[0];
  }

  int shipStart = 1;

  long altPort = 0;
  if (argc > 1 && strcmp("-p", argv[1]) == 0) {
    // Ships start at index 3 now
    shipStart = 3;

    if (argc > 2) {
      altPort = strtol(argv[2], NULL, 10);
      if (altPort == 0) {
        (void) fprintf(stderr, "Port must be a valid port\n");
        exit(EXIT_FAILURE);
      }
    } else {
      usage();
    }
  }

  int shipCount = SHIP_CNT_LEN2 + SHIP_CNT_LEN3 + SHIP_CNT_LEN4;
  int shipEnd = shipStart + (shipCount - 1) + 1;
  if (argc != shipEnd) {
    (void) fprintf(stderr, "Please specify exactly %i ships.\n", shipCount);
    exit(EXIT_FAILURE);
  }

  ships = malloc(shipCount * sizeof(Ship*));

  int currShip = 0;
  for (int i = shipStart; i < shipEnd; i++) {
    char *curr = argv[i];
    if (strlen(curr) != 4) {
      (void) fprintf(
          stderr,
          "Ship coordinates must be exactly 4 characters long (e.g.: C2E2)\n"
      );
      exit(EXIT_FAILURE);
    }

    uint8_t startVertical = vchartoi(curr[0]);
    uint8_t startHorizontal = hchartoi(curr[1]); 
    uint8_t endVertical = vchartoi(curr[2]);
    uint8_t endHorizontal = hchartoi(curr[3]); 

    if (startVertical == 255 || startHorizontal == 255 || endVertical == 255 || endHorizontal == 255) {
      (void) fprintf(stderr, "%s are not valid ship coordinates!", curr);
      exit(EXIT_FAILURE);
    }

    Ship *ship = malloc(sizeof(Ship));
    ship->startVertical = startVertical;
    ship->startHorizontal = startHorizontal;
    ship->endVertical = endVertical;
    ship->endHorizontal = endHorizontal;

    printf(
        "Ship coordinates: %i %i %i %i\n",
        ship->startVertical,
        ship->startHorizontal,
        ship->endVertical,
        ship->endHorizontal
    );
    fflush(stdout);

    ships[currShip] = ship;
    currShip++;
  }
}

