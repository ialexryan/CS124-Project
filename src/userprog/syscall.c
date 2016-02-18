#include "userprog/syscall.h"
#include "devices/shutdown.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler(struct intr_frame *);

#define GET_ARG(type, f, n) (*(type *)((uint32_t *)((f)->esp) + (n)))
#define ARG(type, name, f, n) type name = GET_ARG(type, f, n)

#define syscall_type(type, handler) void handler(struct intr_frame *f);
SYSCALL_TYPES
#undef syscall_type

#define syscall_type(type, handler) handler,
void (*handlers[])(struct intr_frame *) = { SYSCALL_TYPES };
#undef syscall_type

void syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void syscall_handler(struct intr_frame *f) {
    // Run system call
    ARG(uint32_t, syscall_id, f, 0);
    handlers[syscall_id](f);
}

void sys_halt(struct intr_frame *f UNUSED) {
    shutdown_power_off();
}

void sys_exit(struct intr_frame *f) {
    ARG(int, status UNUSED, f, 1);
    // TODO: Set exit status
    thread_exit()
}

void sys_exec(struct intr_frame *f) {
    ARG(const char *, file UNUSED, f, 1);
    printf("sys_exec!\n");
    thread_exit();
}

void sys_wait(struct intr_frame *f) {
    ARG(int, pid UNUSED, f, 1);
    printf("sys_wait!\n");
    thread_exit();
}

void sys_create(struct intr_frame *f) {
    ARG(const char *, file UNUSED, f, 1);
    ARG(unsigned, initial_size UNUSED, f, 2);
    //return filesys_create(file, initial_size);
    printf("sys_create!\n");
    thread_exit();
}

void sys_remove(struct intr_frame *f) {
    ARG(const char *, file UNUSED, f, 1);
    //return filesys_remove(file);
    printf("sys_remove!\n");
    thread_exit();
}

void sys_open(struct intr_frame *f) {
    ARG(const char *, file UNUSED, f, 1);
    //return filesys_open(file);
    printf("sys_open!\n");
    thread_exit();
}

void sys_filesize(struct intr_frame *f) {
    ARG(int, fb UNUSED, f, 1);
    file_length()
    printf("sys_filesize!\n");
    thread_exit();
}

void sys_read(struct intr_frame *f ) {
    ARG(int, fd UNUSED, f, 1);
    ARG(void *, buffer UNUSED, f, 2);
    ARG(unsigned, size UNUSED, f, 3);
    printf("sys_read!\n");
    thread_exit();
}

void sys_write(struct intr_frame *f) {
    ARG(int, fd UNUSED, f, 1);
    ARG(const void *, buffer UNUSED, f, 2);
    ARG(unsigned, size UNUSED, f, 3);
    printf("sys_write!\n");
    thread_exit();
}

void sys_seek(struct intr_frame *f) {
    ARG(int, fd UNUSED, f, 1);
    ARG(unsigned, position UNUSED, f, 2);
    printf("sys_seek!\n");
    thread_exit();
}

void sys_tell(struct intr_frame *f) {
    ARG(int, fd UNUSED, f, 1);
    printf("sys_tell!\n");
    thread_exit();
}

void sys_close(struct intr_frame *f) {
    ARG(int, fd UNUSED, f, 1);
    printf("sys_close!\n");
    thread_exit();
}

void sys_mmap(struct intr_frame *f) {
    ARG(int, fd UNUSED, f, 1);
    ARG(void *, addr UNUSED, f, 2);
    printf("sys_mmap!\n");
    thread_exit();
}

void sys_munmap(struct intr_frame *f) {
    ARG(int, mapid UNUSED, f, 1);
    printf("sys_munmap!\n");
    thread_exit();
}

void sys_chdir(struct intr_frame *f) {
    ARG(const char *, dir UNUSED, f, 1);
    printf("sys_chdir!\n");
    thread_exit();
}

void sys_mkdir(struct intr_frame *f) {
    ARG(const char *, dir UNUSED, f, 1);
    printf("sys_mkdir!\n");
    thread_exit();
}

void sys_readdir(struct intr_frame *f) {
    ARG(int, fd UNUSED, f, 1);
    ARG(char *, name UNUSED, f, 2);
    printf("sys_readdir!\n");
    thread_exit();
}

void sys_isdir(struct intr_frame *f) {
    ARG(int, fd UNUSED, f, 1);
    printf("sys_isdir!\n");
    thread_exit();
}

void sys_inumber(struct intr_frame *f) {
    ARG(int, fd UNUSED, f, 1);
    printf("sys_inumber!\n");
    thread_exit();
}
