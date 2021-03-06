#ifndef FRAME_H
#define FRAME_H

#include <hash.h>
#include "threads/palloc.h"

struct frame {
    void *addr; // Physical address of page
    void *upage; // Virtual address of page
    struct thread *thread;  // Thread that frame belongs to
    struct hash_elem elem;
};


// Basic frame fuctions
void frame_init(void);
void* get_frame(void *upage, enum palloc_flags flags);
bool free_frame(void *addr);

#endif

/*
#include "threads/synch.h"

/*scan lock*/
	//struct lock scan_lock;

/* A physical frame 
struct frame {
  struct lock lock;
  void* base;
  struct page* page;
  /*page*/
	//};

/*lets use a hash table for frame table */

/*
struct frame* get_free_frame(void);
void scan_and_evict(void);
=======*/
