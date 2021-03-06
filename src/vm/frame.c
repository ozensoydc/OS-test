#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "vm/frame.h"
#include "threads/synch.h"
#include <hash.h>
#include <stdbool.h>

static struct hash frames;
static struct lock frames_lock;

void scan_table(void* upage, enum palloc_flags flags,bool evict_dirty);
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
    if (page != NULL) {
        struct frame *f = (struct frame *) malloc(sizeof(struct frame));
        f->addr = page;
        f->upage = upage;
        f->thread = thread_current();
        lock_acquire(&frames_lock);
        hash_insert(&frames, &f->elem);
        lock_release(&frames_lock);
        return page;
    } else {
      //PANIC("no frames left\n");
      scan_table(upage,flags,false);
    }
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


void scan_table(void* upage, enum palloc_flags flags,bool evict_dirty){
  printf("in scan table\n");
  int num_emptied = 0;
  int num_dirty_evicted=0;
  bool is_clean;
  bool is_accessed;
  struct hash_iterator i;
  struct frame* cur_frame;
  hash_first(&i,&frames);
  lock_acquire(&frames_lock);
  while(hash_next(&i)){
    cur_frame = hash_entry(hash_cur(&i), struct frame, elem);
    is_clean = pagedir_is_dirty(cur_frame->thread->pagedir,
				cur_frame->upage);
    is_accessed = pagedir_is_dirty(cur_frame->thread->pagedir,
				   cur_frame->upage);
    if(is_clean && !is_accessed){
      pagedir_clear_page(cur_frame->thread->pagedir,cur_frame->upage);
      free_frame(cur_frame->upage);
    }
    else if(!is_clean && !is_accessed && (num_emptied == 0) && evict_dirty){
      pagedir_clear_page(cur_frame->thread->pagedir,cur_frame->upage);
      free_frame(cur_frame->upage);
      printf("need to evict to swap\n");
    }
    else if(is_accessed){
      pagedir_set_accessed(cur_frame->thread->pagedir, cur_frame->upage,false);
    }
  }
  lock_release(&frames_lock);
  if(num_emptied>0 || num_dirty_evicted>0){
    get_frame(upage,flags);
  }
  else{
    scan_table(upage,flags,true);
  }
  
}
