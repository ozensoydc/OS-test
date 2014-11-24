#include <hash.h>
#include "vm/page.h"
#include <threads/thread.h>
#include "threads/malloc.h"
#include "filesys/off_t.h"
#include "filesys/file.h"
#include <string.h>
#include "userprog/process.h"
#include "vm/frame.h"
#include "threads/palloc.h"
#include "userprog/process.h"
#include <stdio.h>
#include "threads/vaddr.h"


bool
create_sup_page_table(struct file *file, off_t ofs, uint8_t *upage, 
        uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
    struct thread *t = thread_current();
    struct sup_page_table *spt = (struct sup_page_table *) malloc(sizeof(struct sup_page_table));

    spt->file = file;
    spt->ofs = ofs;
    spt->upage = upage;
    spt->read_bytes = read_bytes;
    spt->zero_bytes = zero_bytes;
    spt->writable = writable;

    // does this need to be protected
    if (spt != NULL) {
        hash_insert(&t->sup_page_tables, &spt->elem);
        return true;
    } else {
        return false;
    }
}

bool
load_page(struct sup_page_table *spt)
{
    uint8_t *kpage = get_frame(spt->upage, PAL_USER);
    if (kpage == NULL)
        return false;

    // Lock filesystem?
    if (file_read_at(spt->file, kpage, spt->read_bytes, spt->ofs) != 
            (int) spt->read_bytes) {
        free_frame(kpage);
        return false;
    }
    memset(kpage + spt->read_bytes, 0, spt->zero_bytes);

    if (!install_page(spt->upage, kpage, spt->writable)) {
        free_frame(kpage);
        return false;
    }
    return true;
    
}

struct sup_page_table*
get_sup_page_table(uint8_t *upage)
{
  struct sup_page_table spt;
  struct hash_elem *e;
  struct hash *sup_page_tables = &thread_current()->sup_page_tables;
  
  spt.upage = pg_round_down(upage);
  e = hash_find(sup_page_tables, &spt.elem);
  
  if (e != NULL) {
    return hash_entry(e, struct sup_page_table, elem);
  } else {
    return NULL;
  }
}

bool 
sup_page_table_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
  struct sup_page_table *a_table = hash_entry(a, struct sup_page_table, elem);
  struct sup_page_table *b_table = hash_entry(b, struct sup_page_table, elem);
  
  return (a_table->upage) < (b_table->upage);
}

unsigned
sup_page_table_hash(const struct hash_elem *elem, void *aux UNUSED)
{
  struct sup_page_table *spt = hash_entry(elem, struct sup_page_table, elem);
  
  return hash_int((unsigned) spt->upage);
}




