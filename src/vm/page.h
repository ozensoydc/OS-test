#ifndef PAGE_H
#define PAGE_H

#include <filesys/off_t.h>

struct sup_page_table {
    // should I include the thread?
    struct file *file;
    off_t ofs;
    uint8_t *upage;
    uint32_t read_bytes;
    uint32_t zero_bytes;
    bool writable;
    struct hash_elem elem;
};

bool create_sup_page_table(struct file *file, off_t ofs, uint8_t *upage,
        uint32_t read_bytes, uint32_t zero_bytes, bool writable);
bool load_page(struct sup_page_table *spt);
struct sup_page_table* get_sup_page_table(uint8_t *upage);
bool sup_page_table_less(const struct hash_elem *a, const struct hash_elem *b, void *aux);
unsigned sup_page_table_hash(const struct hash_elem *elem, void *aux);


#endif