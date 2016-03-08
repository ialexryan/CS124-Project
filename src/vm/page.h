#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <stdbool.h>
#include "filesys/file.h"
#include "threads/vaddr.h"

void pagetable_init(void);

// Index that indicates the file should be discarded instead of swapped.
#define DO_NOT_SWAP_INDEX (-1)

// Supplementary page info associated with a page telling us
// how to initialize, load, and evict it.
struct page_info {
    // The user virtual address at which the page is mapped. Used
    // as the key in the hash map.
    void *virtual_address;
    
    // The element in the thread's hash map.
    struct hash_elem hash_elem;
    
    // The current state of the page in memory.
    enum {
        // Uninitialized means it has never before
        // been loaded. On load, it should use the specified
        // `initialization_method`.
        UNINITIALIZED_STATE,
        
        // Evicted means it has already been loaded and later
        // evicted. Use the specified `restoration_method` to
        // load the page.
        EVICTED_STATE,
        
        // Loaded means it is currently loaded. On eviction,
        // it should use the method compatible with the page's
        // `restoration_method`.
        LOADED_STATE
    } state : 2;
    
    bool pinned;

    // The method by which the page should be loaded on first load.
    enum {
        // Load the page such that all memory is set to 0.
        ZERO_INITIALIZATION,
        
        // Load the page using the specified `file_info`.
        FILE_INITIALIZATION
    } initialization_method : 1;
    
    // The method by which the page should be restorated
    // after eviction. A compatible method should be used
    // for eviction as well.
    enum {
        // Restore the page by reading it from swap.
        SWAP_RESTORATION,
        
        // Restore the page by reading its data from file as
        // specified in `file_info`. Note that a page with this
        // restoration method will evict by writing to file unless
        // the page has not been dirtied.
        FILE_RESTORATION
    } restoration_method : 1;
    
    // Whether or not the page supports writes. If that page
    // does not support writes, the eviction method for a page
    // with the `FILE_RESTORATION` restoration method should be
    // to completely toss the data.
    bool writable;
    
    // The data that's used to load the page, either for
    // initialization or after eviction.
    union {
        // If the restoration method is `SWAP_RESTORATION` and
        // the state is `EVICTED_STATE`, this stores the info
        // necessary to load the page back from swap. Otherwise,
        // this contains junk and it is illegal to modify it.
        struct {
            int swap_index;
        } swap_info;
        
        // If the restoration method is `FILE_RESTORATION` and
        // the state is `EVICTED_STATE`, or if the initialization
        // method is FILE_INITIALIZATION and the state is
        // `UNINITIALIZED_STATE`, this stores the info
        // necessary to load the page from file. Otherwise,
        // this contains junk and it is illegal to modify it.
        struct {
            // The file that the page corresponds to.
            struct file *file;
            
            // The offset into the file at which the page begins.
            off_t offset;
            
            // The number of bytes from the file that should be
            // mapped to the page.
            uint32_t num_bytes;
            
            // The next page that is assocated with this file.
            // Utilized for unmapping entire files from a
            // reference to the first page of the file.
            struct page_info *next;
        } file_info;
    };
};

// Used by hash table
bool page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux);
unsigned page_hash(const struct hash_elem *e, void *aux);
void uninstall_page(struct hash_elem *e, void *aux UNUSED);

struct page_info *pagetable_info_for_address(struct hash *pagetable,
                                             void *address);
void pagetable_load_page(struct page_info *page);
void *pagetable_evict_page(struct page_info *page);

void pagetable_install_file(struct hash *pagetable,
                            struct file *file,
                            bool writable,
                            void *address);
void pagetable_install_segment(struct hash *pagetable,
                               struct file *file,
                               off_t offset,
                               int32_t read_bytes,
                               int32_t zero_bytes,
                               bool writable,
                               void *address);
void pagetable_uninstall_file(struct page_info *page);

void pagetable_install_allocation(struct hash *pagetable, void *address);
void pagetable_install_and_load_allocation(struct hash *pagetable, void *address);

void pagetable_uninstall_all(struct hash *pagetable);

struct page_info *pagetable_try_acquire_info_for_address(struct hash *pagetable, void *address);
void release_page(struct page_info *page);

#endif /* vm/page.h */
