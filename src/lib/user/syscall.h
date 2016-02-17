#ifndef __LIB_USER_SYSCALL_H
#define __LIB_USER_SYSCALL_H

#include <debug.h>
#include "../syscall-shared.h"

/* Projects 2 and later. */
void halt(void) NO_RETURN;
void exit(int status) NO_RETURN;
pid_t exec(const char *file);
int wait(pid_t);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open(const char *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned length);
int write(int fd, const void *buffer, unsigned length);
void seek(int fd, unsigned position);
unsigned tell(int fd);
void close(int fd);

/* Project 3 and optionally project 4. */
mapid_t mmap(int fd, void *addr);
void munmap(mapid_t);

/* Project 4 only. */
bool chdir(const char *dir);
bool mkdir(const char *dir);
bool readdir(int fd, char name[READDIR_MAX_LEN + 1]);
bool isdir(int fd);
int inumber(int fd);

#endif /* lib/user/syscall.h */

