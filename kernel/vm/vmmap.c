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

#include "kernel.h"
#include "errno.h"
#include "globals.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "proc/proc.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/fcntl.h"
#include "fs/vfs_syscall.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/mmobj.h"

static slab_allocator_t *vmmap_allocator;
static slab_allocator_t *vmarea_allocator;

void vmmap_init(void) {
	vmmap_allocator = slab_allocator_create("vmmap", sizeof(vmmap_t));
	KASSERT(NULL != vmmap_allocator && "failed to create vmmap allocator!");
	vmarea_allocator = slab_allocator_create("vmarea", sizeof(vmarea_t));
	KASSERT(NULL != vmarea_allocator && "failed to create vmarea allocator!");
}

vmarea_t *
vmarea_alloc(void) {
	vmarea_t *newvma = (vmarea_t *) slab_obj_alloc(vmarea_allocator);
	if (newvma) {
		newvma->vma_vmmap = NULL;
	}
	return newvma;
}

void vmarea_free(vmarea_t *vma) {
	KASSERT(NULL != vma);
	slab_obj_free(vmarea_allocator, vma);
}

/* a debugging routine: dumps the mappings of the given address space. */
size_t vmmap_mapping_info(const void *vmmap, char *buf, size_t osize) {
	KASSERT(0 < osize);
	KASSERT(NULL != buf);
	KASSERT(NULL != vmmap);

	vmmap_t *map = (vmmap_t *) vmmap;
	vmarea_t *vma;
	ssize_t size = (ssize_t) osize;

	int len = snprintf(buf, size, "%21s %5s %7s %8s %10s %12s\n", "VADDR RANGE",
			"PROT", "FLAGS", "MMOBJ", "OFFSET", "VFN RANGE");

	list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
				{
					size -= len;
					buf += len;
					if (0 >= size) {
						goto end;
					}

					len =
							snprintf(buf, size,
									"%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
									vma->vma_start << PAGE_SHIFT,
									vma->vma_end << PAGE_SHIFT,
									(vma->vma_prot & PROT_READ ? 'r' : '-'),
									(vma->vma_prot & PROT_WRITE ? 'w' : '-'),
									(vma->vma_prot & PROT_EXEC ? 'x' : '-'),
									(vma->vma_flags & MAP_SHARED ?
											" SHARED" : "PRIVATE"),
									vma->vma_obj, vma->vma_off, vma->vma_start,
									vma->vma_end);
				}list_iterate_end();

	end: if (size <= 0) {
		size = osize;
		buf[osize - 1] = '\0';
	}
	/*
	 KASSERT(0 <= size);
	 if (0 == size) {
	 size++;
	 buf--;
	 buf[0] = '\0';
	 }
	 */
	return osize - size;
}

/* Create a new vmmap, which has no vmareas and does
 * not refer to a process. */
vmmap_t *
vmmap_create(void) {
	/* NOT_YET_IMPLEMENTED("VM: vmmap_create");*/
	vmmap_t *newvmm = (vmmap_t *) slab_obj_alloc(vmmap_allocator);
	dbg(DBG_PRINT, "(GRADING3F)\n");
	if (newvmm != NULL) {
		dbg(DBG_PRINT, "(GRADING3F)\n");
		newvmm->vmm_proc = NULL;
		list_init(&newvmm->vmm_list);
	}
	return newvmm;

}

/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void vmmap_destroy(vmmap_t *map) {
	/* NOT_YET_IMPLEMENTED("VM: vmmap_destroy"); */
	KASSERT(NULL != map);
	dbg(DBG_PRINT, "(GRADING3A 3.a)\n");
	vmarea_t *vma;
	list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
				{
					dbg(DBG_PRINT, "(GRADING3D 3)\n");
					if (vma->vma_obj != NULL) {
						dbg(DBG_PRINT, "(GRADING3D 4)\n");
						vma->vma_obj->mmo_ops->put(vma->vma_obj);
					}
					list_remove(&vma->vma_plink);
					if (list_link_is_linked(&vma->vma_olink)) {
						dbg(DBG_PRINT, "(GRADING3D 5)\n");
						list_remove(&vma->vma_olink);
					}
					vmarea_free(vma);
				}list_iterate_end();
	slab_obj_free(vmmap_allocator, map);
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
void vmmap_insert(vmmap_t *map, vmarea_t *newvma) {
	/*NOT_YET_IMPLEMENTED("VM: vmmap_insert"); */

	KASSERT(NULL != map && NULL != newvma);
	dbg(DBG_PRINT, "(GRADING3A 3.b)\n");
	KASSERT(NULL == newvma->vma_vmmap);
	dbg(DBG_PRINT, "(GRADING3A 3.b)\n");
	KASSERT(newvma->vma_start < newvma->vma_end);
	dbg(DBG_PRINT, "(GRADING3A 3.b)\n");
	KASSERT(
			ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start && ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end);
	dbg(DBG_PRINT, "(GRADING3A 3.b)\n");
	vmarea_t *vma, *prev = NULL;
	newvma->vma_vmmap = map;

	list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
				{
					dbg(DBG_PRINT, "(GRADING3F)\n");
					if (vma->vma_end >= newvma->vma_end) {
						dbg(DBG_PRINT, "(GRADING3F)\n");
						list_insert_before(&vma->vma_plink,
								&(newvma->vma_plink));
						return;
					}

				}list_iterate_end();

	list_insert_tail(&map->vmm_list, &newvma->vma_plink);

}

/* Find a contiguous range of free virtual pages of length npages in
 * the given address space. Returns starting vfn for the range,
 * without altering the map. Returns -1 if no such range exists.
 *
 * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
 * should find a gap as high in the address space as possible; if dir
 * is VMMAP_DIR_LOHI, the gap should be as low as possible. */
int vmmap_find_range(vmmap_t *map, uint32_t npages, int dir) {
	/* NOT_YET_IMPLEMENTED("VM: vmmap_find_range");*/
	KASSERT(NULL != map);
	dbg(DBG_PRINT, "(GRADING3A 3.c)\n");

	KASSERT(0 < npages);
	dbg(DBG_PRINT, "(GRADING3A 3.c)\n");

	vmarea_t *vma, *nvma;
	/*
	 if (list_empty(&(map->vmm_list))) {
	 dbg(DBG_ERROR, "(GRADING3 8)\n");
	 if (dir == VMMAP_DIR_HILO){
	 dbg(DBG_ERROR, "(GRADING3 9)\n");
	 return ADDR_TO_PN(USER_MEM_HIGH) - npages;
	 }
	 else{
	 dbg(DBG_ERROR, "(GRADING3 10)\n");
	 return ADDR_TO_PN(USER_MEM_LOW);
	 }

	 } else*/{
		dbg(DBG_PRINT, "(GRADING3F)\n");
		/*if (dir == VMMAP_DIR_LOHI) {
		 dbg(DBG_ERROR, "(GRADING3 12)\n");
		 if ((list_item(map->vmm_list.l_next, vmarea_t, vma_plink))->vma_start
		 - ADDR_TO_PN(USER_MEM_LOW) >= npages) {
		 dbg(DBG_ERROR, "(GRADING3 13)\n");
		 return ADDR_TO_PN(USER_MEM_LOW);
		 }

		 list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
		 {
		 dbg(DBG_ERROR, "(GRADING3 14)\n");
		 nvma = list_item(vma->vma_plink.l_next, vmarea_t,
		 vma_plink);
		 if (vma != NULL && nvma != NULL
		 && nvma->vma_start - vma->vma_end
		 >= npages) {
		 dbg(DBG_ERROR, "(GRADING3 15)\n");
		 return vma->vma_end;
		 }

		 }list_iterate_end();
		 if (ADDR_TO_PN(USER_MEM_HIGH)
		 - (list_item(map->vmm_list.l_prev, vmarea_t, vma_plink))->vma_end
		 >= npages) {
		 dbg(DBG_ERROR, "(GRADING3 16)\n");
		 return vma->vma_end;
		 }

		 }*/
		if (dir == VMMAP_DIR_HILO) {
			dbg(DBG_PRINT, "(GRADING3F)\n");
			if (ADDR_TO_PN(USER_MEM_HIGH)
					- (list_item(map->vmm_list.l_prev, vmarea_t, vma_plink))->vma_end
					>= npages) {
				dbg(DBG_PRINT, "(GRADING3F)\n");
				return ADDR_TO_PN(USER_MEM_HIGH) - npages;
			}

			vmarea_t *pvma = NULL, *cvma = NULL;
			list_link_t *cur = NULL, *prev = (&map->vmm_list)->l_prev;

			while (prev != &map->vmm_list) {
				dbg(DBG_PRINT, "(GRADING3F)\n");
				if (cur != NULL) {
					dbg(DBG_PRINT, "(GRADING3F)\n");
					cvma = list_item(cur, vmarea_t, vma_plink);
				} else {
					dbg(DBG_PRINT, "(GRADING3F)\n");
					cvma = NULL;
				}
				if (prev != NULL) {
					dbg(DBG_PRINT, "(GRADING3F)\n");
					pvma = list_item(prev, vmarea_t, vma_plink);
				} /*else {
				 dbg(DBG_ERROR, "(GRADING3 23)\n");
				 pvma = NULL;
				 }*/

				if (pvma != NULL && cvma != NULL
						&& cvma->vma_start - pvma->vma_end >= npages) {
					dbg(DBG_PRINT, "(GRADING3F)\n");
					return cvma->vma_start - npages;
				}

				cur = prev;
				prev = prev->l_prev;
			}

			if ((list_item(map->vmm_list.l_next, vmarea_t, vma_plink))->vma_start
					- ADDR_TO_PN(USER_MEM_LOW) >= npages) {
				dbg(DBG_PRINT, "(GRADING3D 3)\n");
				return (list_item(map->vmm_list.l_next, vmarea_t, vma_plink))->vma_start
						- npages;
			}
		}

	}

	return -1;
}

/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn) {
	/*NOT_YET_IMPLEMENTED("VM: vmmap_lookup");*/
	KASSERT(NULL != map);
	dbg(DBG_PRINT, "(GRADING3A 3.d)\n");
	vmarea_t *vma;
	if (!list_empty(&(map->vmm_list))) {
		dbg(DBG_PRINT, "(GRADING3F)\n");
		list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
					{
						dbg(DBG_PRINT, "(GRADING3F)\n");
						if ((vfn >= vma->vma_start) && (vfn < vma->vma_end)) {
							dbg(DBG_PRINT, "(GRADING3F)\n");
							return vma;
						}

					}list_iterate_end();
				}
	return NULL;
}

/* Allocates a new vmmap containing a new vmarea for each area in the
 * given map. The areas should have no mmobjs set yet. Returns pointer
 * to the new vmmap on success, NULL on failure. This function is
 * called when implementing fork(2). */
vmmap_t *
vmmap_clone(vmmap_t *map) {
	/*NOT_YET_IMPLEMENTED("VM: vmmap_clone");*/
	vmmap_t *new_vmmap = vmmap_create();
	vmarea_t *vma;
	list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
				{
					dbg(DBG_PRINT, "(GRADING3F)\n");
					if (NULL != vma) {
						dbg(DBG_PRINT, "(GRADING3F)\n");
						vmarea_t *new_vma = vmarea_alloc();
						new_vma->vma_obj = NULL;
						new_vma->vma_vmmap = NULL;

						new_vma->vma_start = vma->vma_start;
						new_vma->vma_end = vma->vma_end;
						new_vma->vma_off = vma->vma_off;
						new_vma->vma_prot = vma->vma_prot;
						new_vma->vma_flags = vma->vma_flags;
						list_link_init(&new_vma->vma_olink);
						list_link_init(&new_vma->vma_plink);
						/*not adding mmobj to cloned vmareas*/
						new_vma->vma_vmmap = new_vmmap;
						list_insert_tail(&new_vmmap->vmm_list,
								&new_vma->vma_plink);
					}

				}list_iterate_end();
	if (new_vmmap) {
		dbg(DBG_PRINT, "(GRADING3F)\n");
		return new_vmmap;
	} /*else {
	 dbg(DBG_ERROR, "(GRADING3 32)\n");
	 return NULL;
	 }*/
	return NULL;
}

/* Insert a mapping into the map starting at lopage for npages pages.
 * If lopage is zero, we will find a range of virtual addresses in the
 * process that is big enough, by using vmmap_find_range with the same
 * dir argument.  If lopage is non-zero and the specified region
 * contains another mapping that mapping should be unmapped.
 *
 * If file is NULL an anon mmobj will be used to create a mapping
 * of 0's.  If file is non-null that vnode's file will be mapped in
 * for the given range.  Use the vnode's mmap operation to get the
 * mmobj for the file; do not assume it is file->vn_obj. Make sure all
 * of the area's fields except for vma_obj have been set before
 * calling mmap.
 *
 * If MAP_PRIVATE is specified set up a shadow object for the mmobj.
 *
 * All of the input to this function should be valid (KASSERT!).
 * See mmap(2) for for description of legal input.
 * Note that off should be page aligned.
 *
 * Be very careful about the order operations are performed in here. Some
 * operation are impossible to undo and should be saved until there
 * is no chance of failure.
 *
 * If 'new' is non-NULL a pointer to the new vmarea_t should be stored in it.
 */
int vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,
		int prot, int flags, off_t off, int dir, vmarea_t **new) {
	/*NOT_YET_IMPLEMENTED("VM: vmmap_map");*/
	KASSERT(NULL != map);
	dbg(DBG_PRINT, "(GRADING3A 3.f)\n");

	KASSERT(0 < npages);
	dbg(DBG_PRINT, "(GRADING3A 3.f)\n");

	KASSERT(!(~(PROT_NONE | PROT_READ | PROT_WRITE | PROT_EXEC) & prot));
	dbg(DBG_PRINT, "(GRADING3A 3.f)\n");

	KASSERT((MAP_SHARED & flags) || (MAP_PRIVATE & flags));
	dbg(DBG_PRINT, "(GRADING3A 3.f)\n");

	KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_LOW) <= lopage));
	dbg(DBG_PRINT, "(GRADING3A 3.f)\n");

	KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_HIGH) >= (lopage + npages)));
	dbg(DBG_PRINT, "(GRADING3A 3.f)\n");

	KASSERT(PAGE_ALIGNED(off));
	dbg(DBG_PRINT, "(GRADING3A 3.f)\n");

	mmobj_t *mmobj = NULL;
	vmarea_t *vma = NULL;
	int start = lopage;
	if (lopage == 0) {
		dbg(DBG_PRINT, "(GRADING3F)\n");
		start = vmmap_find_range(map, npages, dir);
		if (start == -1) {
			dbg(DBG_PRINT, "(GRADING3D 3)\n");
			return -1;
		}
	}

	vma = vmarea_alloc();
	if (vma != NULL) {
		dbg(DBG_PRINT, "(GRADING3F)\n");
		vma->vma_start = start;
		vma->vma_end = start + npages;
		vma->vma_off = ADDR_TO_PN(off);
		vma->vma_prot = prot;
		vma->vma_flags = flags;
		list_link_init(&vma->vma_plink);
		list_link_init(&vma->vma_olink);
	}
	if (file) {
		dbg(DBG_PRINT, "(GRADING3F)\n");
		/* All the vma fields are set above call vnode's mmap */
		int ret_mmap = file->vn_ops->mmap(file, vma, &mmobj);
		/*if (ret_mmap < 0) {
		 dbg(DBG_ERROR, "(GRADING3 37)\n");
		 vmarea_free(vma);
		 return ret_mmap;
		 }*/
		int retVal = vmmap_remove(map, start, npages);
		/*if (retVal < 0) {
		 dbg(DBG_ERROR, "(GRADING3 38)\n");
		 return retVal;
		 }*/
		/*vma->vma_obj->mmo_ops->ref(vma->vma_obj);*/
		if (flags & MAP_PRIVATE) {
			dbg(DBG_PRINT, "(GRADING3F)\n");
			mmobj_t *shadow = shadow_create();
			if (shadow != NULL) {
				dbg(DBG_PRINT, "(GRADING3F)\n");
				shadow->mmo_shadowed = mmobj;

				/*if (mmobj->mmo_shadowed != NULL) {
				 dbg(DBG_ERROR, "(GRADING3 41)\n");
				 shadow->mmo_un.mmo_bottom_obj =
				 mmobj->mmo_un.mmo_bottom_obj;
				 } else*/if (mmobj->mmo_shadowed == NULL) {
					dbg(DBG_PRINT, "(GRADING3F)\n");
					shadow->mmo_un.mmo_bottom_obj = mmobj;
				}
				mmobj = shadow;
			}

		}
		vma->vma_obj = mmobj;
		mmobj->mmo_ops->ref(mmobj);

	} else {
		dbg(DBG_PRINT, "(GRADING3F)\n");
		/* Even if file==NULL, in MAP_PRIVATE we have to create shadow */
		mmobj = anon_create();
		int retVal = vmmap_remove(map, start, npages);
		/*if (retVal < 0) {
		 dbg(DBG_ERROR, "(GRADING3 44)\n");
		 return retVal;
		 }*/
		if (flags & MAP_PRIVATE) {
			dbg(DBG_PRINT, "(GRADING3F)\n");
			mmobj_t *shadow = shadow_create();
			if (shadow != NULL) {
				dbg(DBG_PRINT, "(GRADING3F)\n");
				shadow->mmo_shadowed = mmobj;

				/*if (mmobj->mmo_shadowed != NULL) {
				 dbg(DBG_ERROR, "(GRADING3 47)\n");
				 shadow->mmo_un.mmo_bottom_obj =
				 mmobj->mmo_un.mmo_bottom_obj;
				 } else*/if (mmobj->mmo_shadowed == NULL) {
					dbg(DBG_PRINT, "(GRADING3F)\n");
					shadow->mmo_un.mmo_bottom_obj = mmobj;
				}

				mmobj = shadow;
			}
		}

		vma->vma_obj = mmobj;
		mmobj->mmo_ops->ref(mmobj);
	}

	list_insert_tail(&(mmobj_bottom_obj(vma->vma_obj))->mmo_un.mmo_vmas,
			&vma->vma_olink);
	vmmap_insert(map, vma);
	if (new != NULL) {
		dbg(DBG_PRINT, "(GRADING3F)\n");
		*new = vma;
	}
	return 0;
}

/*
 * We have no guarantee that the region of the address space being
 * unmapped will play nicely with our list of vmareas.
 *
 * You must iterate over each vmarea that is partially or wholly covered
 * by the address range [addr ... addr+len). The vm-area will fall into one
 * of four cases, as illustrated below:
 *
 * key:
 *          [             ]   Existing VM Area
 *        *******             Region to be unmapped
 *
 * Case 1:  [   ******    ]
 * The region to be unmapped lies completely inside the vmarea. We need to
 * split the old vmarea into two vmareas. be sure to increment the
 * reference count to the file associated with the vmarea.
 *
 * Case 2:  [      *******]**
 * The region overlaps the end of the vmarea. Just shorten the length of
 * the mapping.
 *
 * Case 3: *[*****        ]
 * The region overlaps the beginning of the vmarea. Move the beginning of
 * the mapping (remember to update vma_off), and shorten its length.
 *
 * Case 4: *[*************]**
 * The region completely contains the vmarea. Remove the vmarea from the
 * list.
 */
int vmmap_remove(vmmap_t *map, uint32_t lopage, uint32_t npages) {
	/*NOT_YET_IMPLEMENTED("VM: vmmap_remove");*/
	vmarea_t *vmarea;
	if (!list_empty(&(map->vmm_list))) {
		dbg(DBG_PRINT, "(GRADING3F)\n");
		list_iterate_begin(&(map->vmm_list), vmarea, vmarea_t, vma_plink)
					{
						dbg(DBG_PRINT, "(GRADING3F)\n");
						/*case1 */
						if (lopage > vmarea->vma_start
								&& lopage + npages < vmarea->vma_end) {
							dbg(DBG_PRINT, "(GRADING3D 3)\n");
							vmarea_t * newvmarea1 = vmarea_alloc();

							newvmarea1->vma_start = lopage + npages;
							newvmarea1->vma_end = vmarea->vma_end;
							newvmarea1->vma_off = vmarea->vma_off
									+ (lopage + npages - vmarea->vma_start);
							newvmarea1->vma_prot = vmarea->vma_prot;
							newvmarea1->vma_obj = vmarea->vma_obj;
							newvmarea1->vma_flags = vmarea->vma_flags;
							if (newvmarea1->vma_obj != NULL) {
								dbg(DBG_PRINT, "(GRADING3D 3)\n");
								newvmarea1->vma_obj->mmo_ops->ref(
										newvmarea1->vma_obj);
							}

							list_link_init(&newvmarea1->vma_plink);
							list_link_init(&newvmarea1->vma_olink);
							list_insert_tail(&vmarea->vma_olink,
									&newvmarea1->vma_olink);

							vmarea->vma_end = lopage;
							vmmap_insert(map, newvmarea1);
						}/*case2 */
						else if (lopage > vmarea->vma_start
								&& lopage + npages >= vmarea->vma_end
								&& lopage <= vmarea->vma_end) {
							dbg(DBG_PRINT, "(GRADING3F)\n");
							vmarea->vma_end = lopage;
						}/*case3 */
						else if (lopage <= vmarea->vma_start
								&& lopage + npages < vmarea->vma_end
								&& lopage + npages >= vmarea->vma_start) {
							dbg(DBG_PRINT, "(GRADING3F)\n");
							/*vmarea->vma_off = vmarea->vma_off + lopage + npages
							 - vmarea->vma_start;*/
							vmarea->vma_off = vmarea->vma_off + lopage + npages
									- vmarea->vma_start;
							vmarea->vma_start = lopage + npages;
						}/*case4 */
						else if (lopage <= vmarea->vma_start
								&& lopage + npages >= vmarea->vma_end) {
							dbg(DBG_PRINT, "(GRADING3F)\n");
							list_remove(&(vmarea->vma_plink));
						}
					}list_iterate_end();
				}

	return 0;
}

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages) {
	/*NOT_YET_IMPLEMENTED("VM: vmmap_is_range_empty");*/
	uint32_t endvfn = startvfn + npages;

	KASSERT(
			(startvfn < endvfn) && (ADDR_TO_PN(USER_MEM_LOW) <= startvfn) && (ADDR_TO_PN(USER_MEM_HIGH) >= endvfn));
	dbg(DBG_PRINT, "(GRADING3A 3.e)\n");

	int retVal = 1;
	if (!list_empty(&(map->vmm_list))) {
		dbg(DBG_PRINT, "(GRADING3F)\n");
		vmarea_t *vma;
		list_iterate_begin(&(map->vmm_list), vma, vmarea_t, vma_plink)
					{
						dbg(DBG_PRINT, "(GRADING3F)\n");
						if (!(vma->vma_start >= endvfn
								|| vma->vma_end <= startvfn)) {
							dbg(DBG_PRINT, "(GRADING3D 3)\n");
							retVal = 0;
						}
					}list_iterate_end();
				}

	return retVal;
}

/* Read into 'buf' from the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do so, you will want to find the vmareas
 * to read from, then find the pframes within those vmareas corresponding
 * to the virtual addresses you want to read, and then read from the
 * physical memory that pframe points to. You should not check permissions
 * of the areas. Assume (KASSERT) that all the areas you are accessing exist.
 * Returns 0 on success, -errno on error.
 */
int vmmap_read(vmmap_t *map, const void *vaddr, void *buf, size_t count) {
	/*NOT_YET_IMPLEMENTED("VM: vmmap_read"); */
	/* TODO : errno*/
	vmarea_t *vma;
	pframe_t *pf;
	const void *curAdr = vaddr;
	size_t vfn = ADDR_TO_PN(curAdr), to_write, tmpCount = 0, chunkSize = 0;
	int retVal = 0;

	while (tmpCount < count) {
		dbg(DBG_PRINT, "(GRADING3F)\n");
		vfn = ADDR_TO_PN(curAdr);

		vma = vmmap_lookup(map, vfn);
		KASSERT(vma!=NULL);
		dbg(DBG_PRINT, "VM area is not null");
		to_write = ADDR_TO_PN(PAGE_ALIGN_UP(count-tmpCount));
		/*if (to_write > vma->vma_end - vfn) { * given pages are more than in the curr vm area size*
			dbg(DBG_ERROR, "(GRADING3 61)\n");
			to_write = vma->vma_end - vfn;
		}*/
		size_t i = 0;
		while (i < to_write) {
			dbg(DBG_PRINT, "(GRADING3F)\n");
			pframe_lookup(vma->vma_obj,
								vfn - vma->vma_start + vma->vma_off + i, 1, &pf);
			/*if ((retVal = ) < 0) {
				dbg(DBG_ERROR, "(GRADING3 63)\n");
				return retVal;
			}*/

			chunkSize = PAGE_SIZE - (int) curAdr % PAGE_SIZE;
			if (chunkSize > count - tmpCount) {
				dbg(DBG_PRINT, "(GRADING3F)\n");
				chunkSize = count - tmpCount;
			}

			memcpy((char *) buf + tmpCount,
					(char *) pf->pf_addr + (int) curAdr % PAGE_SIZE, chunkSize);

			tmpCount += chunkSize;
			curAdr = (char*) curAdr + chunkSize;
			i++;
		}

		dbg(DBG_PRINT, "pnum %d, tmpcount %d\n", i, tmpCount);

	}

	return 0;
}

/* Write from 'buf' into the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do this, you will need to find the correct
 * vmareas to write into, then find the correct pframes within those vmareas,
 * and finally write into the physical addresses that those pframes correspond
 * to. You should not check permissions of the areas you use. Assume (KASSERT)
 * that all the areas you are accessing exist. Remember to dirty pages!
 * Returns 0 on success, -errno on error.
 */
int vmmap_write(vmmap_t *map, void *vaddr, const void *buf, size_t count) {
	/*NOT_YET_IMPLEMENTED("VM: vmmap_write"); */
	/* TODO : errno*/
	vmarea_t *vma;
	pframe_t *pf;
	void *curAdr = vaddr;
	size_t vfn = ADDR_TO_PN(curAdr), to_write, tmpCount = 0, chunkSize = 0;
	int retVal = 0;

	while (tmpCount < count) {
		dbg(DBG_PRINT, "(GRADING3F)\n");
		vfn = ADDR_TO_PN(curAdr);

		vma = vmmap_lookup(map, vfn);
		KASSERT(vma!=NULL);
		dbg(DBG_PRINT, "VM area is not null");
		to_write = ADDR_TO_PN(PAGE_ALIGN_UP(count-tmpCount));
		/*if (to_write > vma->vma_end - vfn) { * given pages are more than in the curr vm area size*
			dbg(DBG_ERROR, "(GRADING3 66)\n");
			to_write = vma->vma_end - vfn;
		}*/
		size_t pageNum = 0;
		while (pageNum < to_write) {
			dbg(DBG_PRINT, "(GRADING3F)\n");
			pframe_lookup(vma->vma_obj,
								vfn - vma->vma_start + vma->vma_off + pageNum, 1, &pf);
			/*
			if ((retVal = )
					< 0) {
				dbg(DBG_ERROR, "(GRADING3 68)\n");
				return retVal;
			}*/

			chunkSize = PAGE_SIZE - (int) curAdr % PAGE_SIZE;
			if (chunkSize > count - tmpCount) {
				dbg(DBG_PRINT, "(GRADING3F)\n");
				chunkSize = count - tmpCount;
			}

			memcpy((char *) pf->pf_addr + (int) curAdr % PAGE_SIZE,
					(char *) buf + tmpCount, chunkSize);
			/*
			if ((retVal = pframe_dirty(pf)) < 0) {
				dbg(DBG_ERROR, "(GRADING3 70)\n");
				return retVal;
			}*/

			curAdr = (char*) curAdr + chunkSize;
			pageNum++;
			tmpCount += chunkSize;
		}

		dbg(DBG_PRINT, "pnum %d, tmpcount %d\n", pageNum, tmpCount);

	}

	return 0;
}
