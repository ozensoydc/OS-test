#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/loader.h"
#include "vm/frame.h"


static struct frame* frames /*array of frames aka frame temple basic*/
static size_t frame_cnt /*number of frames*/
//static struct lock scan_lock;
static size_t hand; /*no idea what this is*/


/*maps user memory to frames*/
void frame_init(void){
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
struct frame* get_free_frame(void){
  int i;
  struct frame* ret_frame;
  for(i=0;i<frame_cnt;i++){
    ret_frame=frames[i];
    if(ret_frame->page == NULL){
      /*here set frame->page to page*/
      return ret_frame;
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
void scan_and_evict(void){
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
      scan_pt_and_remove(cur_frame);
    }
    else if(is_accessed){
      pagedir_set_accessed(frame->page->pd,frame->page->addr);
    }
  }
  lock_release(&scan_lock);
}
