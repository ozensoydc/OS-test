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
