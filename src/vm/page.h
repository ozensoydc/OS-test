#include "threads/thread.h"
#include "threads/palloc.h"
#include "vm/frame.h"
#include "vm/frame.c"
#include "devices/block.h"
#include <hash.h>

struct page{
  void *addr;                     //user virtual address
  bool read_only;                 //read-only page?
  struct thread *thread;          //Owning thread
  bool recent_access;
  /*accessed only in owning process context*/
  struct hash_elem hash_elem      //used for inserting page into page table
  
  /*to be only accessed for the owning process context with frame->frame_lock
    held.  Cleared only with with scan_lock and frame->frame_lock held.*/
  struct frame *frame;
  
  /*swap information, protected by frame->frame_lock */
  block_sector_t sector;
  
  /*memory-mapped file information protected by frame->frame_lock. 
    Not sure what these are for atm*/
  bool private;                   //False to write back to file, true to
                                  //write back to swap.
  struct file *file;              //File
  off_t file_offset;              //Offset infile
  off_t file_bytes;               //bytes to read/write, 1...PGSIZE
};

struct hash page_table;

void page_hash(struct hash_elem *hash_e, void* UNUSED);
bool page_less(struct hash_elem *hash_elem_a,
	       struct hash_elem *hash_elem_b,
	       void* UNUSED);

void init_pt(void);

bool page_in(void* fault_addr);
struct page* page_for_addr(const void *addr);
bool page_accessed_recently(struct page* p);
void* get_free_page(enum palloc_flags flag);
struct page* create_page(void* address);
void* free_page(char* name);
