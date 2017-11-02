/**
 * @file ispalindrom.c
 * @author Koray Koska <e1528624@student.tuwien.ac.at>
 * @date 30.10.2016
 *
 * @brief ispalindrom for assignment 1
 *
 * This program implements the ispalindrom function from assignment 1
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

/// Name of this program
static const char *programName = "palindrom";

// ******* Function signatures *******

static void usage(void);

// ******* End Function signatures *******

/** The starting point of the program
 *
 * @param argc the number of arguments
 * @param argv the arguments
 * @return 0 if successful, something other if not
 */
int main(int argc, char *const *argv) {
  int ignoreWhitespaces = 0;
  int ignoreCase = 0;

  if (argc > 0) {
    programName = argv[0];
  }

  int getopt_result;
  while ((getopt_result = getopt(argc, argv, "si")) != -1) {
    switch (getopt_result) {
      case 's':
        ignoreWhitespaces = 1;
        break;
      case 'i':
        ignoreCase = 1;
        break;
      case '?':
        usage();
        break;
      default:
        assert(0);
    }
  }

  if (argc != optind) {
    usage();
  }

  char *buffer;
  size_t buffsize = 40;
  size_t characters;

  buffer = (char *)malloc(buffsize * sizeof(char));
  if (buffer == NULL) {
    (void) fprintf(stderr, "%s\n", "Unable to allocate buffer.");
    exit(EXIT_FAILURE);
  }

  printf("Type something: ");
  characters = getline(&buffer, &buffsize, stdin);
  printf("%zu characters were read.\n", characters);
  printf("You typed: '%s'\n", buffer);

  // Free allocated memory
  free(buffer);

  return EXIT_SUCCESS;
}

/**
 * Prints the usage of this program and terminates with EXIT_FAILURE
 */
static void usage(void) {
  (void) fprintf(stderr, "Usage: %s [-s] [-i]\n",
                 programName);
  exit(EXIT_FAILURE);
}
