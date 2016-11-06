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

#define COLORS (8)
#define SHIFT_WIDTH (3)
#define SLOTS (5)
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


/* Name of the program */
static const char *progname = "client"; /* default name */

/* File descriptor for server socket */
static int sockfd = -1;

/* File descriptor for connection socket */
static int connfd = -1;

/* number of possible solutions in search space */
static int possible_solutions = 0;

/* pointer to the first element of the search space list */
static struct try* first = NULL;

/* pointer to a list of address informations as returned by getaddrinfo */
static struct addrinfo *addr_list;

/* This variable is set to ensure cleanup is performed only once */
volatile sig_atomic_t terminating = 0;


/* === Type Definitions === */

/* represents a possible solution in the search space linked list */
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
 * @param addr_list Pointer to the address pointer used to store the server address
 */
static void parse_args(int argc, char **argv, struct addrinfo **addr_list);

/**
 * @brief terminate program on program error
 * @param eval exit code
 * @param fmt format string
 */
static void bail_out(int eval, const char *fmt, ...);

/**
 * @brief converts a color array to 2 bytes, with parity
 * @param colors Array with colors
 * @return 2 bytes containing the color information and parity
 */
static int get_color_bytes(uint8_t *colors);

/**
 * @brief sends colors to server as two seperate bytes
 * @param colors Array with colors
 */
static int send_message(uint8_t *colors);

/**
 * @brief compares secret with guess and returns the number of red and white hits
 * @param red Pointer for saving the red count
 * @param white Pointer for saving the white count
 * @param secret Array with colors
 * @param guess Array with colors
 * @return -1 if bytes couldn't be sent, 0 for success
 */
static void compare_solutions(uint8_t* red, uint8_t* white, uint8_t * secret, uint8_t * guess);

/**
 * @brief creates a linked list with all possible solutions (the global variable first
 *        points to the beginning of the list)
 * @param i Current position of the array
 * @param tmp_solution Array used to store the temporary solutions during the recursions
 */

static void get_all_solutions(int i, uint8_t *tmp_solution);

/**
 * @brief gets the next try from the possible solutions, that match the previous
 *        submitted try (red and white count must match) and deletes all invalid elements
 *        (that do not match the red and white count when compared to the previous
 *        try)
 * @param colors Array of colors (previous try that was sent to the server)
 * @param server_red Red count for the previous try
 * @param server_white White count for the previous try
 */
static void get_next_try(uint8_t* colors, uint8_t server_red, uint8_t server_white);

/**
 * @brief free allocated resources
 */
static void free_resources(void);

/**
 * @brief Signal handler
 * @param sig Signal number catched
 */
static void signal_handler(int sig);


/* === Implementations === */

static void parse_args(int argc, char **argv, struct addrinfo **addr_list)
{
  char *port_arg;
  char *hostname_arg;
  struct addrinfo hints;

  if(argc > 0) {
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
    bail_out(EXIT_FAILURE, "Failed getting address info for hostname: %s", hostname_arg);
  }
}

static void bail_out(int eval, const char *fmt, ...)
{
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
  exit(eval);
}


static int get_color_bytes(uint8_t *colors)
{
  uint16_t erg = 0;
  uint16_t current_color = 0;
  uint16_t parity_calc = 0;
  uint16_t parity_tmp = 0;

  for(int i = 0 ; i < SLOTS; i++){
    current_color = colors[i];
    parity_tmp = current_color;
    current_color = current_color << (i * 3);
    erg = erg | current_color;
    parity_calc ^= parity_tmp ^ (parity_tmp >> 1) ^ (parity_tmp >> 2);
  }

  erg = erg | ((parity_calc << 15));
  return erg;
}

static int send_message(uint8_t *colors)
{
  uint16_t data = 0xF000;
  uint8_t bytes[2];
  data =  get_color_bytes(colors);
  DEBUG("Send message 0x%x\n", data);

  bytes[0] = (data & 0x00FF);
  bytes[1] = (data & 0xFF00) >> 8;
  for(int i = 0; i < 2; i++){
    if(send(sockfd, &bytes[i], sizeof(bytes[i]), 0) < 0){
      return -1;
    }
  }
  return 1;
}

static void get_all_solutions(int i, uint8_t *tmp_solution)
{
  struct try* new;
  int j;

  if(i == SLOTS){
    new = malloc(sizeof(struct try));
    if(new == NULL){
      bail_out(EXIT_FAILURE, "couldn't allocate memory for try\n");
    }
    new->next = NULL;
    for(j = 0; j < SLOTS; j++){
      new->colors[j] = tmp_solution[j];
    }
    new->next = first;
    first = new;
    possible_solutions += 1;

  } else {
    for(j = 0; j < COLORS; j++){
      tmp_solution[i] = j;
      get_all_solutions(i+1, &tmp_solution[0]);
    }
  }
}

static void compare_solutions(uint8_t* red, uint8_t* white, uint8_t * secret, uint8_t * guess)
{
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

static void get_next_try(uint8_t* colors, uint8_t server_red, uint8_t server_white)
{
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

static void free_resources(void)
{
  struct try *delete;
  sigset_t blocked_signals;
  (void) sigfillset(&blocked_signals);
  (void) sigprocmask(SIG_BLOCK, &blocked_signals, NULL);

  /* signals need to be blocked here to avoid race */
  if(terminating == 1) {
    return;
  }
  terminating = 1;

  /* clean up resources */
  DEBUG("Shutting down client\n");
  if(connfd >= 0) {
    (void) close(connfd);
  }
  if(sockfd >= 0) {
    (void) close(sockfd);
  }

  /* free all remaining entries in the search space list */
  while(first != NULL){
    delete = first;
    first = first->next;
    free(delete);
  }

  freeaddrinfo(addr_list);
}

static void signal_handler(int sig)
{
    /* signals need to be blocked by sigaction */
    DEBUG("Caught Signal\n");
    free_resources();
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv){
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

  if(sockfd == -1){
    bail_out(EXIT_FAILURE, "Couldn't create TCP/IP socket");
  }
  if (connfd == -1){
    bail_out(EXIT_FAILURE, "Couldn't connect to server");
  }

  get_all_solutions(0, &tmp_solution[0]);

  while (error == 0) {
    rounds += 1;
    if (send_message(&colors[0]) == -1) {
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
