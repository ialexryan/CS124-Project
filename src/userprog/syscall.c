#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler(struct intr_frame *);

#define ARG_N(f, n, type) (*(type *)((uint32_t *)((f)->esp) + (n)))
#define ARG_0(f, type) ARG_N(f, type, 0)
#define ARG_1(f, type) ARG_N(f, type, 1)
#define ARG_2(f, type) ARG_N(f, type, 2)
#define ARG_3(f, type) ARG_N(f, type, 3)

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
    uint32_t syscall_id = ARG_0(f, uint32_t);
    handlers[syscall_id](f);
}

void sys_halt(struct intr_frame *f UNUSED) {
    printf("sys_halt!\n");
    thread_exit();
}

void sys_exit(struct intr_frame *f UNUSED) {
//    int status = ARG_0(
    printf("sys_exit!\n");
    thread_exit();
}

void sys_exec(struct intr_frame *f UNUSED) {
    printf("sys_exec!\n");
    thread_exit();
}

void sys_wait(struct intr_frame *f UNUSED) {
    printf("sys_wait!\n");
    thread_exit();
}

void sys_create(struct intr_frame *f UNUSED) {
    printf("sys_create!\n");
    thread_exit();
}

void sys_remove(struct intr_frame *f UNUSED) {
    printf("sys_remove!\n");
    thread_exit();
}

void sys_open(struct intr_frame *f UNUSED) {
    printf("sys_open!\n");
    thread_exit();
}

void sys_filesize(struct intr_frame *f UNUSED) {
    printf("sys_filesize!\n");
    thread_exit();
}

void sys_read(struct intr_frame *f UNUSED) {
    printf("sys_read!\n");
    thread_exit();
}

void sys_write(struct intr_frame *f UNUSED) {
    printf("sys_write!\n");
    thread_exit();
}

void sys_seek(struct intr_frame *f UNUSED) {
    printf("sys_seek!\n");
    thread_exit();
}

void sys_tell(struct intr_frame *f UNUSED) {
    printf("sys_tell!\n");
    thread_exit();
}

void sys_close(struct intr_frame *f UNUSED) {
    printf("sys_close!\n");
    thread_exit();
}

void sys_mmap(struct intr_frame *f UNUSED) {
    printf("sys_mmap!\n");
    thread_exit();
}

void sys_munmap(struct intr_frame *f UNUSED) {
    printf("sys_munmap!\n");
    thread_exit();
}

void sys_chdir(struct intr_frame *f UNUSED) {
    printf("sys_chdir!\n");
    thread_exit();
}

void sys_mkdir(struct intr_frame *f UNUSED) {
    printf("sys_mkdir!\n");
    thread_exit();
}

void sys_readdir(struct intr_frame *f UNUSED) {
    printf("sys_readdir!\n");
    thread_exit();
}

void sys_isdir(struct intr_frame *f UNUSED) {
    printf("sys_isdir!\n");
    thread_exit();
}

void sys_inumber(struct intr_frame *f UNUSED) {
    printf("sys_inumber!\n");
    thread_exit();
}
