#include "vm/page.h"
#include <hash.h>
#include "threads/thread.h"
#include "vm/frames.h"
#include "threads/synch.h"

unsigned page_hash(struct hash_elem *hash_elem, void* UNUSED){
  struct page *p = 
}
