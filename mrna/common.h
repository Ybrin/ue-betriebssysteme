/**
 * @file common.h
 * @author Koray Koska <e1528624@student.tuwien.ac.at>
 * @date 2018-01-07
 *
 * @brief Commons for client and server.
 */

#include <sys/types.h>
#include <unistd.h>

#define SHM_NAME "/1528624_memory.mem"
#define SHM_MAX_DATA (1024)
#define SHM_PERMISSION (0600)

#define SHM_END_BYTE (0xFF)

#define SHM_SUCCESS_BYTE (0x00)
#define SHM_ERROR_BYTE (0x01)
#define SHM_END_REACHED_BYTE (0x02)

typedef struct {
  // 0 idle, 1 used
  uint8_t state;
  // First byte: command (or response status), rest is free for needed data.
  uint8_t data[SHM_MAX_DATA];
} SharedMemory;

#define SEM_1 "/1528624_sem_1.sem"
#define SEM_2 "/1528624_sem_2.sem"
#define SEM_3 "/1528624_sem_3.sem"
#define SEM_4 "/1528624_sem_4.sem"

#define AMINO_STOP (0xFE)
#define AMINO_START 'M'

uint8_t getAminoAcid(uint8_t f, uint8_t s, uint8_t t) {
  if (f == 'U') {
    if (s == 'U') {
      if (t == 'U') {
        return 'F';
      } else if (t == 'C') {
        return 'F';
      } else if (t == 'A') {
        return 'L';
      } else if (t == 'G') {
        return 'L';
      }
    } else if (s == 'C') {
      return 'S';
    } else if (s == 'A') {
      if (t == 'U') {
        return 'Y';
      } else if (t == 'C') {
        return 'Y';
      } else if (t == 'A') {
        return AMINO_STOP;
      } else if (t == 'G') {
        return AMINO_STOP;
      }
    } else if (s == 'G') {
      if (t == 'U') {
        return 'C';
      } else if (t == 'C') {
        return 'C';
      } else if (t == 'A') {
        return AMINO_STOP;
      } else if (t == 'G') {
        return 'W';
      }
    }
  } else if (f == 'C') {
    if (s == 'U') {
      return 'L';
    } else if (s == 'C') {
      return 'P';
    } else if (s == 'A') {
      if (t == 'U') {
        return 'H';
      } else if (t == 'C') {
        return 'H';
      } else if (t == 'A') {
        return 'Q';
      } else if (t == 'G') {
        return 'Q';
      }
    } else if (s == 'G') {
      return 'R';
    }
  } else if (f == 'A') {
    if (s == 'U') {
      if (t == 'U') {
        return 'I';
      } else if (t == 'C') {
        return 'I';
      } else if (t == 'A') {
        return 'I';
      } else if (t == 'G') {
        return AMINO_START;
      }
    } else if (s == 'C') {
      return 'T';
    } else if (s == 'A') {
      if (t == 'U') {
        return 'N';
      } else if (t == 'C') {
        return 'N';
      } else if (t == 'A') {
        return 'K';
      } else if (t == 'G') {
        return 'K';
      }
    } else if (s == 'G') {
      if (t == 'U') {
        return 'S';
      } else if (t == 'C') {
        return 'S';
      } else if (t == 'A') {
        return 'R';
      } else if (t == 'G') {
        return 'R';
      }
    }
  } else if (f == 'G') {
    if (s == 'U') {
      return 'V';
    } else if (s == 'C') {
      return 'A';
    } else if (s == 'A') {
      if (t == 'U') {
        return 'D';
      } else if (t == 'C') {
        return 'D';
      } else if (t == 'A') {
        return 'E';
      } else if (t == 'G') {
        return 'E';
      }
    } else if (s == 'G') {
      return 'G';
    }
  }

  return 0;
}
