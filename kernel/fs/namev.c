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
#include "globals.h"
#include "types.h"
#include "errno.h"

#include "util/string.h"
#include "util/printf.h"
#include "util/debug.h"

#include "fs/dirent.h"
#include "fs/fcntl.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "fs/vnode.h"

/* This takes a base 'dir', a 'name', its 'len', and a result vnode.
 * Most of the work should be done by the vnode's implementation
 * specific lookup() function, but you may want to special case
 * "." and/or ".." here depnding on your implementation.
 *
 * If dir has no lookup(), return -ENOTDIR.
 *
 * Note: returns with the vnode refcount on *result incremented.
 */
int lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result) {
	/*  NOT_YET_IMPLEMENTED("VFS: lookup"); */
	KASSERT(NULL != dir);
	dbg(DBG_PRINT, "(GRADING2A 2.a)\n");
	KASSERT(NULL != name);
	dbg(DBG_PRINT, "(GRADING2A 2.a)\n");
	KASSERT(NULL != result);
	dbg(DBG_PRINT, "(GRADING2A 2.a)\n");
	int retValue = 0;
	dbg(DBG_PRINT, "(GRADING2B)\n");
	if (len == 0) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		*result = dir;
		vref(*result);
	} else {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		retValue = dir->vn_ops->lookup(dir, name, len, result);
	}
	return retValue;

}
/* When successful this function returns data in the following "out"-arguments:
 *  o res_vnode: the vnode of the parent directory of "name"
 *  o name: the `basename' (the element of the pathname)
 *  o namelen: the length of the basename
 *
 * For example: dir_namev("/s5fs/bin/ls", &namelen, &name, NULL,
 * &res_vnode) would put 2 in namelen, "ls" in name, and a pointer to the
 * vnode corresponding to "/s5fs/bin" in res_vnode.
 *
 * The "base" argument defines where we start resolving the path from:
 * A base value of NULL means to use the process's current working directory,
 * curproc->p_cwd.  If pathname[0] == '/', ignore base and start with
 * vfs_root_vn.  dir_namev() should call lookup() to take care of resolving each
 * piece of the pathname.
 *
 * Note: A successful call to this causes vnode refcount on *res_vnode to
 * be incremented.
 */
int dir_namev(const char *pathname, size_t *namelen, const char **name,
		vnode_t *base, vnode_t **res_vnode) {
	/*NOT_YET_IMPLEMENTED("VFS: dir_namev"); */
	KASSERT(NULL != pathname);
	dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
	KASSERT(NULL != namelen);
	dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
	KASSERT(NULL != name);
	dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
	KASSERT(NULL != res_vnode);
	dbg(DBG_PRINT, "(GRADING2A 2.b)\n");

	if (*pathname == '\0'){
	   return -EINVAL;
	}

	/* first, find the end of the current dir name */
	int dir_name_start = 0;
	int next_name = 0;
        while (pathname[next_name] != '/' && pathname[next_name] != '\0'){
            next_name++;
        }
	int cur_name_len = next_name - dir_name_start;

        if (next_name - dir_name_start > NAME_LEN){
           return -ENAMETOOLONG;
        }

	vnode_t* cur_base = base;
	*name = pathname;
	int ret_lookup = 0;
	dbg(DBG_PRINT, "(GRADING2B)\n");
	if (NULL == base) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		cur_base = curproc->p_cwd;
	}

	if (pathname[0] == '/') {
		cur_base = vfs_root_vn;
		*name = pathname + 1;
		dbg(DBG_PRINT, "(GRADING2B)\n");
	}
	char chunkName[NAME_LEN];
	char *chunk = (char*) *name;
	char *delim = strchr((char *) *name, '/');
	char *lastdelim = strrchr((char *) *name, '/');
	char *nextdelim;

	if (delim != NULL) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		while (lastdelim == *name + strlen(*name) - 1) { /* case for ..// */
			*lastdelim = '\0';
			lastdelim = strrchr((char *) *name, '/');
			dbg(DBG_PRINT, "(GRADING2B)\n");
		}
		if (lastdelim != NULL) { /* case for  /.. */
			dbg(DBG_PRINT, "(GRADING2B)\n");
			while (*name != lastdelim + 1) {
				dbg(DBG_PRINT, "(GRADING2B)\n");
				chunk = (char *) *name;
				delim = strchr((char *) *name, '/');
				*name = delim + 1;
				nextdelim = strchr((char *) *name, '/');

				strncpy(chunkName, chunk, delim - chunk);
				*namelen = delim - chunk;

				while (nextdelim == delim + 1) { /* case for 1//2//3  */
					delim = strchr((char *) *name, '/');
					*name = delim + 1;
					nextdelim = strchr((char *) *name, '/');
					dbg(DBG_PRINT, "(GRADING2B)\n");

				}

				/**delim = '\0';*/

				ret_lookup = lookup(cur_base, chunkName, *namelen, res_vnode);

				if (ret_lookup != 0) {
					dbg(DBG_PRINT, "(GRADING2B)\n");
					break;
				}
				/*vref(*res_vnode);*/
				if (!S_ISDIR((*res_vnode)->vn_mode)) {
					vput(*res_vnode);
					dbg(DBG_PRINT, "(GRADING2B)\n");
					return -ENOTDIR;
				}

				KASSERT(NULL != *res_vnode);
				dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
				/*vput(*res_vnode);*/
				cur_base = *res_vnode;

			}
		} else {
			dbg(DBG_PRINT, "(GRADING2B)\n");
			*res_vnode = cur_base;
		}

		base = cur_base;
	} else {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		*res_vnode = cur_base;
	}
	if (*res_vnode != NULL && ret_lookup == 0) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		/*if((*res_vnode)->vn_mmobj.mmo_refcount ==0 ){
			(*res_vnode)->vn_mmobj.mmo_refcount++;
		}*/
		vref(*res_vnode);
	}
	*namelen = strlen(*name);
	return ret_lookup;
}

/* This returns in res_vnode the vnode requested by the other parameters.
 * It makes use of dir_namev and lookup to find the specified vnode (if it
 * exists).  flag is right out of the parameters to open(2); see
 * <weenix/fcntl.h>.  If the O_CREAT flag is specified, and the file does
 * not exist call create() in the parent directory vnode.
 *
 * Note: Increments vnode refcount on *res_vnode.
 */
int open_namev(const char *pathname, int flag, vnode_t **res_vnode,
		vnode_t *base) {
	/* NOT_YET_IMPLEMENTED("VFS: open_namev"); */
	int retValue = 0;
	size_t namelen = 0;
	const char *name = NULL;
	vnode_t *result = NULL;
	retValue = dir_namev(pathname, &namelen, &name, base, res_vnode);
	dbg(DBG_PRINT, "(GRADING2B)\n");
	if (retValue != 0) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return retValue;
	}
	base = *res_vnode;
	vput(*res_vnode);

	retValue = lookup(*res_vnode, name, namelen, &result);

	if (result == NULL && (flag & O_CREAT)) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		KASSERT(NULL != (*res_vnode)->vn_ops->create);
		dbg(DBG_PRINT, "(GRADING2A 2.c)\n");
		retValue = (*res_vnode)->vn_ops->create(*res_vnode, name, namelen,
				&result);

	}
	*res_vnode = result;
	return retValue;
}

#ifdef __GETCWD__
/* Finds the name of 'entry' in the directory 'dir'. The name is writen
 * to the given buffer. On success 0 is returned. If 'dir' does not
 * contain 'entry' then -ENOENT is returned. If the given buffer cannot
 * hold the result then it is filled with as many characters as possible
 * and a null terminator, -ERANGE is returned.
 *
 * Files can be uniquely identified within a file system by their
 * inode numbers. */
int
lookup_name(vnode_t *dir, vnode_t *entry, char *buf, size_t size)
{
	NOT_YET_IMPLEMENTED("GETCWD: lookup_name");
	return -ENOENT;
}

/* Used to find the absolute path of the directory 'dir'. Since
 * directories cannot have more than one link there is always
 * a unique solution. The path is writen to the given buffer.
 * On success 0 is returned. On error this function returns a
 * negative error code. See the man page for getcwd(3) for
 * possible errors. Even if an error code is returned the buffer
 * will be filled with a valid string which has some partial
 * information about the wanted path. */
ssize_t
lookup_dirpath(vnode_t *dir, char *buf, size_t osize)
{
	NOT_YET_IMPLEMENTED("GETCWD: lookup_dirpath");

	return -ENOENT;
}
#endif /* __GETCWD__ */
