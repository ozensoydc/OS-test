#include "threads/synch.h"

/*scan lock*/
struct lock scan_lock;

/* A physical frame */
struct frame {
  struct lock lock;
  void* base;
  struct page* page;
  /*page*/
};

/*lets use a hash table for frame table */

void frame_init(void);
struct frame* get_free_frame(void);
void scan_and_evict(void);
