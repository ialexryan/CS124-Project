#ifndef __LIB_SYSCALL_SHARED_H
#define __LIB_SYSCALL_SHARED_H

#include <stdbool.h>
#include <debug.h>

/*! Process identifier. */
typedef int pid_t;
#define PID_ERROR ((pid_t) -1)

/*! Map region identifier. */
typedef int mapid_t;
#define MAP_FAILED ((mapid_t) -1)

/*! Maximum characters in a filename written by readdir(). */
#define READDIR_MAX_LEN 14

/*! Typical return values from main() and arguments to exit(). */
#define EXIT_SUCCESS 0          /*!< Successful execution. */
#define EXIT_FAILURE 1          /*!< Unsuccessful execution. */

#endif /* lib/syscall-shared.h */

