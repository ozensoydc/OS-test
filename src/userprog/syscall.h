#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdint.h>

typedef uint8_t* mapid_t;
void syscall_init (void);
#define USER_VADDR_BOTTOM ((void *) 0x0804000)
#endif /* userprog/syscall.h */
