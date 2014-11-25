#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/input.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "threads/malloc.h"
//#define USER_VADDR_BOTTOM ((void *) 0x0804000)

static void syscall_handler (struct intr_frame *);
const void *user_to_kernel_pointer(const void *vaddr);
static inline bool get_user(uint8_t *dst, const uint8_t *usrc);
static char *copy_in_string(const char *us);
void check_valid_buffer(void *buffer, unsigned size);
void check_valid_pointer(const void *vaddr);
void exit(int status);
tid_t exec(const char *cmd_line);
int write(int fd, const void *buffer, unsigned size);
int wait(tid_t tid);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned size);
void halt(void);
void seek(int fd, unsigned position);
unsigned tell(int fd);
int open(const char *file);
void close(int fd);
mapid_t mmap(int fd, void *addr);
void munmap(mapid_t mapping);

struct lock filesys_lock;
void get_args(struct intr_frame *f, int *arg, int n);

void
syscall_init (void) 
{
    lock_init(&filesys_lock);
    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void// 
syscall_handler (struct intr_frame *f) 
{
  int args[3];
  // The f-esp is void *, stack memory location, so dereference at location to get value
  switch (* (int *) f->esp)
    {
    case SYS_HALT:
      {
	halt();
	break;
      }
    case SYS_EXIT:
      {
	//printf("exiting\n");
	get_args(f, args, 1);
	f->eax = args[0];
	exit(args[0]);
	break;
      }
    case SYS_EXEC:
      {
	f->eax = 10;
	get_args(f, args, 1);
	args[0] = user_to_kernel_pointer((const void *) args[0]);
	f->eax = exec((const char *) args[0]);
	break;
      }
    case SYS_WRITE:
      {
	//printf("writing file\n");
	get_args(f, args, 3);
	check_valid_buffer((void *) args[1], (unsigned) args[2]);
	const void *test = user_to_kernel_pointer((const void *) args[1]);
	f->eax = write(args[0], test, (unsigned) args[2]);
	break;
      }
    case SYS_WAIT:
      {
	get_args(f, args, 1);
	f->eax = wait(args[0]);
	break;
      }
    case SYS_CREATE:
      {
	//printf("creating file\n");
	get_args(f, args, 2);
	args[0] = user_to_kernel_pointer((const void *) args[0]);
	f->eax = create((const char *) args[0], (unsigned) args[1]);
	break;
      }
    case SYS_REMOVE:
      {
	//printf("removing file\n");
	get_args(f, args, 1);
	args[0] = user_to_kernel_pointer((const void *) args[0]);
	f->eax = remove((const char *) args[0]);
      }
    case SYS_OPEN:
      {
	//printf("opening file\n");
	get_args(f, args, 1);
	args[0] = user_to_kernel_pointer((const void *) args[0]);
	f->eax = open((const char *) args[0]);
	break;
      }
    case SYS_FILESIZE:
      {
	//printf("checking filesize\n");
	get_args(f, args, 1);
	f->eax = filesize(args[0]);
	break;
      }
    case SYS_READ:
      {
	//printf("reading file\n");
	get_args(f, args, 3);
	check_valid_buffer((void *) args[1], (unsigned) args[2]);
	args[1] = user_to_kernel_pointer((const void *) args[1]);
	f->eax = read(args[0], (void *) args[1], (unsigned) args[2]);
	break;
      }
    case SYS_SEEK:
      {
	//printf("seeking file\n");
	get_args(f, args, 2);
	seek(args[0], (unsigned) args[1]);
	break;
      }
    case SYS_TELL:
      {
	//printf("telling file\n");
	get_args(f, args, 1);
	seek(args[0], (unsigned) args[1]);
	break;
      }
    case SYS_CLOSE:
      {
	//printf("closing file\n");
	get_args(f, args, 1);
	close(args[0]);
	break;
      }
    case SYS_MMAP:
      {
	get_args(f,args,2);
	f->eax = mmap((int)args[0],(void*)args[1]);
	//printf("returned %d\n",f->eax);
	break;
      }
    case SYS_MUNMAP:
      {
	get_args(f,args,1);
	f->eax = args[0];
	munmap((int)args[0]);
	break;
      }
    }
}

mapid_t mmap(int fd, void *addr){
  //printf("in mmap fd is %d, addr is %d\n",fd,addr);
  size_t file_size;// = filesize(fd);
  struct thread *t = thread_current();
  struct file_handle *fh = thread_get_fh(&t->files, fd);
  
  if (fh == NULL) {
    //printf("no file pointed to by descriptor going to return -1\n");
    return -1;
  }
  
  file_size=file_length(fh->file);

  //printf("file size is %d\n", file_size);
  if(file_size == 0){
    printf("file_size 0\n");
    return -1;
  }
  else if(pg_ofs(addr)!=0){
    //printf("not page aligned\n");
    return -1;
  }
  else if(get_sup_page_table(addr)!=NULL){
    //printf("address already mapped by other process\n");
    return -1;
  }
  else if(addr==0 || fd==1 || fd==0){
    //printf("invalid addr, or fd\n");
    return -1;
  }
  fh->uaddr=addr;
  //printf("going to malloc buf\n");
  
  int num_pages = file_size/PGSIZE;
  if(file_size%PGSIZE>0){
    num_pages++;
  }
  int i=0;
  int offset=0;
  int mmap_fd = thread_add_mmap_file(file_reopen(fh->file));
  
  struct file_handle * mmap_fh = thread_get_fh(&t->mmap_files,mmap_fd);
  mmap_fh->uaddr=addr;
  //printf("about to enter the for loop. num_pages: %d, mmap_fd: %d\n, PGSIZE: %d", num_pages, mmap_fd, PGSIZE);
  for(;i<num_pages;i++){
    offset=i*PGSIZE;
    lock_acquire(&t->pgdir_lock);
    //printf("about to pgdir\n");
    void *pgdir = pagedir_get_page(t->pagedir,addr+offset);
    lock_release(&t->pgdir_lock);
    
    if(pgdir != NULL){
      printf("overlap\n");
      return -1;
    }
    //printf("about to enter create_sup_page_table\n");
    if(i!=(num_pages-1)){
	create_sup_page_table(mmap_fh->file,offset,addr+offset,PGSIZE,0,true);
    }
    if(i==num_pages-1){
      int zeroes=PGSIZE - file_size%PGSIZE;
	create_sup_page_table(mmap_fh->file,offset,addr+offset,
			      file_size%PGSIZE,zeroes,true);
    }
    //printf("sup_page_table created\n");
  }
  return mmap_fd;
  
}

void munmap(mapid_t mapping){
  printf("in munmap mapid_t: %d\n",mapping);
  struct thread* t = thread_current();
  struct file_handle* fh = thread_get_fh(&t->mmap_files, mapping);
  if(fh == NULL){
    printf("file was not found\n");
  }
  //struct frame* init_frame = frame_lookup(vtop((void*)mapping));
  /*need init frame*/
  //struct sup_page_table* init_stp = get_sup_page_table(mapping);
  int size = file_length(fh->file);
  int num_frames = size/PGSIZE;
  if(size%PGSIZE>0){
    num_frames++;
  }
  //printf("about to enter for loop\n");
  int i=0;
  int offset=0;
  //char *buf=(char*)malloc(sizeof(char)*PGSIZE);
  bool dirty;
  printf("num_frames: %d\n",num_frames);
  void* addr = fh->uaddr;
  for(;i<num_frames;i++){
    void* mem_addr;
    mem_addr=addr + i*PGSIZE;
    dirty = pagedir_is_dirty(t->pagedir,mem_addr);
    
    void * cur_page=pagedir_get_page(t->pagedir,mem_addr);
    printf("see if page is dirty\n");
    
    if(dirty){
      printf("page is dirty\n");
      file_seek(fh->file,i*PGSIZE);
      printf("about to write\n");
      lock_acquire(&filesys_lock);
      
      if(i<num_frames-1){
	file_write(fh->file, mem_addr,PGSIZE);
      }
      else{
	file_write(fh->file, mem_addr,size%PGSIZE);
      }
      lock_release(&filesys_lock);
    }
    
    lock_acquire(&t->pgdir_lock);
    pagedir_clear_page(t->pagedir, fh->uaddr+i*PGSIZE);
    lock_release(&t->pgdir_lock);
    
  }
  
  list_remove(&fh->elem);
  file_close(fh->file);
  free(fh);
}

void
halt(void)
{
    shutdown_power_off();
}

void
exit(int status)
{
    printf("%s: exit(%d)\n", thread_current()->name, status);
    thread_current()->ret_status = status;
    thread_exit();
}

tid_t
exec(const char *cmd_line)
{
    lock_acquire(&filesys_lock);
    //printf("locked in exec\n");
    tid_t pid = process_execute(cmd_line);
    lock_release(&filesys_lock);
    //printf("released in exec\n");
    return pid;
}

int
wait(tid_t tid)
{
    return process_wait(tid);
}

int
write(int fd, const void *buffer, unsigned size)
{
    if (fd == STDOUT_FILENO) {
        lock_acquire(&filesys_lock);
        putbuf(buffer, size);
        lock_release(&filesys_lock);
        return size;
    } else {
        struct thread *t = thread_current();
        uint8_t *buf = buffer;  // NEEDED???

        struct file_handle *fh = thread_get_fh(&t->files, fd);

        if (fh == NULL) {
            exit(-1);
        }

        lock_acquire(&filesys_lock);
        off_t wrote = file_write(fh->file, buf, size);
        lock_release(&filesys_lock);

        return wrote;
    }
}
        
void
seek(int fd, unsigned position)
{
  struct thread *t = thread_current();
  struct file_handle *fh = thread_get_fh(&t->files, fd);
  
  if (fh == NULL) {
    exit(-1);
  }
  
  lock_acquire(&filesys_lock);
  file_seek(fh->file, position);
  lock_release(&filesys_lock);
}

bool
create(const char *file, unsigned initial_size)
{
    lock_acquire(&filesys_lock);
    bool i = filesys_create(file, initial_size);
    lock_release(&filesys_lock);
    return i;
}

bool
remove(const char *file)
{
    lock_acquire(&filesys_lock);
    bool i = filesys_remove(file);
    lock_release(&filesys_lock);
    return i;
}

int
filesize(int fd)
{
  struct thread *t = thread_current();
  struct file_handle *fh = thread_get_fh(&t->files, fd);
  
  if (fh == NULL) {
    exit(-1);
  }
  
  return file_length(fh->file);
}

int
read(int fd, void *buffer, unsigned size)
{
    struct thread *t = thread_current();
    if (fd == 0) {
        unsigned i;
        uint8_t *buf = buffer;

        lock_acquire(&filesys_lock);
        for (i = 0; i < size; i++) {
            buf[i] = input_getc();
        }
        lock_release(&filesys_lock);
        return size;
    } else {
        uint8_t *buf = buffer;

        struct file_handle *fh = thread_get_fh(&t->files, fd);

        if (fh == NULL) {
            exit(-1);
        }

        lock_acquire(&filesys_lock);
        off_t read = file_read(fh->file, buf, size);
        lock_release(&filesys_lock);
 
        return read;
    }
}

unsigned
tell(int fd)
{
    struct thread *t = thread_current();
    struct file_handle *fh = thread_get_fh(&t->files, fd);
    
    if (fh == NULL) {
        exit(-1);
    }

    lock_acquire(&filesys_lock);
    off_t position = file_tell(fh->file);
    lock_release(&filesys_lock);
    return position;
}

void
get_args(struct intr_frame *f, int *arg, int n)
{
    int i;
    int *ptr;

    for (i = 0; i < n; i++) {
        //printf("getting arg %d\n %x\n", i, ((int *)f->esp + i + 1));
        check_valid_pointer((int *) f->esp + i + 1);
        ptr = (int *) f->esp + i + 1;
        arg[i] = *ptr;
    }
}

int
open(const char *file)
{
    lock_acquire(&filesys_lock);
    struct file *f = filesys_open(file);
    lock_release(&filesys_lock);

    int fd;
    if (f == NULL) {
        fd = -1;
    } else {
        fd = thread_add_fd(f);
    }

    return fd;
}

void 
check_valid_buffer(void *buffer, unsigned size)
{
    unsigned i;
    char *local_buffer = (char *) buffer;
    for (i = 0; i < size; i++) {
        check_valid_pointer((const void *) local_buffer);
        local_buffer++;
    }
}

void
close(int fd)
{
    struct thread *t = thread_current();
    struct file_handle *fh = thread_get_fh(&t->files, fd);

    if (fh == NULL) {
        exit(-1);
    }

    lock_acquire(&filesys_lock);
    file_close(fh->file);
    thread_remove_file(fh);
    lock_release(&filesys_lock);
}

void
check_valid_pointer(const void *vaddr)
{
  if (!is_user_vaddr(vaddr) || !pagedir_get_page(thread_current()->pagedir, vaddr) ) {
        exit(-1);
    }
}

const void *
user_to_kernel_pointer(const void *vaddr)
{
    check_valid_pointer(vaddr);
    void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);

    if (!ptr) {
        exit(-1);
    }
    //printf("done converting\n");
    return  ptr;
}
