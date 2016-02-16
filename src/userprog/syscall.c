#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler(struct intr_frame *);

#define syscall_type(type, handler) void handler(struct intr_frame *f);
SYSCALL_TYPES
#undef syscall_type

#define syscall_type(type, handler) handler,
void (*handlers[])(struct intr_frame *) = { SYSCALL_TYPES };
#undef syscall_type

void syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void syscall_handler(struct intr_frame *f UNUSED) {
    printf("system call!\n");
    thread_exit();
}

void sys_halt(struct intr_frame *f UNUSED) {
    
}

void sys_exit(struct intr_frame *f UNUSED) {
    
}

void sys_exec(struct intr_frame *f UNUSED) {
    
}

void sys_wait(struct intr_frame *f UNUSED) {
    
}

void sys_create(struct intr_frame *f UNUSED) {
    
}

void sys_remove(struct intr_frame *f UNUSED) {
    
}

void sys_open(struct intr_frame *f UNUSED) {
    
}

void sys_filesize(struct intr_frame *f UNUSED) {
    
}

void sys_read(struct intr_frame *f UNUSED) {
    
}

void sys_write(struct intr_frame *f UNUSED) {
    
}

void sys_seek(struct intr_frame *f UNUSED) {
    
}

void sys_tell(struct intr_frame *f UNUSED) {
    
}

void sys_close(struct intr_frame *f UNUSED) {
    
}

void sys_mmap(struct intr_frame *f UNUSED) {
    
}

void sys_munmap(struct intr_frame *f UNUSED) {
    
}

void sys_chdir(struct intr_frame *f UNUSED) {
    
}

void sys_mkdir(struct intr_frame *f UNUSED) {
    
}

void sys_readdir(struct intr_frame *f UNUSED) {
    
}

void sys_isdir(struct intr_frame *f UNUSED) {
    
}

void sys_inumber(struct intr_frame *f UNUSED) {
    
}
