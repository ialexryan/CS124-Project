#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <debug.h>

void syscall_init(void);

void sys_exit_helper(int status) NO_RETURN;

#endif /* userprog/syscall.h */

