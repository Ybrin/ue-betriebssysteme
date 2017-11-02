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
#include <ctype.h>

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
  buffer = (char *)malloc(40 * sizeof(char));
  if (buffer == NULL) {
    (void) fprintf(stderr, "%s\n", "Unable to allocate buffer.");
    exit(EXIT_FAILURE);
  }

  // printf("Type something: ");

  int c = getchar();
  int count = 0;
  while (c != '\n') {
    // Ignore whitespaces if -s option is enabled
    if (ignoreWhitespaces && c == 32) {
      c = getchar();
      continue;
    }
    if (count >= 40) {
      (void) fprintf(stderr, "You can't type in more than 40 characters.\n");
      exit(EXIT_FAILURE);
    }
    if (ignoreCase) {
      buffer[count] = tolower(c);
    } else {
      buffer[count] = c;
    }
    c = getchar();
    count++;
  }

  for (int i = 0; i < count; i++) {
    if (buffer[i] != buffer[count - 1 -  i]) {
      // printf("%s\n", "This is not a palindrom!");
      for (int j = 0; j < count; j++) {
        printf("%c", buffer[j]);
      }
      printf("%s\n", " ist kein Palindrom");
      exit(EXIT_SUCCESS);
    }
  }

  for (int i = 0; i < count; i++) {
    printf("%c", buffer[i]);
  }
  printf("%s\n", " ist ein Palindrom");

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
