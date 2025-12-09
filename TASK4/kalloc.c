#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

#ifndef PGSHIFT
#define PGSHIFT 12
#endif

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
  uchar ref_count[PHYSTOP >> PGSHIFT];
} kmem;

void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE) {
    kmem.ref_count[V2P(p) >> PGSHIFT] = 1;
    kfree(p);
  }
}


void
inc_ref(uint pa)
{
  if(pa >= PHYSTOP || pa < V2P(end))
    panic("inc_ref");

  if(kmem.use_lock)
    acquire(&kmem.lock);

  kmem.ref_count[pa >> PGSHIFT]++;

  if(kmem.use_lock)
    release(&kmem.lock);
}

int
dec_ref(uint pa)
{
  if(pa >= PHYSTOP || pa < V2P(end))
    panic("dec_ref");

  int count;

  if(kmem.use_lock)
    acquire(&kmem.lock);

  if(kmem.ref_count[pa >> PGSHIFT] > 0)
    kmem.ref_count[pa >> PGSHIFT]--;

  count = kmem.ref_count[pa >> PGSHIFT];

  if(kmem.use_lock)
    release(&kmem.lock);

  return count;
}

int
get_ref(uint pa)
{
  if(pa >= PHYSTOP || pa < V2P(end))
    return -1;

  int count;
  if(kmem.use_lock)
    acquire(&kmem.lock);

  count = kmem.ref_count[pa >> PGSHIFT];

  if(kmem.use_lock)
    release(&kmem.lock);

  return count;
}
// ------------------------------------------------

void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");
  if(dec_ref(V2P(v)) > 0)
    return;

  memset(v, 1, PGSIZE);

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;
  if(kmem.use_lock)
    release(&kmem.lock);
}

char*
kalloc(void)
{
  struct run *r;

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
    kmem.ref_count[V2P((char*)r) >> PGSHIFT] = 1;
  }
  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}
