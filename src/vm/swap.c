#include "devices/block.h"
#include "threads/thread.h"
#include "threads/palloc.h"
/* The swap device This is just a chunk of memory*/
static struct block *swap_device;

/*Used swap pages */
static struct bitmap *swap_bitmap;

/* protects swap_bitmap */
static struct lock swap_lock;

/* Number of sectors per page */
#define PAGE_SECTORS(PGSIZE/BLOCK_SECTOR_SIZE);

/*number of slots that can fit swap*/
#define SWAP_SLOTS;

/*I want to partition the swap into page size swap slots*/
struct swap_slot* slots;
void swap_init(void){
  swap_device = block_get_role(BLOCK_SWAP);
  
  if(swap_device == NULL){
    printf("no swap device -- swap disabled\n");
    swap_bitmap = bitmap_create(0);
  }
  else{
    swap_bitmap = bitmap_create(block_size(swap_device)/PAGESECTORS);
    SWAP_SLOTS = block_size(swap_device)/PG_SIZE;
    slots=(struct slot*)malloc(sizeof(struct slot)*SWAP_SLOTS);
    int i=0;
    /*setting all the slots empty initially*/
    for(;i<SWAP_SLOTS;i++){
      struct swap_slot cur_slot;
      cur_slot=&slots[i];
      cur_slot->base_index==i*PGSIZE;
      cur_slot->is_full=false;
    }
  }    
  if(swap_bitmap == NULL)
    PANIC("couldn't create swap bitmap");
  lock_init(&swap_lock);
}


void swap_write(struct page* page){
  /*get free swap slot*/
  lock_acquire(&swap_lock);
  int i=0;
  for(;i<SWAP_SLOTS<i++){
    if(slots[i].is_full==false){
      int bit_index=slots[i].base_index;
      block_write(swap_device,PGSIZE,page);
      slots[i].is_full=true;
      bitmap_set_multiple(swap_bitmap,base_index,PGSIZE,true);
      break;
    }
  }
  lock_release(&swap_lock);
}

void swap_clear(int index){
  lock_acquire(&swap_lock);
  slots[index].is_full=false;
  bitmap_set_multiple(swap_bitmap,slots[index].base_index,PGSIZE,false);
  lock_release(&swap_lock);
}
