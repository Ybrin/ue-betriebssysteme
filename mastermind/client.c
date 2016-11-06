/**
 * @file client.c
 * @author Koray Koska (1528624)
 * @date 2016-10-24
 */

 #include <errno.h>
 #include <limits.h>
 #include <netdb.h>
 #include <netinet/in.h>
 #include <signal.h>
 #include <stdarg.h>
 #include <stdint.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <sys/socket.h>
 #include <sys/types.h>
 #include <time.h>
 #include <unistd.h>

/* === Constants === */

#define SLOTS (5)
#define COLORS (8)

#define SHIFT_WIDTH (3)
#define PARITY_ERR_BIT (6)
#define GAME_LOST_ERR_BIT (7)

#define EXIT_PARITY_ERROR (2)
#define EXIT_GAME_LOST (3)
#define EXIT_MULTIPLE_ERRORS (4)

/* === Macros === */

#ifdef ENDEBUG
#define DEBUG(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define DEBUG(...)
#endif

/* Length of an array */
#define COUNT_OF(x) (sizeof(x)/sizeof(x[0]))

/* === Global Variables === */

/* Name of the program */
static const char *progname = "client"; /* default name */

/* File descriptor for server socket */
static int sockfd = -1;

/* File descriptor for connection socket */
static int connfd = -1;

/* This variable is set upon receipt of a signal */
volatile sig_atomic_t quit = 0;

/* count possible solutions */
static int possible_solutions = 0;

/* pointer to the first element of the search space list */
static struct try* first = NULL;

/* pointer to a list of address informations as returned by getaddrinfo */
static struct addrinfo *addr_list;

/* === Type Definitions === */

/* represents a solution with a linked list */
struct try {
  uint8_t colors[SLOTS];
  struct try* next;
};

enum { beige, darkblue, green, orange, red, black, violet, white };

/* === Prototypes === */

/**
 * @brief Parse command line options
 * @param argc The argument counter
 * @param argv The argument vector
 * @param options Struct where parsed arguments are stored
 */
static void parse_args(int argc, char **argv, struct addrinfo **addr_list);

/**
 * @brief terminate program on program error
 * @param exitcode exit code
 * @param fmt format string
 */
static void bail_out(int exitcode, const char *fmt, ...);

/**
 * @brief Signal handler
 * @param sig Signal number catched
 */
static void signal_handler(int sig);

/**
 * @brief free allocated resources
 */
static void free_resources(void);

/**
 * @brief Converts the given colors array into the requested 2 byte response
 * @param Color Array which should be converted
 * @return 2 byte response with colors and parity bit
 */
static int get_client_response(uint8_t *colors);

/**
 * @brief Sends the given color array encoded to the server
 * @param Color array which should be sent
 * @return 1 if successful, -1 if not
 */
static int send_message_to_server(uint8_t *colors);

/**
 * @brief Sets the global variable first to the first element of a linked list
          which contains all possible solutions
 * @param i Position of the array
 * @param tmp_solution Array where temporary solutions will be saved
 */
static void get_possible_solutions(int i, uint8_t *tmp_solution);

/**
 * @brief Gets next try of the possible solutions, that match the previous try (red and white count must match)
 * @param colors Array of colors for the previous try
 * @param server_red Red count of pins for the previous try
 * @param server_white White count of pins for the previous try
 */
static void get_next_try(uint8_t* colors, uint8_t server_red, uint8_t server_white);

/**
 * @brief compares secret with guess and returns the number of red and white pins
 * @param red Pointer for saving the count of red pins
 * @param white Pointer for saving the count of white pins
 * @param secret Secret Array with colors
 * @param guess Guess Array with colors
 * @return -1 if bytes couldn't be sent, 0 for success
 */
static void compare_solutions(uint8_t* red, uint8_t* white, uint8_t *secret, uint8_t *guess);

/* === Main program === */

int main(int argc, char **argv) {
  struct addrinfo *rp;
  uint8_t colors[] = {beige, beige, darkblue, red, green};
  uint8_t red = 0, white = 0;
  uint8_t tmp_solution[] = {0, 0, 0, 0, 0};
  uint8_t response;
  int rounds = 0;
  int ret = EXIT_SUCCESS;
  char error = 0;
  sigset_t blocked_signals;

  /* setup signal handlers */
  if(sigfillset(&blocked_signals) < 0) {
    bail_out(EXIT_FAILURE, "sigfillset");
  } else {
    const int signals[] = { SIGINT, SIGQUIT, SIGTERM };
    struct sigaction s;
    s.sa_handler = signal_handler;
    (void) memcpy(&s.sa_mask, &blocked_signals, sizeof(s.sa_mask));
    s.sa_flags   = SA_RESTART;
    for(int i = 0; i < 3; i++) {
      if (sigaction(signals[i], &s, NULL) < 0) {
        bail_out(EXIT_FAILURE, "sigaction");
      }
    }
  }

  parse_args(argc, argv, &addr_list);

  for(rp = addr_list; rp != NULL && connfd == -1 ; rp = rp->ai_next){
    sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if(sockfd != -1){
      connfd = connect(sockfd, rp->ai_addr, rp->ai_addrlen);
    }
  }

  if (sockfd == -1) {
    bail_out(EXIT_FAILURE, "Can't create TCP/IP socket...");
  }
  if (connfd == -1) {
    bail_out(EXIT_FAILURE, "Can't connect to server...");
  }

  get_possible_solutions(0, &tmp_solution[0]);

  while (error == 0) {
    rounds += 1;
    if (send_message_to_server(&colors[0]) == -1) {
      bail_out(EXIT_FAILURE, "couldn't send message to server\n");
    }

    if (recv(sockfd, &response, sizeof(response), 0) < 0) {
      bail_out(EXIT_FAILURE, "Couldn't read response from server");
    }

    DEBUG("read 0x%x from client, error code: 0x%x \n", response, (response & (0xC0)) >> 6);

    /* We sent the answer to the client; now stop the game
       if its over, or an error occured */
    if (response & (1 << PARITY_ERR_BIT)) {
      (void) fprintf(stderr, "Parity error\n");
      error = 1;
      ret = EXIT_PARITY_ERROR;
    }
    if (response & (1 << GAME_LOST_ERR_BIT)) {
      (void) fprintf(stderr, "Game lost\n");
      error = 1;
      if (ret == EXIT_PARITY_ERROR) {
          ret = EXIT_MULTIPLE_ERRORS;
      } else {
          ret = EXIT_GAME_LOST;
      }
    }

    red = response & 0x07;
    white = (response >> 3) & 0x07;
    if (red == SLOTS) {
      break;
    }

    get_next_try(&colors[0], red, white);
  }
  if (!error) {
    fprintf(stdout, "%d\n", rounds);
  }
  free_resources();
  return ret;
}


/* === Implementations === */

static void parse_args(int argc, char **argv, struct addrinfo **addr_list) {
  char *hostname_arg;
  char *port_arg;
  struct addrinfo hints;

  if (argc > 0) {
    progname = argv[0];
  }
  if (argc < 3) {
    bail_out(EXIT_FAILURE, "Usage: %s <server-hostname> <server-port>", progname);
  }

  hostname_arg = argv[1];
  port_arg = argv[2];

  (void) memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if(getaddrinfo(hostname_arg, port_arg, &hints, addr_list) != 0){
    bail_out(EXIT_FAILURE, "Can't get addrinfo for hostname: %s", hostname_arg);
  }
}

static void bail_out(int exitcode, const char *fmt, ...) {
  va_list ap;

  (void) fprintf(stderr, "%s: ", progname);
  if (fmt != NULL) {
      va_start(ap, fmt);
      (void) vfprintf(stderr, fmt, ap);
      va_end(ap);
  }
  if (errno != 0) {
      (void) fprintf(stderr, ": %s", strerror(errno));
  }
  (void) fprintf(stderr, "\n");

  free_resources();
  exit(exitcode);
}

static void signal_handler(int sig) {
  DEBUG("Caught Signal\n");
  free_resources();
  exit(EXIT_SUCCESS);
}

static void free_resources(void) {
  DEBUG("Shutting down server\n");
  if (connfd >= 0) {
      (void) close(connfd);
  }
  if (sockfd >= 0) {
      (void) close(sockfd);
  }

  // Free allocated try linked list
  struct try *delete;
  while(first != NULL){
    delete = first;
    first = first->next;
    free(delete);
  }

  // Free the global addrinfo
  freeaddrinfo(addr_list);
}

static int get_client_response(uint8_t *colors) {
  uint16_t response = 0;
  uint16_t current_color = 0;
  uint16_t parity_calc = 0;
  uint16_t parity_tmp = 0;

  for (int i = 0 ; i < SLOTS; i++) {
    // Set current color to the ith value of the array
    current_color = colors[i];
    // Set the parity operation we will do to current color
    parity_tmp = current_color;
    // Left shift current color by i * 3 for the bitwise or operation afterwords
    current_color = current_color << (i * 3);
    // Do a bitwise or operation with response and current color to get current value to the left
    response = response | current_color;
    // Do the parity calculation for the current values. The right most bit (the first bit) will be our current parity bit
    parity_calc ^= parity_tmp ^ (parity_tmp >> 1) ^ (parity_tmp >> 2);
  }

  // Set the response to the colors (response) together with the parity bit which is the right most element (first bit) of parity_calc
  response = response | ((parity_calc << 15));
  return response;
}

static int send_message_to_server(uint8_t *colors) {
  uint16_t client_response = 0xF000;
  // Create an array with two one-byte sequences
  uint8_t bytes[2];
  // Get the client response for the color array
  client_response = get_client_response(colors);
  DEBUG("Send message 0x%x\n", client_response);

  // bytes[0] will be the second 8 bits (second byte) of the client response
  bytes[0] = (client_response & 0x00FF);
  // bytes[1] will be the first 8 bits (first byte) of the client response (right shifting by 8 so the correct bits fit into the uint8_t)
  bytes[1] = (client_response & 0xFF00) >> 8;
  for (int i = 0; i < 2; i++) {
    // Send the first byte first and then the second one
    if(send(sockfd, &bytes[i], sizeof(bytes[i]), 0) < 0){
      return -1;
    }
  }
  return 1;
}

static void get_possible_solutions(int i, uint8_t *tmp_solution) {
  struct try* new;
  int j;

  if (i == SLOTS) {
    new = malloc(sizeof(struct try));
    if (new == NULL) {
      bail_out(EXIT_FAILURE, "Memory allocation failed...");
    }
    new->next = NULL;
    for (j = 0; j < SLOTS; j++) {
      new->colors[j] = tmp_solution[j];
    }
    new->next = first;
    first = new;
    possible_solutions += 1;
  } else {
    for (j = 0; j < COLORS; j++) {
      tmp_solution[i] = j;
      get_possible_solutions(i+1, &tmp_solution[0]);
    }
  }
}

static void get_next_try(uint8_t* colors, uint8_t server_red, uint8_t server_white) {
  struct try *current = first;
  struct try *prev = NULL;
  struct try *delete;
  uint8_t white = 0;
  uint8_t red = 0;
  int i = 0;
  int steps;


   while (current != NULL) {
    compare_solutions(&red, &white, &(current->colors[0]), &colors[0]);
    if (red == server_red && white == server_white) {
      prev = current;
    } else {
      delete = current;
      if(prev == NULL){
        first = current->next;
      } else {
        prev->next = current->next;
      }
      free(delete);
      possible_solutions -= 1;
    }
    current = current->next;

  }

  steps = (rand() / RAND_MAX) * possible_solutions;

  current = first;
  for(i = 0; i <= steps && current->next != NULL; i++){
    current = current->next;
  }
  (void) memcpy(colors, &current->colors[0], sizeof(colors));
}

static void compare_solutions(uint8_t* red, uint8_t* white, uint8_t * secret, uint8_t * guess) {
  int colors_left[COLORS];
  int j;
  /* marking red and white */
  (void) memset(&colors_left[0], 0, sizeof(colors_left));
  *(red) = 0;
  *(white) = 0;
  for (j = 0; j < SLOTS; ++j) {
    /* mark red */
    if (guess[j] == secret[j]) {
      *(red) += 1;
    } else {
      colors_left[secret[j]]++;
    }
  }
  for (j = 0; j < SLOTS; ++j) {
    /* not marked red */
    if (guess[j] != secret[j]) {
      if (colors_left[guess[j]] > 0) {
        *(white) += 1;
        colors_left[guess[j]]--;
      }
    }
  }
}
