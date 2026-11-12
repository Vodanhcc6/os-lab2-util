#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#ifdef PGTBL_SOL
#include "riscv.h"
#endif
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > UTOP)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_vmprint(void)
{
  struct proc *p;

  p = myproc();
  vmprint(p->pagetable);
  return 0;
}
#endif

#ifdef LAB_PGTBL
uint64
sys_pgaccess(void)
{
  // A byte array supports the lab's 256-page test without large shifts.
  // Bound both the scan time and the kernel stack buffer (512 bytes).
  enum { MAX_PGACCESS_PAGES = 4096 };
  uchar mask[MAX_PGACCESS_PAGES / 8];
  struct proc *p = myproc();
  uint64 start, dst;
  int npages;

  argaddr(0, &start);
  argint(1, &npages);
  argaddr(2, &dst);
  if (npages <= 0 || npages > MAX_PGACCESS_PAGES || start >= MAXVA)
    return -1;

  // malloc() need not return a page-aligned pointer. Scan the page
  // containing start, then the next npages-1 pages.
  start = PGROUNDDOWN(start);
  if ((uint64)npages > (MAXVA - start) / PGSIZE)
    return -1;
  uint64 nbytes = (npages + 7) / 8;
  if (dst >= MAXVA || nbytes > MAXVA - dst)
    return -1;

  // Validate the entire output first, including a possible page boundary.
  // copyout() can lazily allocate; this syscall instead rejects holes.
  for (uint64 va = PGROUNDDOWN(dst); va < dst + nbytes; va += PGSIZE) {
    pte_t *pte = walk(p->pagetable, va, 0);
    if (pte == 0 || (*pte & (PTE_V | PTE_U | PTE_R | PTE_W)) !=
                        (PTE_V | PTE_U | PTE_R | PTE_W))
      return -1;
  }

  memset(mask, 0, nbytes);
  for (int i = 0; i < npages; i++) {
    pte_t *pte = walk(p->pagetable, start + (uint64)i * PGSIZE, 0);
    if (pte == 0 || (*pte & (PTE_V | PTE_U)) != (PTE_V | PTE_U) ||
        !PTE_LEAF(*pte))
      return -1;
    if (*pte & PTE_A)
      mask[i / 8] |= 1U << (i % 8);
  }

  // Do not consume the sample if validation or copying fails. xv6 has
  // no user threads sharing this page table while this syscall runs.
  if (copyout(p->pagetable, p->sz, dst, (char *)mask, nbytes) < 0)
    return -1;
  for (int i = 0; i < npages; i++) {
    pte_t *pte = walk(p->pagetable, start + (uint64)i * PGSIZE, 0);
    *pte &= ~PTE_A;
  }
  // Discard translations that could otherwise retain the old A state.
  sfence_vma();
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

#ifdef LAB_LOCK
uint64
sys_cpupin(void)
{
  struct proc *p = myproc();
  int cpu;

  argint(0, &cpu);
  if (cpu < 0 || cpu >= NCPU)
    return -1;
  acquire(&p->lock);
  p->pincpu = &cpus[cpu];
  release(&p->lock);
  return 0;
}
#endif
