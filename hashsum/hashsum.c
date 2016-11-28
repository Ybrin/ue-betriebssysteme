/**
 * @file hashsum.c
 * @author Koray Koska <e1528624@student.tuwien.ac.at>
 * @date 24.11.2016
 *
 * @brief hashsum for assignment 2
 *
 * This program implements the hashsum function from assignment 2
 **/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

/* Length of an array */
#define COUNT_OF(x) (sizeof(x)/sizeof(x[0]))

// Name of the program
static char *program_name = "client";

// Exits the program with an error message
static void usage(void);

static char *getHash(char *path);

static int isFile(char *path);

int main(int argc, char **argv)
{
    char *ignore = NULL;
    char *path;

    if (argc > 0) {
        program_name = argv[0];
    }

    int getopt_result;
    while ((getopt_result = getopt(argc, argv, "i:")) != -1) {
        switch (getopt_result) {
        case 'i':
            if (ignore != NULL) {
              usage();
            }
            ignore = optarg;
            break;
        case '?':
            usage();
            break;
        default:
            assert(0);
        }
    }

    if (argc != optind + 1) {
      usage();
    }

    path = argv[argc - 1];

    /*
    printf("%s\n", "argc");
    printf("%d\n", argc);
    printf("%s\n", "optind");
    printf("%d\n", optind);

    printf("%s\n", "SUCCESS");
    printf("%s\n", ignore);

    printf("%s\n", "SUCCESS");
    printf("%s\n", path);
    */

    // Create pipe
    int fd[2];
    if (pipe(fd) < 0) {
      (void) fprintf(stderr, "Failed while creating pipe()...");
      exit(EXIT_FAILURE);
    }

    pid_t parent = getpid();
    pid_t pid = fork();

    if (pid == -1) {
      // error, failed to fork()
      (void) fprintf(stderr, "%s\n", "Failed to fork() process...");
      exit(EXIT_FAILURE);
    } else if (pid > 0) {
      close(fd[1]);

      int status;
      waitpid(pid, &status, 0);
      // printf("%s: %d\n", "STATUS", status);
      if (status != 0) {
        (void) fprintf(stderr, "%s\n", "Something went wrong with a child process...");
        exit(EXIT_FAILURE);
      }

      // We are in the parent process

      char *line = NULL;
      size_t len = 0;
      ssize_t read;

      FILE *file = fdopen(fd[0], "r");
      if (file == NULL) {
        (void) fprintf(stderr, "%s\n", "Failed to open file...");
        exit(EXIT_FAILURE);
      }

      while ((read = getline(&line, &len, file)) != -1) {
          // printf("Retrieved line of length %zu :\n", read);
          char tmpPath[COUNT_OF(path)];
          strcpy(tmpPath, path);

          // Combine path and line to be the path to the *file*
          strcat(strcat(tmpPath, "/"), line);

          // Remove newline characters
          char *s;
          if ((s = strchr(tmpPath, '\n')) != NULL) { *s = '\0'; }

          if (!isFile(tmpPath)) {
            // printf("%s\n", tmpPath);
            // printf("%s\n", line);
            // printf("%s\n", "Is this a file? This is not a file!");
            continue;
          }
          char *hash = getHash(tmpPath);
          printf("%s ", line);
          printf("%s", hash);
      }
    } else {
      close(fd[0]);

      dup2(fd[1], STDOUT_FILENO);

      // we are the child
      char *cmd[] = { "ls", "-1a", path, (char *) 0 };
      (void) execvp ("ls", cmd);
      assert(0);   // exec never returns
    }

    return EXIT_SUCCESS;
}

static void usage(void)
{
    (void) fprintf(stderr, "Usage: %s [-i ignoreprefix] <directory>\n",
                   program_name);
    exit(EXIT_FAILURE);
}

static char *getHash(char *path)
{
  // Create pipe
  int fd[2];
  if (pipe(fd) < 0) {
    (void) fprintf(stderr, "Failed while creating pipe()...");
    exit(EXIT_FAILURE);
  }

  pid_t parent = getpid();
  pid_t pid = fork();

  if (pid == -1) {
    // error, failed to fork()
    (void) fprintf(stderr, "%s\n", "Failed to fork() process...");
    exit(EXIT_FAILURE);
  } else if (pid > 0) {
    close(fd[1]);

    int status;
    waitpid(pid, &status, 0);
    // printf("%s: %d\n", "STATUS", status);
    if (status != 0) {
      (void) fprintf(stderr, "%s\n", "Something went wrong with a child process...");
      exit(EXIT_FAILURE);
    }

    // We are in the parent process

    char *line = NULL;
    size_t len = 0;

    FILE *file = fdopen(fd[0], "r");
    if (file == NULL) {
      (void) fprintf(stderr, "%s\n", "Failed to open file...");
      exit(EXIT_FAILURE);
    }

    if ((getline(&line, &len, file)) == -1) {
      (void) fprintf(stderr, "%s\n", "Faild to read from pipe...");
      exit(EXIT_FAILURE);
    }

    return line;

  } else {
    close(fd[0]);

    dup2(fd[1], STDOUT_FILENO);

    // we are the child
    char *cmd[] = { "md5sum", path, (char *) 0 };
    (void) execvp ("md5sum", cmd);
    assert(0);   // exec never returns
  }

  return "";
}

static int isFile(char *path)
{
  // printf("%s\n", path);
  struct stat s;
  if (stat(path, &s) == 0) {
    if (s.st_mode & S_IFREG) {
      //it's a file
      return 1;
    }
    // printf("%s\n", "AT LEAST I TRIED");
  }

  // printf("%s\n", "AWESOME");

  return 0;
}
