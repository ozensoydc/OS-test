#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "threads/synch.h"
#include <hash.h>
#include <stdbool.h>
#include <stdio.h>
static struct hash frames;
//static struct lock frames_lock;
void frame_init(void);
void* get_frame(void *upage, enum palloc_flags flags);
bool free_frame(void *upage);
unsigned frames_hash(const struct hash_elem *elem, void *aux);
bool frame_less(const struct hash_elem *a, const struct hash_elem *b, 
		void *aux);
struct frame* frame_lookup(void *addr);

void 
frame_init(void)
{
  //printf("in frame init\n");
  hash_init(&frames, frames_hash, frame_less, NULL);
  lock_init(&frames_lock);
  //printf("exit frame init\n");
  
}

int scan_table(void* upage, enum palloc_flags flags){
  
  
  int num_emptied=0;
  bool is_clean;
  bool is_accessed;
  struct hash_iterator i;
  struct frame* cur_frame;
  hash_first(&i,&frames);
  struct page* cur_page;
  printf("acquired frames_lock\n");
  lock_acquire(&frames_lock);
  while(hash_next(&i))
    {
      
      cur_frame = hash_entry(hash_cur(&i), struct page, hash_elem);
      cur_page = page_for_addr(cur_frame->upage);
      is_clean = pagedir_is_dirty(cur_page->thread->pagedir,
				  cur_frame->upage);
      is_accessed = pagedir_is_accessed(cur_page->thread->pagedir,
					cur_frame->upage);
      if(is_clean && is_accessed){
	//lock_release(&frames_lock);
	free_frame(cur_page->frame);
	free_page(cur_page->addr);
	num_emptied++;
	//PANIC("eviction_time");
      }
      else if(is_accessed){
	//lock_release(&frames_lock);
	pagedir_set_accessed(cur_page->thread->pagedir, cur_frame->upage,true);
      }
      
    }
  printf("released frames_lock\n");
  lock_release(&frames_lock);
  if(num_emptied>0){
    
    get_frame(upage, flags);
  }
  else{
    
    scan_table(upage,flags);
  }
  
  return num_emptied;
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
        struct page* page=create_page(upage);
	page->frame=f;
	f->page=page;
	lock_acquire(&frames_lock);
        hash_insert(&frames, &f->elem);
        lock_release(&frames_lock);
        return page;
    } else {
      printf("going to evict some frames\n");
      scan_table(upage, flags);
    }
}

bool
free_frame(void *addr)
{
    struct frame *f = frame_lookup(addr);

    if (f != NULL) {
      free_page(f->upage);
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

	/*#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/loader.h"
#include "vm/frame.h"


static struct frame* frames /*array of frames aka frame temple basic*/
	//static int frame_cnt /*number of frames*/
//static struct lock scan_lock;
	//static size_t hand; /*no idea what this is*/


/*maps user memory to frames*/
	/*void frame_init(void){
  void* base;
  lock_init(&scan_lock);
  frames = malloc(sizeof *frames * init_ram_pages); 
  if(frames == NULL){
    PANIC("out of memory allocating page frames");
  }
  while((base = palloc_get_page(PAL_USER))!= NULL){
    struct frame *f = &frames[frame_cnt++];
    lock_init(&f->lock);
    f->base = base;
    f->page = NULL;
  }
}


/*make this function take a page as an argument*/
	/*struct frame* get_free_frame(void){
  int i;
  struct frame* ret_frame;
  for(i=0;i<frame_cnt;i++){
    //ret_frame=frames[i];
    if(frames[i].page == NULL){
      //ret_frame->page
	
      return &frames[i];
    }
  }
  PANIC("no empty frame gotta evict some frames yo");
}

/*want to implement 2 strikes eviction. Algorithm explained:
  go over the frame list, check for each frame wether it is dirty, and recently
  accessed.
  If it is clean and not recently accessed->evict
  if it is dirty and not recently accessed -> do nothing
  if it is dirty and recently accessed -> set recently accessed to 0
  if it is clean and recently accessed -> set recently accessed to 0
*/
	/*void scan_and_evict(void){
  lock_acquire(&scan_lock);
  int i=0;
  struct frame* cur_frame;
  bool is_clean;
  bool is_accessed;
  for(;i<frame_cnt;i++){
    cur_frame=frame[i];
    is_clean=pagedir_is_dirty(frame->page->pd,frame->page->addr);
    is_accessed=pagedir_is_accessed(frame->page->pd,frame->page->addr);
    if(is_clean&&!is_accessed){
      printf("evict this bitch\n");
      /*remove references to the frame from any existing page table*/
/* scan_pt_and_remove(cur_frame);
    }
    else if(is_accessed){
      pagedir_set_accessed(frame->page->pd,frame->page->addr);
    }
  }
  lock_release(&scan_lock);
  }*/
