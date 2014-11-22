#include "vm/page.h"
#include <hash.h>
#include "threads/thread.h"
#include "vm/frames.h"
#include "threads/synch.h"

static struct lock hash_lock; /*writes to page table should be locked while 
				another read or write is happening*/

void init_pt(void){
  hash_init(&page_table, page_hash, page_less,NULL);
  lock_init(&hash_lock);
}

unsigned page_hash(struct hash_elem *hash_e, void* UNUSED){
  struct page *p = hash_entry(hash_e,struct page,hash_elem);
  return hash_bytes(&p->addr, sizeof p->addr);
}

bool page_less(struct hash_elem* hash_elem_a,
	       struct hash_elem* hash_elem_b,
	       void* UNUSED){
  struct page *a = hash_entry(hash_elem_a,struct page,hash_elem);
  struct page *b = hash_entry(hash_elem_b,struct page,hash_elem);
  return a->addr < b->addr;
}

static void scan_pages_and_destroy(struct frame* fr){
  struct hash_iterator i;
  lock_acquire(&hash_lock);
  struct page* cur_page;
  struct hash_elem* cur_elem;
  while((cur_elem=hash_next(&i))!=NULL){
    cur_page = hash_entry(cur_elem, struct page, hash_elem);
    if(cur_page->frame==fr){
      /*destroy this page*/
      lock_acquire(&fr->frame_lock);
      cur_page->frame == NULL;
      lock_release(&fr->frame_lock);
      /*remove from supp pt*/
      
      hash_destroy(&page_table,hash_e);
      /*destroy page completely*/
      pagedir_destroy(cur_page->thread->pd);
      /*move page to swap*/
      
      //free(cur_page);
    }
  }
  lock_release(&hash_lock);
}

/* returns true if page with given address is in page table*/

bool page_in(void* fault_addr){
  struct hash_iterator i;
  lock_acquire(&hash_lock);
  struct page cur_page;
  struct hash_elem* cur_elem;
  while((cur_elem=hash_next(&i))!=NULL){
    cur_page = hash_entry(cur_elem, struct page, hash_elem);
    if(cur_page->addr == fault_addr){
      lock_release(&hash_lock);
      return true;
    }
  }
  return false;
}

/*returns the page to the given address*/

struct page* page_for_addr(const void *addr){
  /*if(!page_in(addr)){
    printf("page is not in table\n");
    return NULL;
    }*/
  struct hash_iterator i;
  lock_acquire(&hash_lock);
  struct page cur_page;
  struct hash_elem* cur_elem;
  while((cur_elem=hash_next(&i))!=NULL){
    cur_page=hash_entry(cur_elem,struct page, hash_elem);
    if(cur_page->addr == addr){
      return hash_entry(cur_elem, struct page, hash_elem);
    }
  }
  printf("page not fount\n");
  return NULL;
}

bool page_accessed_recently(struct page* p){
  return pagedir_is_accessed(p->thread->pd);
}




/*
static void page_destroy(struct hash_elem *hash_e, void* UNUSED){
  hash_destroy(&page_table,hash_e);
}
*/
