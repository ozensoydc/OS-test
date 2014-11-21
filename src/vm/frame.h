#include "threads/synch.h"

/*scan lock*/
struct lock scan_lock;

/* A physical frame */
struct frame{
  struct lock lock;
  void* base;
  /*page*/
}

/*lets use a hash table for frame table */
