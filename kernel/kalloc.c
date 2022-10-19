// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

struct{
  struct spinlock locks[(PHYSTOP-KERNBASE)/PGSIZE];
  int counter[(PHYSTOP-KERNBASE)/PGSIZE];
} refcnt;

inline uint64 getindex(uint64 pa){
  return (pa-KERNBASE)/PGSIZE;
}

void addn_ref(uint64 pa,int n){
  uint64 index = getindex(pa);
  acquire(&refcnt.locks[index]);
  refcnt.counter[index]+=n;
  release(&refcnt.locks[index]);
}

void setn_ref(uint64 pa,int n){
  uint64 index = getindex(pa);
  acquire(&refcnt.locks[index]);
  refcnt.counter[index]=n;
  release(&refcnt.locks[index]);
}

// inline getcounter(uint64 pa){
//   uint64 index = getindex(pa);
//   acquire(&refcnt.locks[index]);
//   int ret = refcnt.counter[index];
//   release(&refcnt.locks[index]);
//   return ret;
// }

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int index = getindex((uint64)pa);
  // 这里得这样来
  acquire(&refcnt.locks[index]);
  if(refcnt.counter[index]>1){
    refcnt.counter[index]--;
    release(&refcnt.locks[index]);
    return;
  }


  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  refcnt.counter[index]=0;
  release(&refcnt.locks[index]);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk

  if(r)
    setn_ref((uint64)r,1);
  return (void*)r;
}

int cowcopy(uint64 va,pagetable_t p){
  va = PGROUNDDOWN(va);
  // pagetable_t p = myproc()->pagetable;
  pte_t* pte = walk(p,va,0);
  if(pte==0)
    return -1;
  
  uint64 pa = PTE2PA(*pte);

  uint flags = PTE_FLAGS(*pte);

  if(!(flags & PTE_COW))
  {// { {printf("not cow");
    return -1;
  }
  int idx = getindex(pa);
  acquire(&refcnt.locks[idx]);
  if(refcnt.counter[idx]>1){
    char* mem = kalloc();
    if(mem==0)
    {
      release(&refcnt.locks[idx]);
      // printf("kalloc failed");
      return -1;
    }
    memmove(mem,(char*)pa,PGSIZE);
    if(mappages(p,va,PGSIZE,(uint64)mem,(flags & (~PTE_COW))|PTE_W)<0)
    {
      kfree(mem);
      release(&refcnt.locks[idx]);
      // printf("mappages failed");
      return -1;
    }
    refcnt.counter[idx]--;
  }else{
    *pte |= PTE_W;
    *pte &= ~PTE_COW;
  }
  release(&refcnt.locks[idx]);
  return 0;
}