#include "vm/page.h"
#include <hash.h>
#include "threads/thread.h"
#include "vm/frame.h"
#include "threads/synch.h"
#include <stdio.h>
static struct lock hash_lock; /*writes to page table should be locked while 
				another read or write is happening*/

void init_pt(void){
  hash_init(&page_table, page_hash, page_less,NULL);
  lock_init(&hash_lock);
}

unsigned page_hash(const struct hash_elem *hash_e, void* aux){
  struct page *p = hash_entry(hash_e,struct page,hash_elem);
  return hash_bytes(&p->addr, sizeof p->addr);
}

bool page_less(const struct hash_elem* hash_elem_a,
	       const struct hash_elem* hash_elem_b,
	       void* aux){
  struct page *a = hash_entry(hash_elem_a,struct page,hash_elem);
  struct page *b = hash_entry(hash_elem_b,struct page,hash_elem);
  return a->addr < b->addr;
}

/*static void scan_pages_and_destroy(struct frame* fr){
  printf("In scan_pages_and_destroy\n");
  struct hash_iterator i;
  lock_acquire(&hash_lock);
  struct page* cur_page;
  struct hash_elem* cur_elem;
  while((cur_elem=hash_next(&i))!=NULL){
    cur_page = hash_entry(cur_elem, struct page, hash_elem);
    if(cur_page->frame==fr){
      /*destroy this page*/
/* lock_acquire(&frame_lock);
      cur_page->frame == NULL;
      lock_release(&frame_lock);
      /*remove from supp pt*/
      
// hash_destroy(&page_table,hash_elem);
      /*destroy page completely*/
      //pagedir_destroy(cur_page->thread->pd);
      /*move page to swap*/
      
      //free(cur_page);
/*  }
  }
  printf("exitting scan_pages_and_destroy\n");
  lock_release(&hash_lock);
  }*/

/* returns true if page with given address is in page table*/

bool page_in(void* fault_addr){
  printf("in page_in\n");
  struct hash_iterator i;
  lock_acquire(&hash_lock);
  struct page* cur_page;
  hash_first(&i,&page_table);
  while(hash_next(&i)!=NULL){
    cur_page = hash_entry(hash_cur(&i), struct page, hash_elem);
    if(cur_page->addr == fault_addr){
      lock_release(&hash_lock);
      printf("page found\n");
      return true;
    }
  }
  printf("no page found\n");
  return false;
}

/*returns the page to the given address*/

struct page* page_for_addr(const void *addr){
  /*if(!page_in(addr)){
    printf("page is not in table\n");
    return NULL;
    }*/
  printf("in page_for_addr/n");
  struct hash_iterator i;
  lock_acquire(&hash_lock);
  hash_first(&i,&page_table);
  struct page* cur_page;
  
  while(hash_next(&i)!=NULL){
    cur_page=hash_entry(hash_cur(&i),struct page, hash_elem);
    if(cur_page->addr == addr){
      printf("successfully return from page_for_addr\n");
      return cur_page;//hash_entry(cur_elem, struct page, hash_elem);
    }
  }
  printf("page not fount\n");
  
  return NULL;
}

bool page_accessed_recently(struct page* p){
  return pagedir_is_accessed(p->thread->pagedir);
}



/* get a free page using paloc_get_page, andget a free frame from
   frame table */

void* get_free_page(enum palloc_flags flag){
  void* page_vaddr=(void*)malloc(sizeof(void*));
  page_vaddr=palloc_get_page(flag);
  struct page* page=create_page(page_vaddr);
  lock_acquire(&hash_lock);
  hash_insert(&page_table, &page->hash_elem);
  lock_release(&hash_lock);
  get_frame(page_vaddr, flag);
  return page_vaddr;
}

/*create page for vaddr*/

struct page* create_page(void* address){
  struct page* page=(struct page*)malloc(sizeof(struct page));
  if(page==NULL){
    PANIC("not enough space to allocate new page\n");
  }
  page->addr=address;
  page->thread=thread_current();
  page->recent_access=1;
  //struct frame* fr=get_frame();
  //fr->page=page;
  lock_acquire(&hash_lock);
  hash_insert(&page_table, &page->hash_elem);
  lock_release(&hash_lock);
  return page;
}

void free_page(void* addr){
  //void* vaddr=(void*) page_name;
  //struct hash_iterator i;
  struct page* cur_page = page_for_addr(addr);
  lock_acquire(&hash_lock);
  hash_delete(&page_table,&cur_page->hash_elem);
  lock_release(&hash_lock);
  return;
  //hash_first(&i,&page_table);
  //struct hash_elem* cur_elem;
  /*while(hash_next(&i)!=NULL){
    cur_page = hash_entry(hash_cur(&i), struct page, hash_elem);
    if(cur_page->addr == vaddr){
  
      printf("found page\n");
      //palloc_free_page(cur_page->addr);
      cur_page->frame->upage == NULL;
      //pagedir_destroy(cur_page->thread->pd);
      free(cur_page);
      return;
    }
  */
  //printf("page to be freed not found\n");
}

  /*
void destroy_pd(char* name){
  void* vaddr=(void*) page_name;
  struct hash_iterator i;
  lock_acquire(&hash_lock);
  struct page cur_page;
  struct hash_elem* cur_elem;
  while((cur_elem=hash_next(&i))!=NULL){
    cur_page = hash_entry(cur_elem, struct page, hash_elem);
    if(cur_page->addr == vaddr){
      hash_delete(&page_table,hash_elem);
      lock_release(&hash_lock);
      printf("found page\n");
      palloc_free_page(cur_page->addr);
      cur_page->frame->page==NULL;
      pagedir_destroy(cur_page->thread->pd);
      free(cur_page);
      return;
    }
    
    printf("page to be freed not found\n");
}


  */
/*
static void page_destroy(struct hash_elem *hash_e, void* UNUSED){
  hash_destroy(&page_table,hash_e);
}
*/
