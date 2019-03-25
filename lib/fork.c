// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	
//	pte_t pte = uvpt [(uintptr_t)addr >> PGSHIFT];

        if (!((err & FEC_WR) && (uvpd[PDX(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_COW)))
        {
                panic("pagedault is not cauased by a wire operation. Err: %x \n", err);
        }

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	int perm = PTE_W | PTE_P | PTE_U;
	addr = ROUNDDOWN(addr, PGSIZE);
	int PTE_NOCOW = PTE_SYSCALL & (~PTE_COW);
	sys_page_alloc(0, PFTEMP, PTE_W | PTE_P | PTE_U);
	memcpy(PFTEMP, addr, PGSIZE);
/*	if ((r = sys_page_alloc(0, PFTEMP, perm)) < 0)
	{
		panic ("page Allocation at Utemp failed. Err: %x \n", r);
	}
*/	
//	memmove (PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE);

//	memcpy(PFTEMP, ROUNDDOWN(addr,PGSIZE), PGSIZE);
	
	sys_page_map(0, PFTEMP, 0, addr, (uvpt[PGNUM(addr)] & PTE_NOCOW) | PTE_W);
	/*if ((r = sys_page_map (0, PFTEMP, 0, ROUNDDOWN(addr, PGSIZE), perm)) < 0)
	{
		panic ("Error in mapping. Err: %x \n", r);
	}*/
	/*if ((r = sys_page_unmap(0, PFTEMP)) < 0) {
		panic("Error in Unmapping. Err: %e", r);
	}*/	 
	sys_page_unmap(0, PFTEMP);
	return;
//	panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	
	 if ((uvpt[pn] & PTE_COW) || (uvpt[pn] & PTE_W))
	{
		sys_page_map(0, PGADDR(0, pn, 0), envid, PGADDR(0, pn, 0),((uvpt[pn] & PTE_SYSCALL) & (~PTE_W)) | PTE_COW);
		sys_page_map(0, PGADDR(0, pn, 0),0, PGADDR(0, pn, 0), ((uvpt[pn] & PTE_SYSCALL) & (~PTE_W)) | PTE_COW);
	} else {
		sys_page_map(0, PGADDR(0, pn, 0),envid, PGADDR(0, pn, 0),(uvpt[pn] & PTE_SYSCALL));
	}
	
/*	int r;

	// LAB 4: Your code here.

	pte_t pte = uvpt [pn];
	
	if (!(pte & PTE_W) || !(pte & PTE_COW))
	{
		if ((r = sys_page_map (thisenv->env_id, (void *) (pn*PGSIZE), envid, (void *)(pn*PGSIZE), pte & PTE_SYSCALL)) < 0)
		{
			panic("Page mapping Failed. ERR: %x \n", r);
		} 
	return 0;
	}
	
	pte &= ~PTE_W;
	pte |= PTE_COW;

	if ((r = sys_page_map (thisenv->env_id, (void *) (pn*PGSIZE), envid, (void *)(pn*PGSIZE), pte & PTE_SYSCALL)) < 0)
	{
		panic("Page mapping Failed. ERR: %x \n", r);
	}

	if ((r = sys_page_map (thisenv->env_id, (void *) (pn*PGSIZE), thisenv->env_id, (void *)(pn*PGSIZE), pte & PTE_SYSCALL)) < 0)
	{
		panic("Page mapping Failed. ERR: %x \n", r);
	} 
*/
//	panic("duppage not implemented");
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
/*
	int r;
	int perm = PTE_P | PTE_U | PTE_W;
	uint32_t addr;

	set_pgfault_handler(pgfault);
	
	envid_t envid = sys_exofork();
	if (envid < 0) {
		panic("sys_exofork failed. Err: %e", envid);
	}
	if (envid == 0) {  // child
		thisenv = &envs[ENVX(sys_getenvid())];
		return envid;
	}

	for (addr = 0; addr < USTACKTOP; addr += PGSIZE)
	{
		if ((uvpd[PDX(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_U))
		{
			duppage (envid, PGNUM(addr));
		} 
	}

	if ((r = sys_page_alloc (envid, (void *) (UXSTACKTOP - PGSIZE), perm)) < 0)
	{
		panic("sys_page_alloc failed. Err: %e", r);
	}
	
	extern void _pgfault_upcall ();
	sys_env_set_pgfault_upcall (envid, _pgfault_upcall);

	if ((r = sys_env_set_status (envid, ENV_RUNNABLE)) < 0)
	{
		 panic("sys_page_alloc failed. Err: %e", r);

	}
	 
	return envid;
*/
set_pgfault_handler(pgfault);
   envid_t child_envid = sys_exofork();
   if (child_envid < 0)
   {
       panic("fork failed");
   }
   if (child_envid == 0)
   {
      set_pgfault_handler(pgfault);
       thisenv = &envs[ENVX(sys_getenvid())];
       return 0;
   }
  int i;
   for (i = 0; i < USTACKTOP; i += PGSIZE)
   {
       if ((uvpd[PDX(i)] & PTE_P) &&
           (uvpt[PGNUM(i)] & PTE_P) &&
           (uvpt[PGNUM(i)] & PTE_U))
       {
          duppage(child_envid, PGNUM(i));
      }
   }
   extern void _pgfault_upcall();
   sys_page_alloc(child_envid, (void *)(0xeebff000), PTE_U|PTE_W|PTE_P);
   sys_env_set_pgfault_upcall(child_envid, _pgfault_upcall);
   sys_env_set_status(child_envid, ENV_RUNNABLE);
   return child_envid;

//	panic("fork not implemented");

}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
