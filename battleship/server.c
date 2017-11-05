/**
 * @file server.c
 * @author Koray Koska <e1528624@student.tuwien.ac.at>
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
#include <math.h>

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

#define SQUARE_SHIP 3
#define SQUARE_NOTHING 4
#define SQUARE_SHIP_HIT 5

static inline void print_map_server(uint8_t map[MAP_SIZE][MAP_SIZE])
{
    int x, y;

    printf("  ");
    for (x = 0; x < MAP_SIZE; x++)
        printf("%c ", 'A' + x);
    printf("\n");

    for (y = 0; y < MAP_SIZE; y++) {
        printf("%c ", '0' + y);
        for (x = 0; x < MAP_SIZE; x++)
            printf("%c ", map[x][y] ? ((map[x][y] == SQUARE_SHIP) ? 's' : (map[x][y] == SQUARE_NOTHING) ? ' ' : 'h') : ' ');
        printf("\n");
    }
}

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

  int sunk;
} Ship;

/**
 * An array of ships
 */
Ship **ships = NULL;

/**
 * The game map.
 */
uint8_t map[MAP_SIZE][MAP_SIZE];

/**
 * The current round.
 */
int currentRound = 0;

/**
 * Prints the usage and exits.
 */
static void usage(void);

/**
 * Parses command line arguments
 */
static void parseArgs(int argc, char *argv[]);

/**
 * Setup the map with ships.
 */
static void setupMap(void);

/**
 * Parses a one byte request.
 */
static int parseRequest(char req);

/**
 * Sunk?
 */
static int sunk(uint8_t horizontal, uint8_t vertical);

/**
 * Checks whether the game is won or not.
 */
static int checkWon(void);

int main(int argc, char *argv[]) {
  /* TODO
   * Add code to parse the command line arguments, maybe as a separate
   * function.
   */

  // Parse command line arguments
  parseArgs(argc, argv);

  // Setup map
  setupMap();

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  int res = getaddrinfo(NULL, port, &hints, &ai);
  /* TODO
   * check for errors
   */
  if (res != 0) {
    (void) fprintf(stderr, "Something bad happened with getaddinfo().\n");
    exit(EXIT_FAILURE);
  }

  sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
  /* TODO
   * check for errors
   */
  if (sockfd == -1) {
    (void) fprintf(stderr, "Something bad happened while getting a socket().\n");
    exit(EXIT_FAILURE);
  }

  int val = 1;
  res = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof val);
  /* TODO
   * check for errors
   */
  if (res != 0) {
    (void) fprintf(stderr, "Something bad happened while setting socket options setsockopt().\n");
    exit(EXIT_FAILURE);
  }

  res = bind(sockfd, ai->ai_addr, ai->ai_addrlen);
  /* TODO
   * check for errors
   */
  if (res != 0) {
    (void) fprintf(stderr, "Something bad happened while binding the socket bind().\n");
    exit(EXIT_FAILURE);
  }

  printf("Listening on port %s...\n", port);

  res = listen(sockfd, 1);
  /* TODO
   * check for errors
   */
  if (res != 0) {
    (void) fprintf(stderr, "Something bad happened while listening for connections listen().\n");
    exit(EXIT_FAILURE);
  }

  connfd = accept(sockfd, NULL, NULL);
  /* TODO
   * check for errors
   */
  if (connfd == -1) {
    (void) fprintf(stderr, "Something bad happened while accepting a connection accept().\n");
    exit(EXIT_FAILURE);
  }

  /* TODO
   * Here you might want to add variables to keep track of the game status,
   * for instance the number of rounds that have been played.
   */

  int won = 0;

  printf("Starting main loop...\n");
  fflush(stdout);

  while (!won) {
      /* TODO
       * add code to:
       *  - wait for a request from the client
       *  - check for a parity error or invalid coordinates in the request
       *  - check whether a ship was hit and determine the status to return
       *  - send an according response to the client
       */
    char *buf = malloc(sizeof(char));

    int bytes = read(connfd, buf, 1);
    if (bytes == 0) {
      (void) fprintf(stderr, "read reached EOF.\n");
      exit(EXIT_FAILURE);
    }

    won = parseRequest(buf[0]);

    free(buf);

    currentRound++;
  }

  /* TODO
   * cleanup
   */
   if (won == 1) {
     // Won!
     printf("Won in %i rounds!\n", currentRound);
   } else if (won == 2) {
     // Lost :/
     printf("Game lost\n");
   }

  close(connfd);
  close(sockfd);

  return EXIT_SUCCESS;
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
      if (altPort <= 0 || altPort > pow(2, 16)) {
        (void) fprintf(stderr, "Port must be a valid port\n");
        exit(EXIT_FAILURE);
      }
    } else {
      usage();
    }
  }
  if (altPort != 0) {
    int length = snprintf(NULL, 0, "%ld", altPort);
    char* str = malloc(length + 1);
    snprintf(str, length + 1, "%ld", altPort);
    port = str;
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

/**
 * Setup the map.
 */
static void setupMap(void) {
  memset(&map, SQUARE_NOTHING, sizeof(map));

  int shipCount = SHIP_CNT_LEN2 + SHIP_CNT_LEN3 + SHIP_CNT_LEN4;
  for (int i = 0; i < shipCount; i++) {
    Ship *s = ships[i];

    if (s->startVertical == s->endVertical) {
      uint8_t hStart = (s->startHorizontal < s->endHorizontal) ? s->startHorizontal : s->endHorizontal;
      uint8_t hEnd = (s->startHorizontal > s->endHorizontal) ? s->startHorizontal : s->endHorizontal;

      for (uint8_t j = hStart; j <= hEnd; j++) {
        map[s->startVertical][j] = SQUARE_SHIP;
      }
    } else if (s->startHorizontal == s->endHorizontal) {
      uint8_t vStart = (s->startVertical < s->endVertical) ? s->startVertical : s->endVertical;
      uint8_t vEnd = (s->startVertical > s->endVertical) ? s->startVertical : s->endVertical;

      for (uint8_t j = vStart; j <= vEnd; j++) {
        map[j][s->startHorizontal] = SQUARE_SHIP;
      }
    }
  }

  print_map_server(map);
  fflush(stdout);
}

/**
 * Parses a one byte request.
 *
 * Returns 0 if not finished yet, 1 if the game was won,
 * and 2 if the game was finished but not won.
 */
static int parseRequest(char req) {
  char parityMask = 0x01;
  char parityBit = (req >> 7) & parityMask;

  char cMask = 0x01;
  char check = (req & cMask) ^
    ((req >> 1) & cMask) ^
    ((req >> 2) & cMask) ^
    ((req >> 3) & cMask) ^
    ((req >> 4) & cMask) ^
    ((req >> 5) & cMask) ^
    ((req >> 6) & cMask);

  if (check != parityBit) {
    // Parity error 0000 1000
    // e.g.: Status == 3
    char pE = 0x08;
    write(connfd, &pE, 1);

    printf("REQ: ");
    printCharBitwise(req);
    (void) fprintf(stderr, "Parity error\n");
    exit(2);
  }

  // Oll Korrekt (O.K.). Parsing coordinates...

  // Remove first bit (parity)
  char coordinates = req & 0x7F;

  char horizontal = coordinates / 10;
  char vertical = coordinates % 10;

  if (horizontal < 0 || horizontal > MAP_SIZE - 1 || vertical < 0 || vertical > MAP_SIZE - 1) {
    // Wrong coordinates
    char res = 0x0C;
    write(connfd, &res, 1);

    (void) fprintf(stderr, "Invalid coordinate\n");
    exit(3);
  }

  int hit = 0;
  if (map[(uint8_t) horizontal][(uint8_t) vertical] == SQUARE_SHIP) {
    map[(uint8_t) horizontal][(uint8_t) vertical] = SQUARE_SHIP_HIT;
    hit = 1;
  }

  print_map_server(map);

  if (checkWon()) {
    // 0000 0100
    char res = 0x04;
    if (hit) {
      // Set hit, 3, 0000 0011
      res = res | 0x03;
    } else {
      // Will actually never happen...
      res = res | 0x00;
    }

    write(connfd, &res, 1);
    return 1;
  } else {
    char res = 0x00;
    if (currentRound >= MAX_ROUNDS - 1) {
      // Status == 1 ==> 0000 0100
      res = 0x04;
    } else {
      // Status == 0 ==> 0000 0000
      res = 0x00;
    }
    // Not won yet
    if (hit) {
      // TODO: Check everything...
      if (sunk(horizontal, vertical)) {
        printf("SHIP SUNK!\n");
        res = res | 0x02;
      } else {
        printf("SHIP NOT SUNK!\n");
        res = res | 0x01;
      }
    } else {
      // No hit, hit == 0 ==> ???? ??00
      res = res & 0xFC;
    }

    write(connfd, &res, 1);

    if (currentRound >= MAX_ROUNDS - 1) {
      // Game finished and lost :/
      return 2;
    } else {
      // Game goes on...
      return 0;
    }
  }

  return 0;
}

/**
 * Checks whether the game is won or not.
 *
 * Returns 0 if the game was not won yet and 1 if it was.
 */
static int checkWon(void) {
  for (int i = 0; i < MAP_SIZE; i++) {
    for (int j = 0; j < MAP_SIZE; j++) {
      if (map[j][i] == SQUARE_SHIP) {
        return 0;
      }
    }
  }

  // Won
  return 1;
}

/**
 * Sunk?
 */
static int sunk(uint8_t horizontal, uint8_t vertical) {
  if (horizontal > 0) {
    for (int i = horizontal; i >= 0; i--) {
      if (map[i][vertical] == SQUARE_SHIP) {
        return 0;
      } else if (map[i][vertical] == SQUARE_NOTHING) {
        break;
      }
    }
  }

  if (horizontal < MAP_SIZE - 1) {
    for (int i = horizontal; i < MAP_SIZE; i++) {
      if (map[i][vertical] == SQUARE_SHIP) {
        return 0;
      } else if (map[i][vertical] == SQUARE_NOTHING) {
        break;
      }
    }
  }

  if (vertical > 0) {
    for (int i = vertical; i >= 0; i--) {
      if (map[horizontal][i] == SQUARE_SHIP) {
        return 0;
      } else if (map[horizontal][i] == SQUARE_NOTHING) {
        break;
      }
    }
  }

  if (vertical < MAP_SIZE - 1) {
    for (int i = vertical; i < MAP_SIZE; i++) {
      if (map[horizontal][i] == SQUARE_SHIP) {
        return 0;
      } else if (map[horizontal][i] == SQUARE_NOTHING) {
        break;
      }
    }
  }

  return 1;
}
