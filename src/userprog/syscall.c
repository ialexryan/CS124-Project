#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/input.h"
#include "process.h"

static void syscall_handler(struct intr_frame *);

#define GET_ARG(type, f, n) (*(type *)((uint32_t *)((f)->esp) + (n)))
#define ARG(type, name, f, n) type name = GET_ARG(type, f, n)
#define RET(value, f) ((f)->eax = (uint32_t)(value))

#define syscall_type(type, handler) void handler(struct intr_frame *f);
SYSCALL_TYPES
#undef syscall_type

#define syscall_type(type, handler) handler,
void (*handlers[])(struct intr_frame *) = { SYSCALL_TYPES };
#undef syscall_type

struct file* get_file_pointer_for_fd(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || fd == STDIN_FILENO || fd == STDOUT_FILENO) {
        return NULL;
    } else {
        return thread_current()->file_descriptors[fd];
    }
}

bool verify_user_pointer_is_good(void* p) {
    return is_user_vaddr(p) && pagedir_get_page(thread_current()->pagedir, p) != NULL;
}

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
    ARG(int, status, f, 1);
    thread_current()->exit_status = status;
    thread_exit();
}

void sys_exec(struct intr_frame *f) {
    ARG(const char *, file UNUSED, f, 1);
    tid_t child_tid = process_execute(file);
    
    // If a thread could not be created, fail.
    if (child_tid == TID_ERROR) {
        RET(TID_ERROR, f);
        return;
    }
    
    // Make sure the child successfully loaded.
    struct list *children = &(thread_current()->children);
    struct list_elem *elem;
    // Find child.
    for (elem = list_begin(children); elem != list_end(children); elem = list_next(elem)) {
        struct thread* thread = list_entry(elem, struct thread, child_elem);
        if (thread->tid == child_tid) {
            // Found child! Wait for it to load...
            sema_down(&(thread->loaded));
            RET(thread->load_status, f);
            return;
        }
    }
    
    // The tid should have been valid... awkotaco.
    ASSERT(false);
}

void sys_wait(struct intr_frame *f) {
    ARG(int, pid, f, 1);
    RET(process_wait(pid), f);
}

void sys_create(struct intr_frame *f) {
    ARG(const char *, file, f, 1);
    ARG(unsigned, initial_size, f, 2);

    if (!verify_user_pointer_is_good(file)) {
        thread_current()->exit_status = -1;
        thread_exit();
    }

    if (file == NULL) {
        thread_current()->exit_status = -1;
        thread_exit();
    } else {
        RET(filesys_create(file, initial_size), f);
    }
}

void sys_remove(struct intr_frame *f) {
    ARG(const char *, file, f, 1);
    RET(filesys_remove(file), f);
}

void sys_open(struct intr_frame *f) {
    ARG(const char *, file_name, f, 1);

    if (!verify_user_pointer_is_good(file_name)) {
        thread_current()->exit_status = -1;
        thread_exit();
    }

    if (file_name == NULL) {
        RET(-1, f);
        return;
    }

    struct file* x = filesys_open(file_name);

    if (x == NULL) {
        RET(-1, f);
    } else {
         // Find the first open spot in the file descriptors table
        // and put the pointer to the struct file there.
        int i;
        for (i = 2; i < 32; i++) {  // start at i = 2 b/c first two are reserved for stdin/out
            if (get_file_pointer_for_fd(i) == NULL) {
                thread_current()->file_descriptors[i] = x;
                break;
            }
        }
        // At this point i is the file descriptor; i.e. the index
        // into the array of pointers to struct file's.

        RET(i, f);
    }
}

void sys_filesize(struct intr_frame *f) {
    ARG(int, fd, f, 1);

    ASSERT(fd != STDIN_FILENO);  // These have no size
    ASSERT(fd != STDOUT_FILENO);

    struct file* x = get_file_pointer_for_fd(fd);
    ASSERT(x != NULL);
    RET(file_length(x), f);
}

void sys_read(struct intr_frame *f ) {
    ARG(int, fd, f, 1);
    ARG(void *, buffer, f, 2);
    ARG(unsigned, size, f, 3);

    if (!verify_user_pointer_is_good(buffer)) {
        thread_current()->exit_status = -1;
        thread_exit();
    }

    if (fd == STDOUT_FILENO) {
        RET(-1, f);
        return;
    }

    if (fd == STDIN_FILENO) {
        input_init();
        char* cbuffer = (char*) buffer;
        while (size > 0) {
            *cbuffer = input_getc();
            cbuffer++;
            size--;
        }
        RET(size, f);
    } else {
        struct file *x = get_file_pointer_for_fd(fd);
        if (x == NULL) {
            RET(-1, f);
        } else {
            RET(file_read(x, buffer, size), f);
        }
    }
}

void sys_write(struct intr_frame *f) {
    ARG(int, fd, f, 1);
    ARG(const void *, buffer, f, 2);
    ARG(unsigned, size, f, 3);

    if (!verify_user_pointer_is_good(buffer)) {
        thread_current()->exit_status = -1;
        thread_exit();
    }

    if (fd == STDIN_FILENO) {
        RET(-1, f);
        return;
    }

    if (fd == STDOUT_FILENO) {
        putbuf(buffer, size);
        RET(size, f);
    } else {
        struct file *x = get_file_pointer_for_fd(fd);
        if (x == NULL) {
            RET(-1, f);
        } else {
            RET(file_write(x, buffer, size), f);
        }
    }
}

void sys_seek(struct intr_frame *f) {
    ARG(int, fd, f, 1);
    ARG(unsigned, position, f, 2);

    ASSERT(fd != STDIN_FILENO);
    ASSERT(fd != STDOUT_FILENO);

    struct file *x = get_file_pointer_for_fd(fd);
    ASSERT(x != NULL);

    file_seek(x, position);
}

void sys_tell(struct intr_frame *f) {
    ARG(int, fd, f, 1);

    ASSERT(fd != STDIN_FILENO);
    ASSERT(fd != STDOUT_FILENO);

    struct file *x = get_file_pointer_for_fd(fd);
    ASSERT(x != NULL);

    RET(file_tell(x), f);
}

void sys_close(struct intr_frame *f) {
    ARG(int, fd, f, 1);

    if (fd == STDIN_FILENO || fd == STDOUT_FILENO) {
        return;
    }

    struct file *x = get_file_pointer_for_fd(fd);
    if (x == NULL) { return; }

    thread_current()->file_descriptors[fd] = NULL;
    file_close(x);
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
