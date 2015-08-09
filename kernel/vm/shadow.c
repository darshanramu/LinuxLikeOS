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

#include "util/string.h"
#include "util/debug.h"

#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "mm/slab.h"
#include "mm/tlb.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/shadowd.h"

#define SHADOW_SINGLETON_THRESHOLD 5

int shadow_count = 0; /* for debugging/verification purposes */
#ifdef __SHADOWD__
/*
 * number of shadow objects with a single parent, that is another shadow
 * object in the shadow objects tree(singletons)
 */
static int shadow_singleton_count = 0;
#endif

static slab_allocator_t *shadow_allocator;

static void shadow_ref(mmobj_t *o);
static void shadow_put(mmobj_t *o);
static int shadow_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite,
		pframe_t **pf);
static int shadow_fillpage(mmobj_t *o, pframe_t *pf);
static int shadow_dirtypage(mmobj_t *o, pframe_t *pf);
static int shadow_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t shadow_mmobj_ops = { .ref = shadow_ref, .put = shadow_put,
		.lookuppage = shadow_lookuppage, .fillpage = shadow_fillpage,
		.dirtypage = shadow_dirtypage, .cleanpage = shadow_cleanpage };

/*
 * This function is called at boot time to initialize the
 * shadow page sub system. Currently it only initializes the
 * shadow_allocator object.
 */
void shadow_init() {
	/*NOT_YET_IMPLEMENTED("VM: shadow_init");*/
	shadow_allocator = slab_allocator_create("shadow", sizeof(mmobj_t));

	KASSERT(shadow_allocator);
	dbg(DBG_PRINT, "(GRADING3A 6.a)\n");
}

/*
 * You'll want to use the shadow_allocator to allocate the mmobj to
 * return, then then initialize it. Take a look in mm/mmobj.h for
 * macros which can be of use here. Make sure your initial
 * reference count is correct.
 */
mmobj_t *
shadow_create() {
	/*NOT_YET_IMPLEMENTED("VM: shadow_create");*/
	mmobj_t * shadowobj = (mmobj_t *) slab_obj_alloc(shadow_allocator);
	dbg(DBG_PRINT, "(GRADING3D 2)\n");
	if (shadowobj != NULL) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		mmobj_init(shadowobj, &shadow_mmobj_ops);
		shadowobj->mmo_refcount++;
	}
	return shadowobj;
}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void shadow_ref(mmobj_t *o) {
	/*NOT_YET_IMPLEMENTED("VM: shadow_ref");*/
	KASSERT(o && (0 < o->mmo_refcount) && (&shadow_mmobj_ops == o->mmo_ops));
	dbg(DBG_PRINT, "(GRADING3A 6.b)\n");

	o->mmo_refcount++;
}

/*
 * Decrement the reference count on the object. If, however, the
 * reference count on the object reaches the number of resident
 * pages of the object, we can conclude that the object is no
 * longer in use and, since it is a shadow object, it will never
 * be used again. You should unpin and uncache all of the object's
 * pages and then free the object itself.
 */
static void shadow_put(mmobj_t *o) {
	/* NOT_YET_IMPLEMENTED("VM: shadow_put");*/
	pframe_t *pframe;
	KASSERT(o && (0 < o->mmo_refcount) && (&shadow_mmobj_ops == o->mmo_ops));
	dbg(DBG_PRINT, "(GRADING3A 6.c)\n");

	if (o->mmo_refcount == o->mmo_nrespages + 2) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		if (!(list_empty(&(o->mmo_respages)))) {
			dbg(DBG_PRINT, "(GRADING3D 2)\n");
			list_iterate_begin(&(o->mmo_respages), pframe, pframe_t,pf_olink)
						{
							dbg(DBG_PRINT, "(GRADING3D 2)\n");;
							if (pframe_is_pinned(pframe)) {
								dbg(DBG_PRINT, "(GRADING3D 2)\n");
								pframe_unpin(pframe);
							}
							pframe_free(pframe);
						}list_iterate_end();
			slab_obj_free(shadow_allocator, o);
		}
	} else {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		o->mmo_refcount--;
	}

}

/* This function looks up the given page in this shadow object. The
 * forwrite argument is true if the page is being looked up for
 * writing, false if it is being looked up for reading. This function
 * must handle all do-not-copy-on-not-write magic (i.e. when forwrite
 * is false find the first shadow object in the chain which has the
 * given page resident). copy-on-write magic (necessary when forwrite
 * is true) is handled in shadow_fillpage, not here. It is important to
 * use iteration rather than recursion here as a recursive implementation
 * can overflow the kernel stack when looking down a long shadow chain */
static int shadow_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite,
		pframe_t **pf) {
	/* NOT_YET_IMPLEMENTED("VM: shadow_lookuppage");*/
	pframe_t *newpf;
	mmobj_t *mmobj;
	int result = 0;
	dbg(DBG_PRINT, "(GRADING3D 2)\n");
	if (!forwrite) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		for (newpf = NULL, mmobj = o;
				newpf == NULL && mmobj->mmo_shadowed != NULL;
				mmobj = mmobj->mmo_shadowed) {
			dbg(DBG_PRINT, "(GRADING3D 2)\n");
			newpf = pframe_get_resident(mmobj, pagenum);

		}

		if (newpf != NULL) {
			dbg(DBG_PRINT, "(GRADING3D 2)\n");
			*pf = newpf;
		} else {
			dbg(DBG_PRINT, "(GRADING3D 2)\n");
			result = pframe_lookup(mmobj, pagenum, 0, pf);
			return result;
		}
	} else {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		result = pframe_get(o, pagenum, pf);
		return result;
	}

	return result;
}

/* As per the specification in mmobj.h, fill the page frame starting
 * at address pf->pf_addr with the contents of the page identified by
 * pf->pf_obj and pf->pf_pagenum. This function handles all
 * copy-on-write magic (i.e. if there is a shadow object which has
 * data for the pf->pf_pagenum-th page then we should take that data,
 * if no such shadow object exists we need to follow the chain of
 * shadow objects all the way to the bottom object and take the data
 * for the pf->pf_pagenum-th page from the last object in the chain).
 * It is important to use iteration rather than recursion here as a
 * recursive implementation can overflow the kernel stack when
 * looking down a long shadow chain */
static int shadow_fillpage(mmobj_t *o, pframe_t *pf) {
	/*NOT_YET_IMPLEMENTED("VM: shadow_fillpage");*/
	KASSERT(pframe_is_busy(pf));
	dbg(DBG_PRINT, "(GRADING3A 6.d)\n");

	KASSERT(!pframe_is_pinned(pf));
	dbg(DBG_PRINT, "(GRADING3A 6.d)\n");

	pframe_t *newpf;
	mmobj_t *mmobj;
	int result = 0;

	for (newpf = NULL, mmobj = o->mmo_shadowed;
			newpf == NULL && mmobj != o->mmo_un.mmo_bottom_obj;
			mmobj = mmobj->mmo_shadowed) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		newpf = pframe_get_resident(mmobj, pf->pf_pagenum);

	}

	if (newpf == NULL) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		if ((result = pframe_lookup(mmobj, pf->pf_pagenum, 1, &newpf)) < 0) {

			return result;
		}
	}
	if (!pframe_is_pinned(pf)) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		pframe_pin(pf);
	}
	memcpy(pf->pf_addr, newpf->pf_addr, PAGE_SIZE);

	return result;
}

/* These next two functions are not difficult. */

static int shadow_dirtypage(mmobj_t *o, pframe_t *pf) {
	/* NOT_YET_IMPLEMENTED("VM: shadow_dirtypage");*/
	dbg(DBG_PRINT, "(GRADING3D 2)\n");
	shadow_cleanpage(o,pf);
	return 0;
}

static int shadow_cleanpage(mmobj_t *o, pframe_t *pf) {
	/*NOT_YET_IMPLEMENTED("VM: shadow_cleanpage");*/
	dbg(DBG_PRINT, "(GRADING3D 2)\n");
	return 0;
}
