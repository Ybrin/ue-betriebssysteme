/**
 * @file main.c
 * @author Koray Koska (1528624)
 * @date 2016-10-11
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

void printUsage();
void printFileDoesNotExist(char const *filename);
int fileExists(char const *filename);
int compareFiles();

/**
 * The main entry point of the program.
 *
 * @param argc The number of command-line parameters in argv.
 * @param argv The array of command-line parameters, argc elements long.
 * @return The exit code of the program. 0 on success, non-zero on failure.
 */
int main(int argc, char const *argv[]) {
  if (argc < 3 || argc > 4) {
    printUsage();
    return 1;
  }

  int fileOnePosition = 1;
  int fileTwoPosition = 2;

  int caseSensitive = 1;
  if (strcmp(argv[1], "-i") == 0) {
    caseSensitive = 0;
    ++fileOnePosition;
    ++fileTwoPosition;

    if (argc != 4) {
      printUsage();
      return 1;
    }
  }

  if (!fileExists(argv[fileOnePosition])) {
    printFileDoesNotExist(argv[fileOnePosition]);
    return 1;
  }

  if (!fileExists(argv[fileTwoPosition])) {
    printFileDoesNotExist(argv[fileTwoPosition]);
    return 1;
  }

  FILE *fileOne = fopen(argv[fileOnePosition], "r");
  FILE *fileTwo = fopen(argv[fileTwoPosition], "r");

  return compareFiles(fileOne, fileTwo, caseSensitive);

  return 0;
}

/**
 * Prints the usage of 'mydiff'.
 */
void printUsage() {
  (void) printf("%s\n", "Usage: mydiff [-i] file1 file2");
}

/**
 * Prints text indicating that the given filename is invalid or does not exist.
 *
 * @param filename The filename of the file which does not exist.
 */
void printFileDoesNotExist(char const *filename) {
  (void) printf("%s %s %s\n", "mydiff: File", filename, "does not exist!");
}

/**
 * Returns true iff 'filename' is a valid path to an existing file.
 *
 * @param filename The path to the file which should be tested.
 * @return True iff the file exists.
 */
int fileExists(char const *filename) {
   FILE *fp = fopen (filename, "r");
   if (fp!= NULL) { fclose (fp); }
   return (fp != NULL);
}

/**
 * Implements the actual logic behind this program. Prints the number of
 * differences for each line for the given files 'fileOne' and 'fileTwo'.
 * Returns 0 iff the execution is successfull.
 *
 * @param fileOne The first file.
 * @param fileTwo The second file.
 * @param caseSensitive Whether the comparison should be case sensitive or not.
 * @return 0 iff the execution finishes successfully.
 */
int compareFiles(FILE *fileOne, FILE *fileTwo, int caseSensitive) {
  int characterOne;
  int characterTwo;

  int characterCount = 0;

  int fileLine = 1;

  while ((characterOne = getc(fileOne)) != EOF && (characterTwo = getc(fileTwo)) != EOF) {
    if (characterOne != '\n' && characterTwo != '\n') {

      if (!caseSensitive) {
        characterOne = tolower(characterOne);
        characterTwo = tolower(characterTwo);
      }

      if (characterOne != characterTwo) {
        ++characterCount;
        // Debug
        // (void) printf("%c is not %c\n", characterOne, characterTwo);
      }
    } else {
      // Go to the next line
      if (characterOne != '\n') {
        while ((characterOne = getc(fileOne) != '\n')) {
          if (characterOne == EOF) { return 0; }
        }
      }
      // Go to the next line
      if (characterTwo != '\n') {
        while ((characterTwo = getc(fileTwo) != '\n')) {
          if (characterTwo == EOF) { return 0; }
        }
      }

      if (characterCount > 0) {
        (void) printf("%s: %d, %s: %d\n", "Line", fileLine, "Characters", characterCount);
      }
      characterCount = 0;
      ++fileLine;
    }
  }

  return 0;
}
