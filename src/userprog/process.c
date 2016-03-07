#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "vm/frame.h"
#include <hash.h>

static thread_func start_process NO_RETURN;
static bool load(const char *cmdline, void (**eip)(void), void **esp);

/*! Starts a new thread running a user program loaded from FILENAME.  The new
    thread may be scheduled (and may even exit) before process_execute()
    returns.  Returns the new process's thread id, or TID_ERROR if the thread
    cannot be created. */
tid_t process_execute(const char *file_name) {
    char *fn_copy;
    tid_t tid;

    /* Make a copy of file_name.
       Otherwise there's a race between the caller and load(). */
    fn_copy = palloc_get_page(0);
    if (fn_copy == NULL)
        return TID_ERROR;
    strlcpy(fn_copy, file_name, PGSIZE);

    /* Create a new thread to execute FILE_NAME. */
    tid = thread_create(file_name, PRI_DEFAULT, start_process, fn_copy);
    if (tid == TID_ERROR)
        palloc_free_page(fn_copy);
    return tid;
}

/*! A thread function that loads a user process and starts it running. */
static void start_process(void *file_name_) {
    char *file_name = file_name_;
    struct intr_frame if_;
    bool success;

    /* Initialize interrupt frame and load executable. */
    memset(&if_, 0, sizeof(if_));
    if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
    if_.cs = SEL_UCSEG;
    if_.eflags = FLAG_IF | FLAG_MBS;
    success = load(file_name, &if_.eip, &if_.esp);

    /* If load failed, quit. */
    struct thread *thread = thread_current();
    palloc_free_page(file_name);
    if (!success) {
        // Let parent know that we failed loading.
        thread->load_status = -1;
        sema_up(&(thread->loaded));
        thread_exit();
    } else {
        // Let parent know that we're done loading.
        thread->load_status = 0;
        sema_up(&(thread->loaded));
    }
    
    /* Start the user process by simulating a return from an
       interrupt, implemented by intr_exit (in
       threads/intr-stubs.S).  Because intr_exit takes all of its
       arguments on the stack in the form of a `struct intr_frame',
       we just point the stack pointer (%esp) to our stack frame
       and jump to it. */
    asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
    NOT_REACHED();
}

/*! Waits for thread TID to die and returns its exit status.  If it was
    terminated by the kernel (i.e. killed due to an exception), returns -1.
    If TID is invalid or if it was not a child of the calling process, or if
    process_wait() has already been successfully called for the given TID,
    returns -1 immediately, without waiting.
*/
int process_wait(tid_t child_tid) {
    struct list *children = &(thread_current()->children);

    struct list_elem *elem;
    for (elem = list_begin(children); elem != list_end(children); elem = list_next(elem)) {
        struct thread* child = list_entry(elem, struct thread, child_elem);
        if (child->tid == child_tid) {
            // We found the child thread that we're waiting for!
            list_remove(elem);
            
            // Wait until this child is waiting to die...
            sema_down(&(child->dying));
            
            int exit_status = child->exit_status;
            
            // Let it die. It just wants the misery to end.
            child->status = THREAD_DYING;
            thread_murder(child);
            
            // Now its exit_status is set, so return it.
            return exit_status;
        }
    }
    return -1;
}

/*! Free the current process's resources. */
void process_exit(void) {
    struct thread *cur = thread_current();
    uint32_t *pd;
    
    // Remove all pages from virtual memory and write mmap'd pages back to disk
    pagetable_uninstall_all(&cur->pagetable);

    file_close(cur->executable_file);
    
    // Deallocate the buckets
    hash_destroy(&cur->pagetable, NULL);

    /* Destroy the current process's page directory and switch back
       to the kernel-only page directory. */
    pd = cur->pagedir;
    if (pd != NULL) {
        /* Correct ordering here is crucial.  We must set
           cur->pagedir to NULL before switching page directories,
           so that a timer interrupt can't switch back to the
           process page directory.  We must activate the base page
           directory before destroying the process's page
           directory, or our active page directory will be one
           that's been freed (and cleared). */
        cur->pagedir = NULL;
        pagedir_activate(NULL);
        pagedir_destroy(pd);
    }
}

/*! Sets up the CPU for running user code in the current thread.
    This function is called on every context switch. */
void process_activate(void) {
    struct thread *t = thread_current();

    /* Activate thread's page tables. */
    pagedir_activate(t->pagedir);

    /* Set thread's kernel stack for use in processing interrupts. */
    tss_update();
}

/*! We load ELF binaries.  The following definitions are taken
    from the ELF specification, [ELF1], more-or-less verbatim.  */

/*! ELF types.  See [ELF1] 1-2. @{ */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;
/*! @} */

/*! For use with ELF types in printf(). @{ */
#define PE32Wx PRIx32   /*!< Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /*!< Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /*!< Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /*!< Print Elf32_Half in hexadecimal. */
/*! @} */

/*! Executable header.  See [ELF1] 1-4 to 1-8.
    This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
};

/*! Program header.  See [ELF1] 2-2 to 2-4.  There are e_phnum of these,
    starting at file offset e_phoff (see [ELF1] 1-6). */
struct Elf32_Phdr {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
};

/*! Values for p_type.  See [ELF1] 2-3. @{ */
#define PT_NULL    0            /*!< Ignore. */
#define PT_LOAD    1            /*!< Loadable segment. */
#define PT_DYNAMIC 2            /*!< Dynamic linking info. */
#define PT_INTERP  3            /*!< Name of dynamic loader. */
#define PT_NOTE    4            /*!< Auxiliary info. */
#define PT_SHLIB   5            /*!< Reserved. */
#define PT_PHDR    6            /*!< Program header table. */
#define PT_STACK   0x6474e551   /*!< Stack segment. */
/*! @} */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. @{ */
#define PF_X 1          /*!< Executable. */
#define PF_W 2          /*!< Writable. */
#define PF_R 4          /*!< Readable. */
/*! @} */

static void setup_stack(void **esp);
static bool validate_segment(const struct Elf32_Phdr *, struct file *);

/*! Loads an ELF executable from FILE_NAME into the current thread.  Stores the
    executable's entry point into *EIP and its initial stack pointer into *ESP.
    Returns true if successful, false otherwise. */
bool load(const char *file_name, void (**eip) (void), void **esp) {
    struct thread *t = thread_current();
    struct Elf32_Ehdr ehdr;
    struct file *file = NULL;
    off_t file_ofs;
    bool success = false;
    int i;

    /* Split file_name by spaces */
    /* strtok_r needs a mutable copy of the argument */
    char file_name_copy[128];
    strlcpy(file_name_copy, file_name, 128);  // This is nice and safe, should truncate at 127 chars

    char *saveptr;
    char *program_name, *foo;
    program_name = strtok_r(file_name_copy, " ", &saveptr);  // First token is the program name
    strlcpy(t->name, program_name, 16);

    /* Allocate and activate page directory. */
    t->pagedir = pagedir_create();
    if (t->pagedir == NULL)
        goto done;
    process_activate();

    /* Open executable file. */
    file = filesys_open(program_name);
    if (file == NULL) {
        printf("load: %s: open failed\n", file_name);
        goto done;
    }

    /* Deny write to executable. Write protection is only enforced
    for as long as this file stays open, so it has to stay open
    until the process terminates. */ 
    file_deny_write(file);
    t->executable_file = file;

    /* Read and verify executable header. */
    if (file_read(file, &ehdr, sizeof ehdr) != sizeof ehdr ||
        memcmp(ehdr.e_ident, "\177ELF\1\1\1", 7) || ehdr.e_type != 2 ||
        ehdr.e_machine != 3 || ehdr.e_version != 1 ||
        ehdr.e_phentsize != sizeof(struct Elf32_Phdr) || ehdr.e_phnum > 1024) {
        printf("load: %s: error loading executable\n", program_name);
        goto done;
    }

    /* Read program headers. */
    file_ofs = ehdr.e_phoff;
    for (i = 0; i < ehdr.e_phnum; i++) {
        struct Elf32_Phdr phdr;

        if (file_ofs < 0 || file_ofs > file_length(file))
            goto done;
        file_seek(file, file_ofs);

        if (file_read(file, &phdr, sizeof phdr) != sizeof phdr)
            goto done;

        file_ofs += sizeof phdr;

        switch (phdr.p_type) {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
            /* Ignore this segment. */
            break;

        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
            goto done;

        case PT_LOAD:
            if (validate_segment(&phdr, file)) {
                bool writable = (phdr.p_flags & PF_W) != 0;
                uint32_t file_page = phdr.p_offset & ~PGMASK;
                uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
                uint32_t page_offset = phdr.p_vaddr & PGMASK;
                uint32_t read_bytes, zero_bytes;
                if (phdr.p_filesz > 0) {
                    /* Normal segment.
                       Read initial part from disk and zero the rest. */
                    read_bytes = page_offset + phdr.p_filesz;
                    zero_bytes = (ROUND_UP(page_offset + phdr.p_memsz, PGSIZE) -
                                 read_bytes);
                }
                else {
                    /* Entirely zero.
                       Don't read anything from disk. */
                    read_bytes = 0;
                    zero_bytes = ROUND_UP(page_offset + phdr.p_memsz, PGSIZE);
                }
                // Install the supplemental page table entries for this
                // segment so that it can be lazily loaded.
                pagetable_install_segment(&thread_current()->pagetable,
                                          file,
                                          file_page,
                                          read_bytes,
                                          zero_bytes,
                                          writable,
                                          (void *)mem_page);
            }
            else {
                goto done;
            }
            break;
        }
    }

    /* Set up stack. */
    setup_stack(esp);

    /* Here's where we tokenize the rest of the command string
       and push all the arguments onto the stack.
       The code that follows is horribly impenetrable.
       Remember that the stack grows downward.
       Remember esp is a void**.
       Check out this real, tested example of how our stack will look for
       "echo -l foo barrr":

        Address	    Name	        Data	    Type
        0xbffffffb	argv[0][...]	echo\0	    char[5]
        0xbffffff8	argv[1][...]	-l\0	    char[3]
        0xbffffff4	argv[2][...]	foo\0	    char[4]
        0xbfffffee	argv[3][...]	barrr\0	    char[6]
        0xbfffffec	word-align	    0 0	        uint8_t
        0xbfffffe8	argv[4]	        0	        char *
        0xbfffffe4	argv[3]	        0xbfffffee	char *
        0xbfffffe0	argv[2]      	0xbffffff4	char *
        0xbfffffdc	argv[1]	        0xbffffff8	char *
        0xbfffffd8	argv[0]	        0xbffffffb	char *
        0xbfffffd4	argv	        0xbfffffd8	char **
        0xbfffffd0	argc	        4	        int
        0xbfffffcc	return address	0	        void (*) ()
    */

    int argc = 0;
    char* argv[96] = {};  // Keep track of where we put all the arguments

    // Push the name of the program on the stack
    *esp -= strlen(program_name) + 1;  // strlen doesn't count the null-terminator
    strlcpy(*esp, program_name, strlen(program_name) + 1);
    argv[0] = (char*)(*esp);
    argc++;

    // Push each space-separated argument onto the stack
    while ((foo = strtok_r(NULL, " ", &saveptr))) {  // heads up, this line is an assignment
        *esp -= strlen(foo) + 1;
        strlcpy(*esp, foo, strlen(foo) + 1);
        argv[argc] = (char*)(*esp);
        argc++;
    }

    // Pad to word-aligned access
    int padding_length = (uint32_t)(*esp) % 4;
    *esp -= padding_length;
    memset(*esp, 0, padding_length);

    // Loop over everything in argv __from last to first__, pushing it into the stack
    for (i = argc; i >= 0; i--) {
        *esp -= sizeof(char*);
        *((char**)*esp) = argv[i];    // here be dragons
    }

    // Push the actual stack address of argv onto the stack (this seems kind of silly)
    char** x = *esp;
    *esp -= sizeof(char*);
    *((char***)*esp) = x;

    // Push argc onto the stack
    *esp -= sizeof(int);
    *((int*)*esp) = argc;

    // And finally, push a "return address" (unused) of zero
    *esp -= sizeof(void*);
    memset(*esp, 0, sizeof(void*));


    /* Start address. */
    *eip = (void (*)(void)) ehdr.e_entry;

    success = true;

done:
    /* We arrive here whether the load is successful or not. */
    // file_close(file); This is where we'd close the file if not for denying writes.
    return success;
}

/* load() helpers. */

/*! Checks whether PHDR describes a valid, loadable segment in
    FILE and returns true if so, false otherwise. */
static bool validate_segment(const struct Elf32_Phdr *phdr, struct file *file) {
    /* p_offset and p_vaddr must have the same page offset. */
    if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK))
        return false;

    /* p_offset must point within FILE. */
    if (phdr->p_offset > (Elf32_Off) file_length(file))
        return false;

    /* p_memsz must be at least as big as p_filesz. */
    if (phdr->p_memsz < phdr->p_filesz)
        return false;

    /* The segment must not be empty. */
    if (phdr->p_memsz == 0)
        return false;

    /* The virtual memory region must both start and end within the
       user address space range. */
    if (!is_user_vaddr((void *) phdr->p_vaddr))
        return false;
    if (!is_user_vaddr((void *) (phdr->p_vaddr + phdr->p_memsz)))
        return false;

    /* The region cannot "wrap around" across the kernel virtual
       address space. */
    if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
        return false;

    /* Disallow mapping page 0.
       Not only is it a bad idea to map page 0, but if we allowed it then user
       code that passed a null pointer to system calls could quite likely panic
       the kernel by way of null pointer assertions in memcpy(), etc. */
    if (phdr->p_vaddr < PGSIZE)
        return false;

    /* It's okay. */
    return true;
}

/*! Create a minimal stack by mapping a zeroed page at the top of
    user virtual memory. */
static void setup_stack(void **esp) {
    struct hash *pagetable = &thread_current()->pagetable;
    void *address = (void *)((uint8_t *) PHYS_BASE) - PGSIZE;
    
    // Set up allocation of initial stack frame and eagerly load stack frame
    pagetable_install_and_load_allocation(pagetable, address);
    
    // Set up stack pointer
    *esp = PHYS_BASE;
}

