#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
    int args[3];
    switch (* (int *) f->esp)
    {
        case SYS_EXIT:
            {
                f->eax = 1;
                exit(1);
                break;
            }
        case SYS_WRITE:
            {
                get_args(f, args, 3);
                args[1] = pagedir_get_page(thread_current()->pagedir, (const void*) args[1]);
                f->eax = write(args[0], (const void *) args[1], (unsigned) args[2]);
                break;
            }
        case SYS_WAIT:
            {
                get_args(f, args, 1);
                f->eax = wait(args[0]);
                break;
            }
    }
}

void
exit(int status)
{
    printf("%s: exit(%d)\n", thread_current()->name, status);
    thread_exit();
}

int
write (int fd, const void *buffer, unsigned size)
{
    struct thread *t = thread_current();
    if (fd == STDOUT_FILENO) {
        putbuf(buffer, size);
        return size;
    }
}
    
int
wait(tid_t tid)
{
    return process_wait(tid);
}

void
get_args(struct intr_frame *f, int *arg, int n)
{
    int i;
    int *ptr;

    for (i = 0; i < n; i++) {
        ptr = (int *) f->esp + i + 1;
        arg[i] = *ptr;
    }
}
