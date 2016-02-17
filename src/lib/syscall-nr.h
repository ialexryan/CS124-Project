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
    /*HOLD_SYSCALL(HALT, halt, void, void)*/\
    \
    /*!< Terminate this process. */\
    HOLD_SYSCALL(EXIT, exit, void, ARG(int, status))\
    \
    /*!< Start another process. */\
    HOLD_SYSCALL(EXEC, exec, pid_t, ARG(const char *, file))\
    \
    /*!< Wait for a child process to die. */\
    HOLD_SYSCALL(WAIT, wait, int, ARG(pid_t, pid))\
    \
    /*!< Create a file. */\
    HOLD_SYSCALL(CREATE, create, bool, ARG(const char *, file), ARG(unsigned, initial_size))\
    \
    /*!< Delete a file. */\
    HOLD_SYSCALL(REMOVE, remove, bool, ARG(const char *, file))\
    \
    /*!< Open a file. */\
    HOLD_SYSCALL(OPEN, open, int, ARG(const char *, file))\
	\
    /*!< Obtain a file's size. */\
    HOLD_SYSCALL(FILESIZE, filesize, int, ARG(int, fd))\
	\
    /*!< Read from a file. */\
    HOLD_SYSCALL(READ, read, int, ARG(int, fd), ARG(void *, buffer), ARG(unsigned, length))\
	\
    /*!< Write to a file. */\
    HOLD_SYSCALL(WRITE, write, int, ARG(int, fd), ARG(const void *, buffer), ARG(unsigned, length))\
	\
    /*!< Change position in a file. */\
    HOLD_SYSCALL(SEEK, seek, void, ARG(int, fd), ARG(unsigned, position))\
	\
    /*!< Report current position in a file. */\
    HOLD_SYSCALL(TELL, tell, unsigned, ARG(int, fd))\
	\
    /*!< Close a file. */\
    HOLD_SYSCALL(CLOSE, close, void, ARG(int, fd))\
	\
    /* Project 3 and optionally project 4. */\
    \
    /*!< Map a file into memory. */\
    HOLD_SYSCALL(MMAP, mmap, mapid_t, ARG(int, fd), ARG(void *, addr))\
    \
    /*!< Remove a memory mapping. */\
    HOLD_SYSCALL(MUNMAP, munmap, void, ARG(mapid_t, mapid))\
	\
	/* Project 4 only. */\
    \
    /*!< Change the current directory. */\
    HOLD_SYSCALL(CHDIR, chdir, bool, ARG(const char *, dir))\
    \
    /*!< Create a directory. */\
	HOLD_SYSCALL(MKDIR, mkdir, bool, ARG(const char *, dir))\
    \
    /*!< Reads a directory entry. */\
    /*HOLD_SYSCALL(READDIR, readdir, bool, ARG(int, fd), ARG(char (*)[READDIR_MAX_LEN + 1], name))*/\
    \
    /*!< Tests if a fd represents a directory. */\
    HOLD_SYSCALL(ISDIR, isdir, bool, ARG(int, fd))\
	\
    /*!< Returns the inode number for a fd. */\
    HOLD_SYSCALL(INUMBER, inumber, int, ARG(int, fd))

/*! System call numbers. */
#define HOLD_SYSCALL(TYPE_NAME, ...) SYS_ ## TYPE_NAME,
enum { SYSCALL_LIST };
#undef HOLD_SYSCALL

#endif /* lib/syscall-nr.h */
