/******************************************************************************/
/* Important Spring 2015 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "types.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/string.h"

#include "proc/proc.h"
#include "proc/kthread.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/pframe.h"
#include "mm/mmobj.h"
#include "mm/pagetable.h"
#include "mm/tlb.h"

#include "fs/file.h"
#include "fs/vnode.h"

#include "vm/shadow.h"
#include "vm/vmmap.h"

#include "api/exec.h"

#include "main/interrupt.h"

/* Pushes the appropriate things onto the kernel stack of a newly forked thread
 * so that it can begin execution in userland_entry.
 * regs: registers the new thread should have on execution
 * kstack: location of the new thread's kernel stack
 * Returns the new stack pointer on success. */
static uint32_t fork_setup_stack(const regs_t *regs, void *kstack) {
	/* Pointer argument and dummy return address, and userland dummy return
	 * address */
	uint32_t esp = ((uint32_t) kstack) + DEFAULT_STACK_SIZE
			- (sizeof(regs_t) + 12);
	*(void **) (esp + 4) = (void *) (esp + 8); /* Set the argument to point to location of struct on stack */
	memcpy((void *) (esp + 8), regs, sizeof(regs_t)); /* Copy over struct */
	return esp;
}

/*
 * The implementation of shadow_destroy.
 * This routine cleans up the shadow objects.
 * We decrement the new ref count and mmobj is reset to old mmobj and its count.
 */
static void shadow_destroy(list_t *parent_vma, list_t *child_vma) {
	list_link_t *parent, *child;

	parent = parent_vma->l_next;
	child = child_vma->l_next;

	while (parent != parent_vma) {
		vmarea_t *pvma, *cvma;
		pvma = list_item(parent, vmarea_t, vma_plink);
		cvma = list_item(child, vmarea_t, vma_plink);
		if (cvma->vma_obj == NULL) {
			break;
		}
		if ((pvma->vma_flags & MAP_TYPE) == MAP_PRIVATE) {
			mmobj_t *parent_mmo = pvma->vma_obj->mmo_shadowed;
			parent_mmo->mmo_ops->ref(parent_mmo);
			pvma->vma_obj->mmo_ops->put(pvma->vma_obj);
			pvma->vma_obj = parent_mmo;
		}

		parent = parent->l_next;
		child = child->l_next;
	}
}

static void init_shadow(vmarea_t *vma, mmobj_t *shadow) {
	mmobj_t *bottom;
	bottom = mmobj_bottom_obj(vma->vma_obj);
	bottom->mmo_ops->ref(bottom);
	shadow->mmo_un.mmo_bottom_obj = bottom;
	shadow->mmo_shadowed = vma->vma_obj;

	if (list_link_is_linked(&vma->vma_olink)) {
		list_remove(&vma->vma_olink);
	}
	list_insert_tail(&bottom->mmo_un.mmo_vmas, &vma->vma_olink);

	vma->vma_obj = shadow;
}

/*
 * The implementation of fork(2). Once this works,
 * you're practically home free. This is what the
 * entirety of Weenix has been leading up to.
 * Go forth and conquer.
 */
int do_fork(struct regs *regs) {
	/*NOT_YET_IMPLEMENTED("VM: do_fork");*/
    KASSERT(regs != NULL);
    dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

    KASSERT(curproc != NULL);
    dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

    KASSERT(curproc->p_state == PROC_RUNNING);
    dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

	proc_t *child_pcb = proc_create("child process");
	/*
	if (child_pcb == NULL) {
		dbg(DBG_ERROR, "(GRADING3A 1)\n");
		curthr->kt_errno = -ENOMEM;
		return -1;
	}*/

	KASSERT(child_pcb->p_state == PROC_RUNNING);
	dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
	KASSERT(child_pcb->p_pagedir != NULL);
	dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
	/*Copy address space from parent to child */
	/*This just copies single level VMA, so later we do the linked lists copy */
	vmmap_t *child_vmap = vmmap_clone(curproc->p_vmmap);

	int retVal = 0;
	if (NULL != child_vmap) {
		dbg(DBG_PRINT, "(GRADING3F)\n");
		list_t *parent_vmm, *child_vmm;
		list_link_t *parent_vma, *child_vma;

		parent_vmm = &curproc->p_vmmap->vmm_list;
		child_vmap->vmm_proc = child_pcb;
		child_vmm = &child_vmap->vmm_list;
		parent_vma = parent_vmm->l_next;
		child_vma = child_vmm->l_next;

		/*Copy VMA linked lists from parent to child */
		while (parent_vma != parent_vmm) {
			dbg(DBG_PRINT, "(GRADING3F)\n");
			/*Want new items every time, so declare again */
			vmarea_t *pvma, *cvma;
			pvma = list_item(parent_vma, vmarea_t, vma_plink);
			cvma = list_item(child_vma, vmarea_t, vma_plink);

			cvma->vma_obj = pvma->vma_obj;

			/* TODO : Verify if this is needed? */
			cvma->vma_obj->mmo_ops->ref(cvma->vma_obj);

			int map_type = pvma->vma_flags & MAP_TYPE;
			if (map_type == MAP_PRIVATE) {
				dbg(DBG_PRINT, "(GRADING3F)\n");
				mmobj_t *p_shadow = shadow_create();
				if (p_shadow != NULL) {
					dbg(DBG_PRINT, "(GRADING3F)\n");
					mmobj_t *c_shadow = shadow_create();
					if (c_shadow != NULL) {
						dbg(DBG_PRINT, "(GRADING3F)\n");
						p_shadow->mmo_ops->ref(p_shadow);
						c_shadow->mmo_ops->ref(c_shadow);
						init_shadow(pvma,p_shadow);
						init_shadow(cvma,c_shadow);
					} /*else {
						dbg(DBG_ERROR, "(GRADING3A 7)\n");
						retVal = -ENOSPC;
					}*/

				} /*else {
					dbg(DBG_ERROR, "(GRADING3A 8)\n");
					retVal = -ENOSPC;
				}*/


				/*if (retVal < 0) {
					dbg(DBG_ERROR, "(GRADING3A 9)\n");
					cvma->vma_obj->mmo_ops->put(cvma->vma_obj);
					cvma->vma_obj = NULL;
					break;
				}*/
			}
			parent_vma = parent_vma->l_next;
			child_vma = child_vma->l_next;
		}

		/*if (retVal) {
			dbg(DBG_ERROR, "(GRADING3A 10)\n");
			shadow_destroy(parent_vmm, child_vmm);
			vmmap_destroy(child_vmap);
			retVal = -ENOMEM;
		} else*/ {
			dbg(DBG_PRINT, "(GRADING3F)\n");
			vmmap_destroy(child_pcb->p_vmmap);
			child_pcb->p_vmmap = child_vmap;
			/*retVal = 0;*/
		}

	} /*else if (child_vmap == NULL) {
		dbg(DBG_ERROR, "(GRADING3A 12)\n");
		retVal = -ENOMEM;
	}*/
	/* Error during clone, clear up new child mess*/
	/*if (retVal) {
		dbg(DBG_ERROR, "(GRADING3A 13)\n");
		list_remove(&child_pcb->p_list_link);
		list_remove(&child_pcb->p_child_link);
		pt_destroy_pagedir(child_pcb->p_pagedir);
		vput(child_pcb->p_cwd);
		vmmap_destroy(child_pcb->p_vmmap);
		curthr->kt_errno = -ENOMEM;
		return -1;
	}*/

	/* Create a new child thread and setup its stack */
	kthread_t *childthr = kthread_clone(curthr);

	/*if (childthr == NULL) {
		dbg(DBG_ERROR, "(GRADING3A 14)\n");
		shadow_destroy(&curproc->p_vmmap->vmm_list,
				&child_pcb->p_vmmap->vmm_list);
		list_remove(&child_pcb->p_list_link);
		list_remove(&child_pcb->p_child_link);
		pt_destroy_pagedir(child_pcb->p_pagedir);
		vput(child_pcb->p_cwd);
		vmmap_destroy(child_pcb->p_vmmap);
		curthr->kt_errno = -ENOMEM;
		return -1;
	}*/

	childthr->kt_proc = child_pcb;
	KASSERT(childthr->kt_kstack != NULL);
	dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
	list_insert_tail(&child_pcb->p_threads, &childthr->kt_plink);

	/* Child gets 0 return status on fork success */
	regs->r_eax = 0;
	childthr->kt_ctx.c_pdptr = child_pcb->p_pagedir;
	childthr->kt_ctx.c_eip = (uint32_t) userland_entry;
	childthr->kt_ctx.c_esp = fork_setup_stack(regs, childthr->kt_kstack);
	childthr->kt_ctx.c_kstack = (uintptr_t) childthr->kt_kstack;
	childthr->kt_ctx.c_kstacksz = DEFAULT_STACK_SIZE;

	/* Copy Parent's FDT */
	int i;
	for (i = 0; i < NFILES; i++) {
		dbg(DBG_PRINT, "(GRADING3F)\n");
		child_pcb->p_files[i] = curproc->p_files[i];
		if (child_pcb->p_files[i] != NULL) {
			dbg(DBG_PRINT, "(GRADING3F)\n");
			fref(child_pcb->p_files[i]);
		}
	}

	tlb_flush_all();
	pt_unmap_range(curproc->p_pagedir, USER_MEM_LOW, USER_MEM_HIGH);

	child_pcb->p_brk = curproc->p_brk;
	child_pcb->p_start_brk = curproc->p_start_brk;

	sched_make_runnable(childthr);

	regs->r_eax = child_pcb->p_pid;

	return child_pcb->p_pid;

}
