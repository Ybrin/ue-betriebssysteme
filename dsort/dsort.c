/**
 * @file dsort.c
 * @author Koray Koska <e1528624@student.tuwien.ac.at>
 * @date 03.12.2017
 *
 * @brief dsort for assignment 2
 *
 * This program implements the dsort function from assignment 2
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

/// Name of this program
static const char *programName = "dsort";

// ******* Function signatures *******

static void getLines(char *command, char ***lines, unsigned int* linesCount);

static void uniq(char **lines, unsigned int linesCount, char ***uniqLines, unsigned int *uniqLinesCount);

static void usage(void);

static void recursiveFree(char** lines, unsigned int linesCount);

static int stringCmp(const void *a, const void *b);

// ******* End Function signatures *******

/** The starting point of the program
 *
 * @param argc the number of arguments
 * @param argv the arguments
 * @return 0 if successful, something other if not
 */
int main(int argc, char *argv[]) {
  if (argc > 0) {
    programName = argv[0];
  }
  if (argc != 3) {
    usage();
  }

  char *commandOne = argv[1];
  char *commandTwo = argv[2];

  // All lines and its current count
  char **lines = NULL;
  unsigned int linesCount = 0;

  // Run first command
  getLines(commandOne, &lines, &linesCount);

  // Run second command
  getLines(commandTwo, &lines, &linesCount);

  /*for (int i = 0; i < linesCount; i++) {
    printf("%s", lines[i]);
  }*/

  // Sort out lines
  qsort(lines, linesCount, sizeof(char*), stringCmp);

  /*printf("* Sorted *\n");
  for (int i = 0; i < linesCount; i++) {
    printf("%s", lines[i]);
  }*/

  // printf("*** AAA ***\n");

  char **uniqLines = NULL;
  unsigned int uniqLinesCount = 0;
  uniq(lines, linesCount, &uniqLines, &uniqLinesCount);

  for (int i = 0; i < uniqLinesCount; i++) {
    printf("%s", uniqLines[i]);
  }

  // Free global stuff
  recursiveFree(lines, linesCount);

  return EXIT_SUCCESS;
}

/**
 * Runs the given command and returns the lines
 *
 * @param command The command you want to run
 * @param lines A pointer to the lines which will be fetched
 * @param linesCount A pointer to the number of lines fetched
 */
static void getLines(char *command, char ***lines, unsigned int* linesCount) {
  // pipe
  int pipes[2];
  if (pipe(pipes) != 0) {
    (void) fprintf(stderr, "pipe() call failed.\n");
    exit(EXIT_FAILURE);
  }

  pid_t childPid = fork();
  if (childPid == -1) {
    (void) fprintf(stderr, "fork() call failed.\n");
    close(pipes[0]);
    close(pipes[1]);
    exit(EXIT_FAILURE);
  }

  if (childPid == 0) {
    // Child process. Run command and exit
    // Close read end
    close(pipes[0]);

    // dup2 stdout with write end
    if (dup2(pipes[1], STDOUT_FILENO) == -1) {
      (void) fprintf(stderr, "dup2() call failed.\n");
      close(pipes[1]);
      exit(EXIT_FAILURE);
    }

    char *buf = malloc(sizeof(char*) * 2048);
    sprintf(buf, "%s%s%s", "/bin/bash -c \"", command, "\"");
    if (system(buf) == -1) {
      (void) fprintf(stderr, "command() call failed.\n");
      free(buf);
      close(pipes[1]);
      exit(EXIT_FAILURE);
    }
    free(buf);

    close(pipes[1]);

    exit(EXIT_SUCCESS);
  } else {
    // Parent process. Wait for child to exit and read from its stdout
    // Close write end
    close(pipes[1]);

    // dup2 stdin with read end
    if (dup2(pipes[0], STDIN_FILENO) == -1) {
      (void) fprintf(stderr, "dup2() call failed.\n");
      close(pipes[0]);
      exit(EXIT_FAILURE);
    }

    int *status = malloc(sizeof(int));
    if (waitpid(childPid, status, 0) == -1) {
      (void) fprintf(stderr, "waitpid() call failed.\n");
      free(status);
      close(pipes[0]);
      exit(EXIT_FAILURE);
    }

    if (*status != 0) {
      (void) fprintf(stderr, "child process returned non zero exit code.\n");
      free(status);
      close(pipes[0]);
      exit(EXIT_FAILURE);
    }

    // Read
    char *line = malloc(sizeof(char) * 1024);
    while (fgets(line, 1024, stdin) != NULL) {
      if (*linesCount == 0) {
        *lines = malloc(sizeof(char*));
      } else {
        *lines = realloc(*lines, sizeof(char*) * (*linesCount + 1));
      }
      (*lines)[*linesCount] = line;
      (*linesCount)++;
      line = malloc(sizeof(char) * 1024);
    }
    // Last allocated line won't be used
    free(line);
    // Reset stdin
    clearerr(stdin);

    /*
    for (int i = 0; i < linesCount; i++) {
      printf("%s", lines[i]);
    }*/

    free(status);

    close(pipes[0]);
  }
}

/**
 * Runs uniq -d on top of the given lines and writes the response
 * to uniqLines.
 *
 * @param lines A pointer to the lines you want to uniq
 * @param linesCount The number of lines in lines
 * @param uniqLines A pointer to the lines which were generated by uniq
 * @param uniqLinesCount A Pointer to the number of lines fetched by uniq
 */
static void uniq(char **lines, unsigned int linesCount, char ***uniqLines, unsigned int *uniqLinesCount) {
  // pipe
  int pipes[2];
  if (pipe(pipes) != 0) {
    (void) fprintf(stderr, "pipe() call failed.\n");
    recursiveFree(lines, linesCount);
    exit(EXIT_FAILURE);
  }

  int linesPipes[2];
  if (pipe(linesPipes) != 0) {
    (void) fprintf(stderr, "pipe() call failed.\n");
    close(pipes[0]);
    close(pipes[1]);
    recursiveFree(lines, linesCount);
    exit(EXIT_FAILURE);
  }

  pid_t childPid = fork();
  if (childPid == -1) {
    (void) fprintf(stderr, "fork() call failed.\n");
    close(pipes[0]);
    close(pipes[1]);
    close(linesPipes[0]);
    close(linesPipes[1]);
    recursiveFree(lines, linesCount);
    exit(EXIT_FAILURE);
  }

  if (childPid == 0) {
    // Child process. Run command and exit
    // Close read end
    close(pipes[0]);

    // Lines!?
    close(linesPipes[1]);
    if (dup2(linesPipes[0], STDIN_FILENO) == -1) {
      (void) fprintf(stderr, "dup2() call failed.\n");
      close(pipes[1]);
      close(linesPipes[0]);
      exit(EXIT_FAILURE);
    }

    // dup2 stdout with write end
    if (dup2(pipes[1], STDOUT_FILENO) == -1) {
      (void) fprintf(stderr, "dup2() call failed.\n");
      close(pipes[1]);
      close(linesPipes[0]);
      exit(EXIT_FAILURE);
    }

    char *buf = malloc(sizeof(char*) * 2048);
    sprintf(buf, "%s%s%s", "/bin/bash -c \"", "uniq -d", "\"");
    if (system(buf) == -1) {
      (void) fprintf(stderr, "command() call failed.\n");
      free(buf);
      close(pipes[1]);
      close(linesPipes[0]);
      exit(EXIT_FAILURE);
    }
    free(buf);

    close(pipes[1]);
    close(linesPipes[0]);

    exit(EXIT_SUCCESS);
  } else {
    // Parent process. Wait for child to exit and read from its stdout
    // Close write end
    close(pipes[1]);

    close(linesPipes[0]);
    // Write given lines to child process
    for (int i = 0; i < linesCount; i++) {
      write(linesPipes[1], lines[i], strlen(lines[i]));
    }
    close(linesPipes[1]);

    // dup2 stdin with read end
    if (dup2(pipes[0], STDIN_FILENO) == -1) {
      (void) fprintf(stderr, "dup2() call failed.\n");
      close(pipes[0]);
      recursiveFree(lines, linesCount);
      exit(EXIT_FAILURE);
    }

    int *status = malloc(sizeof(int));
    if (waitpid(childPid, status, 0) == -1) {
      (void) fprintf(stderr, "waitpid() call failed.\n");
      free(status);
      close(pipes[0]);
      recursiveFree(lines, linesCount);
      exit(EXIT_FAILURE);
    }

    if (*status != 0) {
      (void) fprintf(stderr, "child process returned non zero exit code.\n");
      free(status);
      close(pipes[0]);
      recursiveFree(lines, linesCount);
      exit(EXIT_FAILURE);
    }

    // Read
    char *line = malloc(sizeof(char) * 1024);
    while (fgets(line, 1024, stdin) != NULL) {
      if (*uniqLinesCount == 0) {
        *uniqLines = malloc(sizeof(char*));
      } else {
        *uniqLines = realloc(*uniqLines, sizeof(char*) * (*uniqLinesCount + 1));
      }
      (*uniqLines)[*uniqLinesCount] = line;
      (*uniqLinesCount)++;
      line = malloc(sizeof(char) * 1024);
    }
    // Last allocated line won't be used
    free(line);
    // Reset stdin
    clearerr(stdin);

    free(status);

    close(pipes[0]);
  }
}

/**
 * Prints the usage of this program and terminates with EXIT_FAILURE
 */
static void usage(void) {
  (void) fprintf(stderr, "Usage: %s \"command1\" \"command2\"\n",
                 programName);
  exit(EXIT_FAILURE);
}

/**
 * Frees the given array of strings recursively
 *
 * @param lines The lines you want to recursive free
 * @param linesCount The number of elements in lines
 */
static void recursiveFree(char** lines, unsigned int linesCount) {
  for(int i = 0; i < linesCount; i++) {
    free(lines[i]);
  }
  free(lines);
}

/**
 * Compares two strings
 *
 * @param a The first string
 * @param b The second string
 *
 * @return negative if a is smaller, positive if a is larger and 0 if a and b are equal
 */
static int stringCmp(const void *a, const void *b) {
  const char **ia = (const char **)a;
  const char **ib = (const char **)b;
  return strcmp(*ia, *ib);
}
