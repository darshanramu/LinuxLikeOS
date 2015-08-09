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

/*
 *  FILE: vfs_syscall.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Wed Apr  8 02:46:19 1998
 *  $Id: vfs_syscall.c,v 1.10 2014/12/22 16:15:17 william Exp $
 */

#include "kernel.h"
#include "errno.h"
#include "globals.h"
#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/fcntl.h"
#include "fs/lseek.h"
#include "mm/kmalloc.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/stat.h"
#include "util/debug.h"

/* To read a file:
 *      o fget(fd)
 *      o call its virtual read fs_op
 *      o update f_pos
 *      o fput() it
 *      o return the number of bytes read, or an error
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for reading.
 *      o EISDIR
 *        fd refers to a directory.
 *
 * In all cases, be sure you do not leak file refcounts by returning before
 * you fput() a file that you fget()'ed.
 */
int do_read(int fd, void *buf, size_t nbytes) {
	/*NOT_YET_IMPLEMENTED("VFS: do_read");*/
	if (fd < 0 || fd >= NFILES) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return -EBADF;
	}
	file_t* file = fget(fd);

	if (file == NULL) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return -EBADF;
	}

	if (!(file->f_mode & FMODE_READ)) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		fput(file);
		return -EBADF;
	}

	if (S_ISDIR(file->f_vnode->vn_mode)) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		fput(file);
		return -EISDIR;
	}

	int bytes = file->f_vnode->vn_ops->read(file->f_vnode, file->f_pos, buf,
			nbytes);
	if (bytes == (int) nbytes) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		do_lseek(fd, bytes, SEEK_CUR);
	} /*else if (bytes < 0) {
	 dbg(DBG_ERROR, "5\n");
	 fput(file);
	 return bytes;
	 } */else {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		do_lseek(fd, 0, SEEK_END);
	}
	/*
	 if(bytes==nbytes){
	 do_lseek(fd,bytes,SEEK_CUR);
	 }
	 if(nbytes>bytes){
	 bytes=0;
	 }
	 */
	dbg(DBG_PRINT, "(GRADING2B)\n");
	fput(file);
	return bytes;
}

/* Very similar to do_read.  Check f_mode to be sure the file is writable.  If
 * f_mode & FMODE_APPEND, do_lseek() to the end of the file, call the write
 * fs_op, and fput the file.  As always, be mindful of refcount leaks.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for writing.
 */
int do_write(int fd, const void *buf, size_t nbytes) {
	/* NOT_YET_IMPLEMENTED("VFS: do_write");*/
	if (fd < 0 || fd >= NFILES) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return -EBADF;
	}
	file_t* file = fget(fd);

	if (file == NULL) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return -EBADF;
	}

	if (!(file->f_mode & FMODE_WRITE) && !(file->f_mode & FMODE_APPEND)) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		fput(file);
		return -EBADF;
	}

	int bytes;

	if (file->f_mode & FMODE_APPEND) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		do_lseek(fd, 0, SEEK_END);
		bytes = file->f_vnode->vn_ops->write(file->f_vnode, file->f_pos, buf,
				nbytes);
		if (0 != bytes) {
			dbg(DBG_PRINT, "(GRADING2B)\n");
			KASSERT(
					(S_ISCHR(file->f_vnode->vn_mode)) || (S_ISBLK(file->f_vnode->vn_mode)) || ((S_ISREG(file->f_vnode->vn_mode)) && (file->f_pos <= file->f_vnode->vn_len)));
			dbg(DBG_PRINT, "(GRADING2A 3.a)\n");
		}
		do_lseek(fd, bytes, SEEK_CUR);
		fput(file);
	} else if (file->f_mode & FMODE_WRITE) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		bytes = file->f_vnode->vn_ops->write(file->f_vnode, file->f_pos, buf,
				nbytes);
		if (0 != bytes) {
			dbg(DBG_PRINT, "(GRADING2B)\n");
			KASSERT(
					(S_ISCHR(file->f_vnode->vn_mode)) || (S_ISBLK(file->f_vnode->vn_mode)) || ((S_ISREG(file->f_vnode->vn_mode)) && (file->f_pos <= file->f_vnode->vn_len)));
			dbg(DBG_PRINT, "(GRADING2A 3.a)\n");
		}
		do_lseek(fd, bytes, SEEK_CUR);
		fput(file);
	}
	dbg(DBG_PRINT, "(GRADING2B)\n");
	return bytes;

}

/*
 * Zero curproc->p_files[fd], and fput() the file. Return 0 on success
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't a valid open file descriptor.
 */
int do_close(int fd) {
	/*NOT_YET_IMPLEMENTED("VFS: do_close");*/
	if (fd < 0) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return -EBADF;
	}
	file_t* file = fget(fd);
	if (file == NULL) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return -EBADF;
	}
	dbg(DBG_PRINT, "(GRADING2B)\n");
	curproc->p_files[fd] = NULL;
	fput(file);
	return 0;
}

/* To dup a file:
 *      o fget(fd) to up fd's refcount
 *      o get_empty_fd()
 *      o point the new fd to the same file_t* as the given fd
 *      o return the new file descriptor
 *
 * Don't fput() the fd unless something goes wrong.  Since we are creating
 * another reference to the file_t*, we want to up the refcount.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't an open file descriptor.
 *      o EMFILE
 *        The process already has the maximum number of file descriptors open
 *        and tried to open a new one.
 */
int do_dup(int fd) {
	/*NOT_YET_IMPLEMENTED("VFS: do_dup");*/
	if (fd < 0 || fd >= NFILES) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return -EBADF;
	}

	file_t* file = fget(fd);
	if (file == NULL) {
		dbg(DBG_PRINT, "(GRADING2B)\n");

		return -EBADF;
	}
	int new_empty_fd = get_empty_fd(curproc);
	/*
	 if (new_empty_fd == -EMFILE) {
	 fput(file);
	 dbg(DBG_ERROR, "3\n");

	 return -EMFILE;
	 }
	 */
	curproc->p_files[new_empty_fd] = file;
	/*file_t* newfile = fget(new_empty_fd); */
	dbg(DBG_PRINT, "(GRADING2B)\n");
	return new_empty_fd;
}

/* Same as do_dup, but insted of using get_empty_fd() to get the new fd,
 * they give it to us in 'nfd'.  If nfd is in use (and not the same as ofd)
 * do_close() it first.  Then return the new file descriptor.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        ofd isn't an open file descriptor, or nfd is out of the allowed
 *        range for file descriptors.
 */
int do_dup2(int ofd, int nfd) {
	/*NOT_YET_IMPLEMENTED("VFS: do_dup2");*/
	if (nfd < 0 || nfd >= NFILES) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return -EBADF;
	}
	if (ofd < 0 || ofd >= NFILES) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return -EBADF;
	}
	file_t* file = fget(ofd);
	if (file == NULL) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return -EBADF;
	}

	file_t* nFile = fget(nfd);

	if (ofd != nfd && nFile != NULL) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		fput(nFile);
		do_close(nfd);

	} else if (ofd == nfd) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		fput(file);
		fput(nFile);
	}
	dbg(DBG_PRINT, "(GRADING2B)\n");
	curproc->p_files[nfd] = file;

	return nfd;
}

/* Used to check if the pathlength or name lenght of the file exceeds
 * the maximum limit of the system
 * Returns: 0 on Sucess and 1 on Failure
 *
 int check_pathlen(const char *path) {
 if (strlen(path) > MAXPATHLEN) {
 return 1;
 }
 char *s = strtok((char *) path, "/");
 if (strlen(s) > NAME_LEN)
 return 1;
 while (NULL != (s = strtok(NULL,"/"))) {
 if (strlen(s) > NAME_LEN)
 return 1;
 }
 return 0;
 }
 SCREW STRTOK*/
/*
 * This routine creates a special file of the type specified by 'mode' at
 * the location specified by 'path'. 'mode' should be one of S_IFCHR or
 * S_IFBLK (you might note that mknod(2) normally allows one to create
 * regular files as well-- for simplicity this is not the case in Weenix).
 * 'devid', as you might expect, is the device identifier of the device
 * that the new special file should represent.
 *
 * You might use a combination of dir_namev, lookup, and the fs-specific
 * mknod (that is, the containing directory's 'mknod' vnode operation).
 * Return the result of the fs-specific mknod, or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        mode requested creation of something other than a device special
 *        file.
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int do_mknod(const char *path, int mode, unsigned devid) {
	/* NOT_YET_IMPLEMENTED("VFS: do_mknod"); */

	if (mode != S_IFCHR && mode != S_IFBLK) {
		dbg(DBG_ERROR, "1\n");
		return -EINVAL;
	}
	dbg(DBG_PRINT, "(GRADING2B)\n");
	size_t namelen = NULL;
	const char *name = NULL;
	vnode_t *base;
	vnode_t *res_vnode = NULL, *result = NULL;
	/*if (strlen(path) > MAXPATHLEN){
	 dbg(DBG_ERROR,"2\n");
	 return -ENAMETOOLONG;
	 }*/
	/* Base passed NULL since dir_namev can handle accordingly */
	int retdir_namev = dir_namev(path, &namelen, &name, NULL, &res_vnode);
	/*dbg(DBG_PRINT,"culprit is %s",path);*/
	/*
	 if (strlen(name) > NAME_LEN) {
	 dbg(DBG_ERROR,"3\n");
	 if (retdir_namev == 0) {
	 dbg(DBG_ERROR,"4\n");
	 vput(res_vnode);
	 }
	 return -ENAMETOOLONG;
	 }*/
	if (retdir_namev == 0) {
		dbg(DBG_PRINT, "(GRADING2B)\n");

		int retlookup = lookup(res_vnode, name, namelen, &result);

		if (result != NULL) {
			dbg(DBG_ERROR, "6\n");
			vput(result);
			return -EEXIST;
		} else if (result == NULL) {
			dbg(DBG_PRINT, "(GRADING2B)\n");
			KASSERT(NULL != (res_vnode)->vn_ops->mknod);
			dbg(DBG_PRINT, "(GRADING2A 3.b)\n");

			int ramfs_ret = (res_vnode)->vn_ops->mknod(res_vnode, name, namelen,
					mode, devid);
			/*lookup(res_vnode, name, namelen, &result);*/
			/*dbg(DBG_PRINT, "Created special file %s ref=%d\n", path,
			 result->vn_refcount);*/

			return ramfs_ret;
		}

		return retlookup;

	} /*else {
	 dbg(DBG_ERROR,"8\n");
	 return retdir_namev;
	 }*/
	/*dbg(DBG_ERROR,"9\n");*/
	return 0;
}

/* Use dir_namev() to find the vnode of the dir we want to make the new
 * directory in.  Then use lookup() to make sure it doesn't already exist.
 * Finally call the dir's mkdir vn_ops. Return what it returns.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int do_mkdir(const char *path) {
	/*NOT_YET_IMPLEMENTED("VFS: do_mkdir");*/

	dbg(DBG_PRINT, "(GRADING2B)\n");
	size_t namelen = 0;
	const char *name;
	vnode_t *base;
	vnode_t *res_vnode, *result;
	/*
	 if (strlen(path) > MAXPATHLEN){
	 dbg(DBG_ERROR,"1\n");
	 return -ENAMETOOLONG;
	 }*/
	/* Base passed NULL since dir_namev can handle accordingly */
	int retdir_namev = dir_namev(path, &namelen, &name, NULL, &res_vnode);

	if (strlen(name) > NAME_LEN) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		if (retdir_namev == 0) {
			dbg(DBG_PRINT, "(GRADING2B)\n");

		}
		return -ENAMETOOLONG;
	}
	if (retdir_namev == 0) {
		dbg(DBG_PRINT, "(GRADING2B)\n");

		int retlookup = lookup(res_vnode, name, namelen, &result);

		if (retlookup == 0) {
			dbg(DBG_PRINT, "(GRADING2B)\n");
			vput(result);
			vput(res_vnode);
			return -EEXIST;
		}
		if (retlookup == -ENOENT) {
			dbg(DBG_PRINT, "(GRADING2B)\n");
			KASSERT(NULL != (res_vnode)->vn_ops->mkdir);
			dbg(DBG_PRINT, "(GRADING2A 3.c)\n");

			int ramfs_ret = (res_vnode)->vn_ops->mkdir(res_vnode, name,
					namelen);
			vput(res_vnode);
			return ramfs_ret;
		}
		if (retlookup == -ENOTDIR) {
			vput(res_vnode);
			return -ENOTDIR;
		}

	} else {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return retdir_namev;
	}
	return 0;
}

/* Use dir_namev() to find the vnode of the directory containing the dir to be
 * removed. Then call the containing dir's rmdir v_op.  The rmdir v_op will
 * return an error if the dir to be removed does not exist or is not empty, so
 * you don't need to worry about that here. Return the value of the v_op,
 * or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        path has "." as its final component.
 *      o ENOTEMPTY
 *        path has ".." as its final component.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int do_rmdir(const char *path) {
	/* NOT_YET_IMPLEMENTED("VFS: do_rmdir"); */
	size_t namelen;
	const char *name;
	vnode_t *base;
	vnode_t *res_vnode, *result;
	int tempLookup = 0;
	dbg(DBG_PRINT, "(GRADING2B)\n");
	/*
	 if (strlen(path) > MAXPATHLEN) {
	 dbg(DBG_ERROR, "1\n");
	 return -ENAMETOOLONG;
	 }*/
	/* Base passed NULL since dir_namev can handle accordingly */
	int retdir_namev = dir_namev(path, &namelen, &name, NULL, &res_vnode);
	if (retdir_namev != 0) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return retdir_namev;
	}
	if (name_match(".", name, namelen)) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		vput(res_vnode);
		return -EINVAL;
	}
	if (name_match("..", name, namelen)) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		vput(res_vnode);
		return -ENOTEMPTY;
	}
	if (strlen(name) > NAME_LEN) {
		dbg(DBG_ERROR, "5\n");
		vput(res_vnode);
		return -ENAMETOOLONG;
	}
	if (0 == retdir_namev) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		vput(res_vnode);
		tempLookup = lookup(res_vnode, name, namelen, &result);

		KASSERT(NULL != (res_vnode)->vn_ops->rmdir);
		dbg(DBG_PRINT, "(GRADING2A 3.d)\n");

		int ret_rmdir = (res_vnode)->vn_ops->rmdir(res_vnode, name, namelen);
		if (ret_rmdir == 0 && result != NULL) {
			dbg(DBG_PRINT, "(GRADING2B)\n");
			vput(result);

		} else if (tempLookup == 0 && result != NULL) {
			dbg(DBG_PRINT, "(GRADING2B)\n");
			vput(result);
		}
		return ret_rmdir;
	}
	return 0;
}

/*
 * Same as do_rmdir, but for files.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EISDIR
 *        path refers to a directory.
 *      o ENOENT
 *        A component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int do_unlink(const char *path) {
	/* NOT_YET_IMPLEMENTED("VFS: do_unlink"); */
	size_t namelen;
	const char *name;
	vnode_t *base;
	vnode_t *res_vnode, *result;
	dbg(DBG_PRINT, "(GRADING2B)\n");
	/*if (strlen(path) > MAXPATHLEN) {
	 dbg(DBG_ERROR, "2\n");
	 return -ENAMETOOLONG;
	 }*/
	/* Base passed NULL since dir_namev can handle accordingly */
	int retdir_namev = dir_namev(path, &namelen, &name, NULL, &res_vnode);
	/*if (strlen(name) > NAME_LEN) {
	 dbg(DBG_ERROR, "3\n");
	 if (retdir_namev == 0) {
	 dbg(DBG_ERROR, "4\n");
	 vput(res_vnode);
	 }
	 return -ENAMETOOLONG;
	 }*/
	/*dbg(DBG_ERROR, "name is %s\n", name);*/
	if (0 == retdir_namev) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		vput(res_vnode);
		int retlookup = lookup(res_vnode, name, namelen, &result);

		if (retlookup == 0) {
			dbg(DBG_PRINT, "(GRADING2B)\n");
			if (NULL != result) {
				dbg(DBG_PRINT, "(GRADING2B)\n");

				if (S_ISDIR((result)->vn_mode)) {
					dbg(DBG_PRINT, "(GRADING2B)\n");
					vput(result);
					return -EISDIR;
				}
				vput(result);

				KASSERT(NULL != (res_vnode)->vn_ops->unlink);
				dbg(DBG_PRINT, "(GRADING2A 3.e)\n");

				int ret_unlink = (res_vnode)->vn_ops->unlink(res_vnode, name,
						namelen);

			}
		} else {
			dbg(DBG_PRINT, "(GRADING2B)\n");
			return retlookup;
		}

	} /*else {
	 dbg(DBG_ERROR, "10\n");
	 return retdir_namev;
	 }*/

	dbg(DBG_PRINT, "(GRADING2B)\n");
	return 0;
}

/* To link:
 *      o open_namev(from)
 *      o dir_namev(to)
 *      o call the destination dir's (to) link vn_ops.
 *      o return the result of link, or an error
 *
 * Remember to vput the vnodes returned from open_namev and dir_namev.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        to already exists.
 *      o ENOENT
 *        A directory component in from or to does not exist.
 *      o ENOTDIR
 *        A component used as a directory in from or to is not, in fact, a
 *        directory.
 *      o ENAMETOOLONG
 *        A component of from or to was too long.
 *      o EISDIR
 *        from is a directory.
 */
int do_link(const char *from, const char *to) {
	/*NOT_YET_IMPLEMENTED("VFS: do_link");*/
	vnode_t *from_vn, *to_result;
	size_t to_namelen;
	const char *to_name;
	vnode_t *to_vn;

	int to_retdir_namev, to_retlookup;

	open_namev(from, O_RDONLY, &from_vn, NULL);

	dbg(DBG_PRINT, "(GRADING3D 2)\n");
	/*if (ret_namev < 0) {
		dbg(DBG_ERROR, "2\n");
		return ret_namev;
	}*/

	if (S_ISDIR((from_vn)->vn_mode)) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		vput(from_vn);
		return -EISDIR;
	}

	to_retdir_namev = dir_namev(to, &to_namelen, &to_name, NULL, &to_vn);

	if (to_retdir_namev < 0) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		vput(from_vn);
		return to_retdir_namev;
	}

	/*if (to_vn->vn_ops->link == NULL) {
		dbg(DBG_ERROR, "5\n");
		return -ENOTDIR;
	}*/
	to_retlookup = lookup(to_vn, to_name, to_namelen, &to_result);
	if (to_retlookup == 0) {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		vput(to_result);
		return -EEXIST;
	} else {
		dbg(DBG_PRINT, "(GRADING3D 2)\n");
		int to_retlink = to_vn->vn_ops->link(from_vn, to_vn, to_name, to_namelen);
		vput(to_vn);
		vput(from_vn);
		return to_retlink;
	}
	return -1;
}

/*      o link newname to oldname
 *      o unlink oldname
 *      o return the value of unlink, or an error
 *
 * Note that this does not provide the same behavior as the
 * Linux system call (if unlink fails then two links to the
 * file could exist).
 */
int do_rename(const char *oldname, const char *newname) {
	/*NOT_YET_IMPLEMENTED("VFS: do_rename");*/
	int retValue;

	retValue = do_link(oldname, newname);

	if (retValue == 0) {
		retValue = do_unlink(oldname);
	}

	return retValue;
}

/* Make the named directory the current process's cwd (current working
 * directory).  Don't forget to down the refcount to the old cwd (vput()) and
 * up the refcount to the new cwd (open_namev() or vget()). Return 0 on
 * success.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        path does not exist.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 *      o ENOTDIR
 *        A component of path is not a directory.
 */
int do_chdir(const char *path) {
	/* NOT_YET_IMPLEMENTED("VFS: do_chdir"); */
	int retopen_namev=0;
	vnode_t *result;

	dbg(DBG_PRINT, "(GRADING2B)\n");
	/*int retdir_namev;*/
	/*if (strlen(path) > MAXPATHLEN){
	 dbg(DBG_ERROR, "1\n");
	 return -ENAMETOOLONG;
	 }*/

	/*retdir_namev =*/

	/*if (strlen(name) > NAME_LEN) {
	 dbg(DBG_ERROR, "2\n");
	 if (retdir_namev == 0) {
	 dbg(DBG_ERROR, "3\n");
	 vput(res_vnode);
	 }
	 return -ENAMETOOLONG;

	 }
	 if (retdir_namev != 0) {
	 dbg(DBG_ERROR, "4\n");
	 dbg(DBG_PRINT, "1chdir returns %d\n", retdir_namev);
	 return retdir_namev;
	 }*/
	retopen_namev = open_namev(path, O_RDONLY, &result, NULL);

	if (retopen_namev < 0) {
		return retopen_namev;
	}

	if (!S_ISDIR(result->vn_mode)) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		vput(result);
		return -ENOTDIR;
	}

	vput(curproc->p_cwd);
	curproc->p_cwd = result;
	dbg(DBG_PRINT, "(GRADING2B)\n");
	return 0;

}

/* Call the readdir fs_op on the given fd, filling in the given dirent_t*.
 * If the readdir fs_op is successful, it will return a positive value which
 * is the number of bytes copied to the dirent_t.  You need to increment the
 * file_t's f_pos by this amount.  As always, be aware of refcounts, check
 * the return value of the fget and the virtual function, and be sure the
 * virtual function exists (is not null) before calling it.
 *
 * Return either 0 or sizeof(dirent_t), or -errno.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        Invalid file descriptor fd.
 *      o ENOTDIR
 *        File descriptor does not refer to a directory.
 */
int do_getdent(int fd, struct dirent *dirp) {
	/*  NOT_YET_IMPLEMENTED("VFS: do_getdent"); */
	dbg(DBG_PRINT, "(GRADING2B)\n");
	if (fd < 0 || fd >= NFILES) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return -EBADF;
	}

	file_t *file = fget(fd);
	/*off_t offset = NULL;*/
	if (file == NULL) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return -EBADF;
	}

	if (!S_ISDIR(file->f_vnode->vn_mode)) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		fput(file);
		return -ENOTDIR;
	}
	int retval;
	if (file->f_vnode->vn_ops->readdir != NULL) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		retval = file->f_vnode->vn_ops->readdir(file->f_vnode, file->f_pos,
				dirp);

	}
	file->f_pos += retval;

	fput(file);
	if (retval > 0) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		retval = sizeof(dirent_t);
	}
	return retval;
}

/*
 * Modify f_pos according to offset and whence.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not an open file descriptor.
 *      o EINVAL
 *        whence is not one of SEEK_SET, SEEK_CUR, SEEK_END; or the resulting
 *        file offset would be negative.
 */
int do_lseek(int fd, int offset, int whence) {
	/* NOT_YET_IMPLEMENTED("VFS: do_lseek");*/
	dbg(DBG_PRINT, "(GRADING2B)\n");
	int retVal;
	off_t org_fpos;
	if (fd < 0 || fd >= NFILES) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return -EBADF;
	}
	file_t* file = fget(fd);
	if (file == NULL) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return -EBADF;
	}
	org_fpos = file->f_pos;
	if (whence == SEEK_SET) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		file->f_pos = offset;
	} else if (whence == SEEK_CUR) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		file->f_pos += offset;
	} else if (whence == SEEK_END) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		file->f_pos = file->f_vnode->vn_len + offset;
	} else {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		fput(file);
		return -EINVAL;
	}
	retVal = file->f_pos;
	if (file->f_pos < 0) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		file->f_pos = org_fpos;
		retVal = -EINVAL;
	}

	fput(file);
	return retVal;

}

/*
 * Find the vnode associated with the path, and call the stat() vnode operation.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        A component of path does not exist.
 *      o ENOTDIR
 *        A component of the path prefix of path is not a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int do_stat(const char *path, struct stat *buf) {
	/* NOT_YET_IMPLEMENTED("VFS: do_stat"); */
	vnode_t *result;
	int retopen_namev=0;
	/*size_t *namelen = (size_t) kalloc(sizeof(size_t));
	 const char **name = (char) kalloc(sizeof(char));
	 vnode_t **res_vnode = (vnode_t) kalloc(sizeof(vnode_t)); */
	/*if (strlen(path) > MAXPATHLEN) {
	 dbg(DBG_ERROR, "2\n");
	 return ENAMETOOLONG;
	 }*/
	/* Base passed NULL since dir_nameve can handle acccordingly */

	/*if (retdir_namev != 0) {
	 dbg(DBG_ERROR, "3\n");
	 return retdir_namev;
	 }
	 if (strlen(name) > NAME_LEN) {
	 dbg(DBG_ERROR, "4\n");
	 vput(res_vnode);
	 return -ENAMETOOLONG;
	 }*/

	if (path == NULL || buf == NULL || strlen(path) == 0) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return -EINVAL;
	}

	retopen_namev = open_namev(path, O_RDONLY, &result, NULL);

	if (retopen_namev < 0) {
		dbg(DBG_PRINT, "(GRADING2B)\n");
		return retopen_namev;
	}

	retopen_namev = result->vn_ops->stat(result, buf);
	vput(result);
	return retopen_namev;

}

#ifdef __MOUNTING__
/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutely sure your Weenix is perfect.
 *
 * This is the syscall entry point into vfs for mounting. You will need to
 * create the fs_t struct and populate its fs_dev and fs_type fields before
 * calling vfs's mountfunc(). mountfunc() will use the fields you populated
 * in order to determine which underlying filesystem's mount function should
 * be run, then it will finish setting up the fs_t struct. At this point you
 * have a fully functioning file system, however it is not mounted on the
 * virtual file system, you will need to call vfs_mount to do this.
 *
 * There are lots of things which can go wrong here. Make sure you have good
 * error handling. Remember the fs_dev and fs_type buffers have limited size
 * so you should not write arbitrary length strings to them.
 */
int
do_mount(const char *source, const char *target, const char *type)
{
	NOT_YET_IMPLEMENTED("MOUNTING: do_mount");
	return -EINVAL;
}

/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutley sure your Weenix is perfect.
 *
 * This function delegates all of the real work to vfs_umount. You should not worry
 * about freeing the fs_t struct here, that is done in vfs_umount. All this function
 * does is figure out which file system to pass to vfs_umount and do good error
 * checking.
 */
int
do_umount(const char *target)
{
	NOT_YET_IMPLEMENTED("MOUNTING: do_umount");
	return -EINVAL;
}
#endif
