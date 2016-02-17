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
#define SYSCALL_TYPES \
    syscall_type(SYS_HALT,     sys_halt)     /*!< Halt the operating system. */             \
    syscall_type(SYS_EXIT,     sys_exit)     /*!< Terminate this process. */                \
    syscall_type(SYS_EXEC,     sys_exec)     /*!< Start another process. */                 \
    syscall_type(SYS_WAIT,     sys_wait)     /*!< Wait for a child process to die. */       \
    syscall_type(SYS_CREATE,   sys_create)   /*!< Create a file. */                         \
    syscall_type(SYS_REMOVE,   sys_remove)   /*!< Delete a file. */                         \
    syscall_type(SYS_OPEN,     sys_open)     /*!< Open a file. */                           \
    syscall_type(SYS_FILESIZE, sys_filesize) /*!< Obtain a file's size. */                  \
    syscall_type(SYS_READ,     sys_read)     /*!< Read from a file. */                      \
    syscall_type(SYS_WRITE,    sys_write)    /*!< Write to a file. */                       \
    syscall_type(SYS_SEEK,     sys_seek)     /*!< Change position in a file. */             \
    syscall_type(SYS_TELL,     sys_tell)     /*!< Report current position in a file. */     \
    syscall_type(SYS_CLOSE,    sys_close)    /*!< Close a file. */                          \
                                                                                            \
    /* Project 3 and optionally project 4. */                                               \
    syscall_type(SYS_MMAP,     sys_mmap)     /*!< Map a file into memory. */                \
    syscall_type(SYS_MUNMAP,   sys_munmap)   /*!< Remove a memory mapping. */               \
                                                                                            \
    /* Project 4 only. */                                                                   \
    syscall_type(SYS_CHDIR,    sys_chdir)    /*!< Change the current directory. */          \
    syscall_type(SYS_MKDIR,    sys_mkdir)    /*!< Create a directory. */                    \
    syscall_type(SYS_READDIR,  sys_readdir)  /*!< Reads a directory entry. */               \
    syscall_type(SYS_ISDIR,    sys_isdir)    /*!< Tests if a fd represents a directory. */  \
    syscall_type(SYS_INUMBER,  sys_inumber)  /*!< Returns the inode number for a fd. */

/*! System call numbers. */
#define syscall_type(type, handler) type,
enum { SYSCALL_TYPES };
#undef syscall_type

#endif /* lib/syscall-nr.h */
