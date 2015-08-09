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

#include "globals.h"
#include "errno.h"
#include "types.h"

#include "mm/mm.h"
#include "mm/tlb.h"
#include "mm/mman.h"
#include "mm/page.h"

#include "proc/proc.h"

#include "util/string.h"
#include "util/debug.h"

#include "fs/vnode.h"
#include "fs/vfs.h"
#include "fs/file.h"

#include "vm/vmmap.h"
#include "vm/mmap.h"

/*
 * This function implements the mmap(2) syscall, but only
 * supports the MAP_SHARED, MAP_PRIVATE, MAP_FIXED, and
 * MAP_ANON flags.
 *
 * Add a mapping to the current process's address space.
 * You need to do some error checking; see the ERRORS section
 * of the manpage for the problems you should anticipate.
 * After error checking most of the work of this function is
 * done by vmmap_map(), but remember to clear the TLB.
 */
int do_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off,
		void **ret) {
	/*NOT_YET_IMPLEMENTED("VM: do_mmap");*/
	vnode_t *vnode = NULL;
	vmarea_t *vma;
	int retVal = 0;
	uint32_t vlow;

	dbg(DBG_PRINT, "(GRADING3D 2)\n");
	/*Man page: EINVAL We don't like addr, length, or offset */
	if (!PAGE_ALIGNED(off)) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		return -EINVAL;
	}

	if (addr != NULL && (uint32_t) addr < USER_MEM_LOW) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		return -EINVAL;
	}

	if (len > USER_MEM_HIGH) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		return -EINVAL;
	}

	/*if (addr != NULL && len + (uint32_t) addr > USER_MEM_HIGH) {
		dbg(DBG_ERROR, "5\n");
		return -EINVAL;
	}*/

	if (len == NULL) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		return -EINVAL;
	}
	/* Man page : flags contained neither MAP_PRIVATE or MAP_SHARED, or contained both of these values.*/
	if ((!(flags & MAP_PRIVATE) && !(flags & MAP_SHARED))
			|| ((flags & MAP_PRIVATE) && (flags & MAP_SHARED))) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		return -EINVAL;
	}

	/* Errors when flag is FIXED */

	/*if (!(flags & MAP_ANON) && (flags & MAP_FIXED) && !PAGE_ALIGNED(addr)) {
		dbg(DBG_ERROR, "8\n");
		return -EINVAL;
	}*/

	if (addr == 0 && (flags & MAP_FIXED)) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		return -EINVAL;
	}

	if (!(flags & MAP_ANON)) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");

		/*MAN Page: EBADF  fd is not a valid file descriptor (and MAP_ANONYMOUS was not set).*/
		if (fd < 0 || fd > NFILES || curproc->p_files[fd] == NULL) {
			dbg(DBG_PRINT, "(GRADING3D 2)\n");
			return -EBADF;
		}

		file_t *f = curproc->p_files[fd];
		vnode = f->f_vnode;

		/*MAN page:  MAP_PRIVATE was requested, but fd  is  not  open  for  reading. */
		/*if ((flags & MAP_PRIVATE) && !(f->f_mode & FMODE_READ)) {
			dbg(DBG_ERROR, "12\n");
			return -EACCES;
		}*/
		/*MAN page: MAP_SHARED  was  requested and PROT_WRITE is set, but fd is not open in read/write (O_RDWR) mode.*/

		if ((flags & MAP_SHARED) && (prot & PROT_WRITE)
				&& !((f->f_mode & FMODE_READ) && (f->f_mode & FMODE_WRITE))) {
			dbg(DBG_PRINT, "(GRADING3D 2)\n");
			return -EACCES;
		}
		/*MAN page: MAP_SHARED  was  requested and PROT_WRITE is set, but fd is not open in read/write (O_RDWR) mode.*/

		/*if ((prot & PROT_WRITE) && (f->f_mode & FMODE_APPEND)) {
			dbg(DBG_ERROR, "14\n");
			return -EACCES;
		}*/

	}

	retVal = vmmap_map(curproc->p_vmmap, vnode, ADDR_TO_PN(addr),
			(uint32_t) PAGE_ALIGN_UP(len) / PAGE_SIZE, prot, flags, off,
			VMMAP_DIR_HILO, &vma);

	if (ret != NULL && retVal >= 0) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		*ret = PN_TO_ADDR(vma->vma_start);
		vlow = (uint32_t) PN_TO_ADDR(vma->vma_start);
		pt_unmap_range(curproc->p_pagedir, vlow,
				vlow + (uintptr_t) PAGE_ALIGN_UP(len));

		tlb_flush_range(vlow, (uint32_t) PAGE_ALIGN_UP(len) / PAGE_SIZE);
	}

	KASSERT(NULL != curproc->p_pagedir);
	dbg(DBG_PRINT, "(GRADING3A 2.a)\n");

	return retVal;
}

/*
 * This function implements the munmap(2) syscall.
 *
 * As with do_mmap() it should perform the required error checking,
 * before calling upon vmmap_remove() to do most of the work.
 * Remember to clear the TLB.
 */
int do_munmap(void *addr, size_t len) {
	/*NOT_YET_IMPLEMENTED("VM: do_munmap"); */
	int retVal = 0;
	uint32_t vlow;
	/*Man page: EINVAL We don't like addr, length, or offset */
	if ((uint32_t) addr < USER_MEM_LOW
			|| USER_MEM_HIGH - (uint32_t) addr < len) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		return -EINVAL;
	}

	/*if (!PAGE_ALIGNED(addr)) {
		dbg(DBG_ERROR, "17\n");
		return -EINVAL;
	}*/

	if (len == NULL) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		return -EINVAL;
	}
	dbg(DBG_PRINT, "(GRADING3D 2)\n");

	retVal = vmmap_remove(curproc->p_vmmap, ADDR_TO_PN(addr),
			(uint32_t) PAGE_ALIGN_UP(len) / PAGE_SIZE);
	vlow = (uint32_t) PN_TO_ADDR(ADDR_TO_PN(addr));
	tlb_flush_range(vlow, (uint32_t) PAGE_ALIGN_UP(len) / PAGE_SIZE);
	pt_unmap_range(curproc->p_pagedir, vlow,
			(uint32_t) PN_TO_ADDR(
					ADDR_TO_PN(addr) + (uint32_t) PAGE_ALIGN_UP(len) / PAGE_SIZE));

	KASSERT(NULL != curproc->p_pagedir);
	dbg(DBG_PRINT, "(GRADING3A 2.b)\n");

	return retVal;
}

