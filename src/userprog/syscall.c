#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler(struct intr_frame *);

// Gets the nth argument given the stack frame, casting to the correct type
#define ARG_N(f, n, type) (*(type *)((uint32_t *)((f)->esp) + (n)))
#define ARG_N_(TUPLE) ARG_N TUPLE

// Transforms a variadic list of types to a variadic list of arguments from the stack frame
#define SYS_ARGS(f, ...) MAP(ARG_N, MAP(UNCURRY3, PREPEND(f, ENUMERATE(MAP(ARG_TYPE, __VA_ARGS__)))))

// Declares a function that extracts the arguments from the stack and calls the correct handler
#define DECLARE_SYSCALL_HANDLER(NUMBER, FUNC_NAME, RETURN, ...) \
void _sys_ ## FUNC_NAME(struct intr_frame *f) { \
	RETURN result = CONCAT(sys_, FUNC_NAME)(SYS_ARGS(f, __VA_ARGS__)); \
	f->eax = (uint32_t)result; \
};

#define HOLD_SYSCALL(TYPE_NAME, HANDLER_NAME, ...) void sys_ ## HANDLER_NAME(struct intr_frame *f);
SYSCALL_LIST
#undef HOLD_SYSCALL

#define HOLD_SYSCALL(TYPE_NAME, HANDLER_NAME, ...) sys_ ## HANDLER_NAME,
void (*handlers[])(struct intr_frame *) = { SYSCALL_LIST };
#undef HOLD_SYSCALL

void syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void syscall_handler(struct intr_frame *f) {
    // Run system call
    uint32_t syscall_id = ARG_N(f, 0, uint32_t);
    handlers[syscall_id](f);
}

void sys_halt(void) {
    printf("sys_halt!\n");
    thread_exit();
}

void sys_exit(int status) {
    printf("sys_exit!\n");
    thread_exit();
}

pid_t sys_exec(const char *file) {
    printf("sys_exec!\n");
    thread_exit();
}

int sys_wait(pid_t pid) {
    printf("sys_wait!\n");
    thread_exit();
}

bool sys_create(const char *file, unsigned initial_size) {
    printf("sys_create!\n");
    thread_exit();
}

bool sys_remove (const char *file) {
    printf("sys_remove!\n");
    thread_exit();
}

int sys_open (const char *file) {
    printf("sys_open!\n");
    thread_exit();
}

int sys_filesize(int fd) {
    printf("sys_filesize!\n");
    thread_exit();
}

int sys_read(int fd, void *buffer, unsigned size) {
    printf("sys_read!\n");
    thread_exit();
}

int sys_write(int fd, const void *buffer, unsigned size) {
    printf("sys_write!\n");
    thread_exit();
}

void sys_seek(int fd, unsigned position) {
    printf("sys_seek!\n");
    thread_exit();
}

unsigned sys_tell(int fd) {
    printf("sys_tell!\n");
    thread_exit();
}

void sys_close(int fd) {
    printf("sys_close!\n");
    thread_exit();
}

mapid_t sys_mmap(int fd, void *addr) {
    printf("sys_mmap!\n");
    thread_exit();
}

void sys_munmap(mapid_t mapid) {
    printf("sys_munmap!\n");
    thread_exit();
}

bool sys_chdir(const char *dir) {
    printf("sys_chdir!\n");
    thread_exit();
}

bool sys_mkdir(const char *dir) {
    printf("sys_mkdir!\n");
    thread_exit();
}

bool sys_readdir(int fd, char name[READDIR_MAX_LEN + 1]) {
    printf("sys_readdir!\n");
    thread_exit();
}

bool sys_isdir(int fd) {
    printf("sys_isdir!\n");
    thread_exit();
}

int sys_inumber(int fd) {
    printf("sys_inumber!\n");
    thread_exit();
}
