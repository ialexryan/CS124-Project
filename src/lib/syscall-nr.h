/*! \file sycall-nr.h
 *
 * Specifies the numbers of all supported system calls.  Some syscalls are
 * not available to use until later projects, as students are the ones who
 * must implement them.
 */

#ifndef __LIB_SYSCALL_NR_H
#define __LIB_SYSCALL_NR_H

// Macro keeps enum and handler array in sync
// http://rcr.io/words/syncing-enums-arrays.html
#define SYSCALL_LIST \
    /*!< Halt the operating system. */\
    HOLD_SYSCALL(HALT, halt, void, void)\
    \
    /*!< Terminate this process. */\
    HOLD_SYSCALL(EXIT, exit, void, int)\
    \
    /*!< Start another process. */\
    HOLD_SYSCALL(EXEC, exec, pid_t, const char *)\
    \
    /*!< Wait for a child process to die. */\
    HOLD_SYSCALL(WAIT, wait, int, pid_t)\
    \
    /*!< Create a file. */\
    HOLD_SYSCALL(CREATE, create, bool, const char *, unsigned)\
    \
    /*!< Delete a file. */\
    HOLD_SYSCALL(REMOVE, remove, bool, const char *)\
    \
    /*!< Open a file. */\
    HOLD_SYSCALL(OPEN, open, int, const char *)\
	\
    /*!< Obtain a file's size. */\
    HOLD_SYSCALL(FILESIZE, filesize, int, int)\
	\
    /*!< Read from a file. */\
    HOLD_SYSCALL(READ, read, int, int, void *, unsigned)\
	\
    /*!< Write to a file. */\
    HOLD_SYSCALL(WRITE, write, int, int, const void *, unsigned)\
	\
    /*!< Change position in a file. */\
    HOLD_SYSCALL(SEEK, seek, void, int, unsigned)\
	\
    /*!< Report current position in a file. */\
    HOLD_SYSCALL(TELL, tell, unsigned, int)\
	\
    /*!< Close a file. */\
    HOLD_SYSCALL(CLOSE, close, void, int)\
	\
    /* Project 3 and optionally project 4. */\
    \
    /*!< Map a file into memory. */\
    HOLD_SYSCALL(MMAP, mmap, mapid_t, int, void)\
    \
    /*!< Remove a memory mapping. */\
    HOLD_SYSCALL(MUNMAP, munmap, void, mapid_t)\
	\
	/* Project 4 only. */\
    \
    /*!< Change the current directory. */\
    HOLD_SYSCALL(CHDIR, chdir, bool, const char *)\
    \
    /*!< Create a directory. */\
	HOLD_SYSCALL(MKDIR, mkdir, bool, const char *)\
    \
    /*!< Reads a directory entry. */\
    HOLD_SYSCALL(READDIR, readdir, bool, int, (char (*)[READDIR_MAX_LEN + 1]))\
    \
    /*!< Tests if a fd represents a directory. */\
    HOLD_SYSCALL(ISDIR, isdir, bool, int)\
	\
    /*!< Returns the inode number for a fd. */\
    HOLD_SYSCALL(INUMBER, inumber, int, int)

/*! System call numbers. */
#define HOLD_SYSCALL(TYPE_NAME, ...) SYS_ ## TYPE_NAME,
enum { SYSCALL_LIST };
#undef HOLD_SYSCALL

#endif /* lib/syscall-nr.h */
