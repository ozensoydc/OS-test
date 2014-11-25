#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "vm/frame.h"
#include "threads/synch.h"
#include <hash.h>
#include "vm/page.h"
#include "vm/swap.h"

static struct hash frames;
static struct lock frames_lock;

void frame_init(void);
void* get_frame(void *upage, enum palloc_flags flags);
bool free_frame(void *upage);
unsigned frames_hash(const struct hash_elem *elem, void *aux);
bool frame_less(const struct hash_elem *a, const struct hash_elem *b, void *aux);
struct frame* frame_lookup(void *addr);

void 
frame_init(void)
{
    hash_init(&frames, frames_hash, frame_less, NULL);
    lock_init(&frames_lock);
}

void *
get_frame(void *upage, enum palloc_flags flags)
{
    void *page = palloc_get_page(flags);

    // Adds frame to table if page is available
    if (page == NULL) {
        PANIC("frames are full\n");
        // need to worry if swap is full
        //evict_frame();
        //page = palloc_get_page(flags);
    }

    struct frame *f = (struct frame *) malloc(sizeof(struct frame));
    f->addr = page;
    f->upage = upage;
    f->thread = thread_current();
    lock_acquire(&frames_lock);
    hash_insert(&frames, &f->elem);
    lock_release(&frames_lock);
    return page;
}

bool
free_frame(void *addr)
{
    struct frame *f = frame_lookup(addr);

    if (f != NULL) {
        lock_acquire(&frames_lock);
        hash_delete(&frames, &f->elem);
        lock_release(&frames_lock);
        palloc_free_page(f->addr);
        free(f);
        return true;
    } else {
        return false;
    }
}


/*
bool
evict_frame()
{
    // just evict the first frame that shows up
    size_t index;
    struct hash_iterator i;
    hash_first(&i, &frames);
    printf("1\n");
    struct frame *cur_frame = hash_entry(hash_cur(&i), struct frame, elem);
    printf("numbers %d %d\n", (int) cur_frame->addr, cur_frame->upage);
    
    printf("more stuff\n");
    struct sup_page_table *spt = get_sup_page_table_by_thread(cur_frame->upage, 
                                                            cur_frame->thread);
    printf("3\n");
    if (spt->should_swap) {
        printf("4\n");
        spt->swap_index = swap_out(cur_frame->addr);
        free_frame(cur_frame);
    } else {
        printf("5\n");
        free_frame(cur_frame);
    }

    printf("6\n");
    return true;
}
*/


/*
void 
scan_table(void *upage, enum palloc_flags flags, bool evict_dirty)
{
    int num_emptied = 0;
    int num_dirty_evicted = 0;
    bool is_clean;
    bool is_accessed;
    struct hash_iterator i;
    struct frame *cur_frame;
    hash_first(&i, &frames);
    lock_acquire(&frames_lock);
    while (hash_next(&i)) {
        cur_frame = hash_entry(hash_cur(&i), struct frame, elem);
        is_clean = pagedir_is_dirty(cur_frame->thread->pagedir,
                        cur_frame->upage);
        is_accessed = pagedir_is_accessed(cur_frame->thread->pagedir,
                cur_frame->upage);
        if (is_clean && !is_accessed) {
            free_frame(cur_frame->upage);
        } else if (is_accessed) {
            pagedir_set_accessed(cur_frame->thread->pagedir, cur_frame->upage, false);
        }
    }
    lock_release(&frames_lock);

    if (num_emptied > 0) || num_dirty_evicted
*/

struct frame*
frame_lookup(void *addr)
{
    struct frame f;
    struct hash_elem *e;

    f.addr = addr;
    e = hash_find(&frames, &f.elem);

    if (e != NULL) {
        return hash_entry(e, struct frame, elem);
    } else {
        return NULL;
    }
}
    


// Order frames by physical address
bool
frame_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
    struct frame *a_frame = hash_entry(a, struct frame, elem);
    struct frame *b_frame = hash_entry(b, struct frame, elem);

    return (a_frame->addr) < (b_frame->addr);
}

// We hash the physical address since it must be
// unique
unsigned 
frames_hash(const struct hash_elem *elem, void *aux UNUSED)
{
    struct frame *f = hash_entry(elem, struct frame, elem);

    return hash_int((unsigned) f->addr);
}

