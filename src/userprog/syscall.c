#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <syscall-shared.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <macros.h>

static void syscall_handler(struct intr_frame *);

// Gets the nth argument given the stack frame, casting to the correct type
#define ARG_N(f, n, type) (*(type *)((uint32_t *)((f)->esp) + (n)))
#define ARG_N_(TUPLE) ARG_N TUPLE

// Transforms a variadic list of types to a variadic list of arguments from the stack frame
#define SYS_ARGS(f, ...) MAP(ARG_N_, MAP(UNCURRY3, PREPEND(f, ENUMERATE(MAP(ARG_TYPE, __VA_ARGS__)))))

// Declares a function that extracts the arguments from the stack and calls the correct handler
#define DECLARE_SYSCALL_HANDLER(NUMBER, FUNC_NAME, RETURN, ...) \
static void _sys_ ## FUNC_NAME(struct intr_frame *f) { \
	/*RETURN result = */CONCAT(sys_, FUNC_NAME)(SYS_ARGS(f, __VA_ARGS__)); \
	/*f->eax = (uint32_t)result;*/ \
};

// List delcarations of all sys functions
#define HOLD_SYSCALL(TYPE_NAME, HANDLER_NAME, RETURN, ...) RETURN sys_ ## HANDLER_NAME(MAP(ARG_FULL, __VA_ARGS__));
SYSCALL_LIST
#undef HOLD_SYSCALL

// List handler declarations
#define HOLD_SYSCALL(TYPE_NAME, FUNC_NAME, RETURN, ...) DECLARE_SYSCALL_HANDLER(SYS ## TYPE_NAME, FUNC_NAME, RETURN, __VA_ARGS__)
SYSCALL_LIST
#undef HOLD_SYSCALL

#define HOLD_SYSCALL(TYPE_NAME, HANDLER_NAME, ...) _sys_ ## HANDLER_NAME,
void (*handlers[])(struct intr_frame *) = { SYSCALL_LIST };
#undef HOLD_SYSCALL

void syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void syscall_handler(struct intr_frame *f UNUSED) {
    printf("sys %i!\n", ARG_N(f, 0, uint32_t));
    
    // Run system call
    uint32_t syscall_id = ARG_N(f, 0, uint32_t);
    handlers[syscall_id](f);
}

void sys_halt(void) {
    printf("sys_halt!\n");
    thread_exit();
}

void sys_exit(int status UNUSED) {
    printf("sys_exit!\n");
    thread_exit();
}

pid_t sys_exec(const char *file UNUSED) {
    printf("sys_exec!\n");
    thread_exit();
}

int sys_wait(pid_t pid UNUSED) {
    printf("sys_wait!\n");
    thread_exit();
}

bool sys_create(const char *file UNUSED, unsigned initial_size UNUSED) {
    printf("sys_create!\n");
    thread_exit();
}

bool sys_remove (const char *file UNUSED) {
    printf("sys_remove!\n");
    thread_exit();
}

int sys_open (const char *file UNUSED) {
    printf("sys_open!\n");
    thread_exit();
}

int sys_filesize(int fd UNUSED) {
    printf("sys_filesize!\n");
    thread_exit();
}

int sys_read(int fd UNUSED, void *buffer UNUSED, unsigned size UNUSED) {
    printf("sys_read!\n");
    thread_exit();
}

int sys_write(int fd UNUSED, const void *buffer UNUSED, unsigned size UNUSED) {
    printf("sys_write!\n");
    thread_exit();
}

void sys_seek(int fd UNUSED, unsigned position UNUSED) {
    printf("sys_seek!\n");
    thread_exit();
}

unsigned sys_tell(int fd UNUSED) {
    printf("sys_tell!\n");
    thread_exit();
}

void sys_close(int fd UNUSED) {
    printf("sys_close!\n");
    thread_exit();
}

mapid_t sys_mmap(int fd UNUSED, void *addr UNUSED) {
    printf("sys_mmap!\n");
    thread_exit();
}

void sys_munmap(mapid_t mapid UNUSED) {
    printf("sys_munmap!\n");
    thread_exit();
}

bool sys_chdir(const char *dir UNUSED) {
    printf("sys_chdir!\n");
    thread_exit();
}

bool sys_mkdir(const char *dir UNUSED) {
    printf("sys_mkdir!\n");
    thread_exit();
}

bool sys_readdir(int fd UNUSED, char name[READDIR_MAX_LEN + 1]) {
    (void)name; // unused macro wasn't working
    printf("sys_readdir!\n");
    thread_exit();
}

bool sys_isdir(int fd UNUSED) {
    printf("sys_isdir!\n");
    thread_exit();
}

int sys_inumber(int fd UNUSED) {
    printf("sys_inumber!\n");
    thread_exit();
}
