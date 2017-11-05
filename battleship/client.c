/**
 * @file client.c
 * @author Koray Koska <e1528624@student.tuwien.ac.at>
 * @date 2017-10-06
 *
 * @brief Client for OSUE exercise 1B `Battleship'.
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
#include <math.h>
#include <arpa/inet.h>

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

// Static variables for resources that should be freed before exiting:
static int sockfd = -1;                 // socket file descriptor

/// Name of this program
static const char *programName = "client";

// Current coordinates which are checked
static uint8_t currentHorizontal = 0;
static uint8_t currentVertical = 0;

/**
 * The game map.
 */
uint8_t map[MAP_SIZE][MAP_SIZE];

/**
 * Setup the map with ships.
 */
static void setupMap(void);

/**
 * Send a request and parse the response.
 */
static int sendRequest(void);

/**
 * Prints the usage and exits.
 */
static void usage(void);

/**
 * Parses the response from the server.
 */
static int parseResponse(char res);

int main(int argc, char *argv[]) {

  long altPort = 0;
  char *altHost = NULL;

  int getopt_result;
  while ((getopt_result = getopt(argc, argv, "h:p:")) != -1) {
    switch (getopt_result) {
      case 'h':
        altHost = optarg;
        break;
      case 'p':
        altPort = strtol(optarg, NULL, 10);
        if (altPort <= 0 || altPort > pow(2, 16)) {
          (void) fprintf(stderr, "Port must be a valid port\n");
          usage();
        }
        break;
      default:
        assert(0);
    }
  }

  struct sockaddr_in addr;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(altPort > 0 ? altPort : strtol(DEFAULT_PORT, NULL, 10));
  inet_aton(altHost != NULL ? altHost : DEFAULT_HOST, &addr.sin_addr);
  memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

  sockfd = socket(AF_INET, SOCK_STREAM, 6);
  if(sockfd < 0) {
    (void) fprintf(stderr, "socket() failed\nerrno: %i\n Stopping\n", errno);
    return EXIT_FAILURE;
  }
  int res = connect(sockfd, (struct sockaddr*) &addr, sizeof(addr));
  if (res == -1) {
    (void) fprintf(stderr, "connect() failed\nerrno: %i\n Stopping\n", errno);
    return EXIT_FAILURE;
  }

  setupMap();

  // Main loop
  int won = 0;
  int rounds = 0;
  while (!won) {
    sendRequest();

    // Read response
    char *buf = malloc(sizeof(char));
    int bytes = read(sockfd, buf, 1);
    if (bytes == 0) {
      (void) fprintf(stderr, "read reached EOF.\n");
      exit(EXIT_FAILURE);
    }

    won = parseResponse(buf[0]);

    free(buf);

    rounds++;
  }

  if (won == 1) {
    // Won!
    printf("I won :)\n");
  } else if (won == 2) {
    // Lost :/
    printf("Game lost\n");
  }

  close(sockfd);
  return EXIT_SUCCESS;
}

/**
 * Setup the map.
 */
static void setupMap(void) {
  memset(&map, SQUARE_UNKNOWN, sizeof(map));
  print_map(map);
  fflush(stdout);
}

/**
 * Send the next coordinate request.
 *
 * Returns -1 on error and a value >0 on success.
 */
static int sendRequest(void) {
  for (int i = 0; i < MAP_SIZE; i++) {
    for (int j = 0; j < MAP_SIZE; j++) {
      // map[j][i]
      // j: ------------
      // i: |
      if (map[j][i] == SQUARE_UNKNOWN) {
        // Set current coordinates
        currentHorizontal = j;
        currentVertical = i;

        char coord = i + j * 10;
        // First bit is the parity bit. Reset it to 0 for now...
        coord = coord & 0x7F;

        char pMask = 0x01;
        char parity = (coord & pMask) ^
          ((coord >> 1) & pMask) ^
          ((coord >> 2) & pMask) ^
          ((coord >> 3) & pMask) ^
          ((coord >> 4) & pMask) ^
          ((coord >> 5) & pMask) ^
          ((coord >> 6) & pMask);
        // Set parity to the first bit, set all other bits to 0.
        parity = (parity << 7) & 0x80;

        // Add parity bit to the request...
        char req = parity | coord;

        /*
        printf("COORD: ");
        printCharBitwise(coord);
        printf("PARITY: ");
        printCharBitwise(parity);
        printf("WRITING: ");
        printCharBitwise(req);*/

        // Send request to the server
        return write(sockfd, &req, 1);
      }
    }
  }

  return -1;
}

/**
 * Parses a response from the server
 *
 * Returns 0 if not finished yet, 1 if the game was won,
 * 2 if the game is finished but not won.
 */
static int parseResponse(char res) {
  // Get bit 0 and 1 (hit)
  printf("WTF: ");
  printCharBitwise(res);
  char hit = res & 0x03;
  // Get bit 2 and 3 (status)
  char status = (res >> 2) & 0x03;

  if (status == 1) {
    if (hit == 3) {
      return 1;
    } else {
      return 2;
    }
  } else if (status == 2) {
    (void) fprintf(stderr, "Parity error\n");
    exit(2);
  } else if (status == 3) {
    // printf("RES: ");
    // printCharBitwise(res);
    (void) fprintf(stderr, "Invalid coordinate\n");
    exit(3);
  } else if (status == 0) {
    // Game running. Save results...
    if (hit == 0) {
      map[currentHorizontal][currentVertical] = SQUARE_EMPTY;
    } else if (hit == 1 || hit == 2) {
      map[currentHorizontal][currentVertical] = SQUARE_HIT;
    }

    print_map(map);
    fflush(stdout);

    return 0;
  }

  return 0;
}

/**
 * Prints the usage and exits.
 */
static void usage(void) {
  (void) fprintf(stderr, "Usage: %s [-h HOST] [-p PORT]\n",
                 programName);
  exit(EXIT_FAILURE);
}
