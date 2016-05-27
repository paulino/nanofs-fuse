/******************************************************************************

  NanoFuse, NanoFS FUSE application

  This file is part of NanoFS project

  NanoFS is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  NanoFS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with NanoFS.  If not, see <http://www.gnu.org/licenses/>.

  @author Paulino Ruiz de Clavijo Vázquez <paulino@dte.us.es>,
          Enrique Ostúa, <ostua@dte.us.es>.

  @date 2016-23-05
  @version 0.4

  This code is derived from function prototypes found
  in /usr/include/fuse/fuse.h

******************************************************************************/

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <asm/types.h>

#define FUSE_USE_VERSION 26

#include "fuse.h"
#include "log.h"
#include "nanofs_filedir.h"
#include "nanofuse.h"

// Max number of entries to read in a directory
#define MAX_DIRENTRIES 5000

int nanofuse_buildstatbuf(struct nanofs_filedir_handle *, struct stat *);
int nanofuse_rootfsstat(struct stat *);


///////////////////////////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come indirectly from /usr/include/fuse.h
//
// Some remarks:
// * returning an error: operations should return the negated error
//   value (-errno) directly.
// * the path param can be of any length
//
///////////////////////////////////////////////////////////////////////////////


/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */

int nanofuse_getattr(const char *path, struct stat *statbuf)
{
    int retstat = 0;
    struct nanofs_filedir_handle fd_hd;


    log_debug("nanofuse_getattr: path='%s'",path);

	retstat = nanofs_lookup_absolute(&nanofuse_CONTEXT->fs_hd,path,&fd_hd);
	if (retstat == 0)
	    // Build stat buf
	    retstat = nanofuse_buildstatbuf(&fd_hd,statbuf);
    return -retstat;
}

/** Built stat buff, called from getattr() or fgetattr()
 * @return 0 on succcess or ESTALE when filesustem error
 */
int nanofuse_buildstatbuf(struct nanofs_filedir_handle *fd_hd,
        struct stat *statbuf)
{
    long int f_size;
    // Build stat buf
     if (DN_ISDIR(fd_hd->f_dir_node)) // if its a directory
     {
         f_size = 1 << nanofuse_CONTEXT->fs_hd.h_block_bits;
         statbuf->st_mode = 0040755;
     }
     else //  It must be a regular file, not other things implementedç
     {
         f_size = nanofs_get_file_size(&nanofuse_CONTEXT->fs_hd,fd_hd);
         if(f_size == -1)
             return -ESTALE;
         statbuf->st_mode = 0100755;
     }

     statbuf->st_nlink = 0;  // number of hard links
     statbuf->st_uid = fuse_get_context()->uid;
     statbuf->st_gid = fuse_get_context()->gid;
     statbuf->st_rdev = 0; // device ID (if special file)

     statbuf->st_size = f_size; //  total size, in bytes

     // FIXME: Not implemented used blocks calculation
     statbuf->st_blocks = f_size >> 9; // number of 512B blocks allocated

     statbuf->st_atime = 0x53ac2f27;
     statbuf->st_mtime = 0x53ac2f27;
     statbuf->st_ctime = 0x53ac2f27;
     return 0;
}


int nanofuse_rootfsstat(struct stat *statbuf)
{
	statbuf->st_dev = 0; // IGNORED
	statbuf->st_ino = 0;  // IGNORED (unless option used in mount)
	statbuf->st_mode = 0040755;
	statbuf->st_nlink = 0;
	statbuf->st_uid = 1000;
	statbuf->st_gid = 1000;
	statbuf->st_rdev = 0;
	statbuf->st_size = 4096;
	statbuf->st_blksize = NANOFS_DEFAULT_BLKSIZE; // IGNORED
	statbuf->st_blocks = statbuf->st_size/NANOFS_DEFAULT_BLKSIZE;
    statbuf->st_atime = 0x53c4c1ce;
    statbuf->st_mtime = 0x53c4c1ce;
    statbuf->st_ctime = 0x53c4c1ce;

	return 0;
}


/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.  If the linkname is too long to fit in the
 * buffer, it should be truncated.  The return value should be 0
 * for success.
 */

int nanofuse_readlink(const char *path, char *link, size_t size)
{
    //int retstat = 0;

    log_debug("nanofuse_readlink: not implemented, path=\'s', "
            "link='%s', size=%d)", path, link, size);

    return -1;
}

/** Create a file node
 *
 * This is called for creation of all non-directory, non-symlink
 * nodes.  If the filesystem defines a create() method, then for
 * regular files that will be called instead.
 */

int nanofuse_mknod(const char *path, mode_t mode, dev_t dev)
{
    int retstat = -1;

    log_debug("nanofuse_mknod: not implemented, path='%s', mode=0%3o, "
            "dev=%lld)", path, mode, dev);

   return retstat;
}


/** Create a directory
 *
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits use  mode|S_IFDIR
 * */

int nanofuse_mkdir(const char *path, mode_t mode)
{
    int retstat = 0;
    struct nanofs_filedir_handle dir_hd;
    char *basename_buf = strdup(path);
    char *dirname_buf = strdup(path);
    char *base_name = basename(basename_buf);
	char *dir_name =  dirname(dirname_buf);

    log_debug("nanofuse_mkdir: path='%s', mode=0%3o", path, mode);

    retstat = nanofs_lookup_absolute(&nanofuse_CONTEXT->fs_hd,
            dir_name, &dir_hd);
    if (retstat != 0)
        log_error("nanofuse_mkdir: chdir error for path %s",path);
    else
    {
	    retstat = nanofs_mkdir(&nanofuse_CONTEXT->fs_hd, base_name, &dir_hd);
		if (retstat != 0)
		{
            errno = retstat;
		    log_error("nanofuse_mkdir: mkdir fails for path %s",path);
		}
    }

    free(basename_buf);
    free(dirname_buf);
    return -retstat;
}

/** Remove a file */
int nanofuse_unlink(const char *path)
{
    int retstat = 0;
    struct nanofs_filedir_handle parent_dir_hd;
    char *basename_buf = strdup(path);
    char *dirname_buf= strdup(path);
    char *base_name = basename(basename_buf);
    char *dir_name = dirname(dirname_buf);

    log_debug("nanofuse_unlink: path='%s'", path);

	retstat = nanofs_lookup_absolute(&nanofuse_CONTEXT->fs_hd,
	        dir_name, &parent_dir_hd);
    if (retstat != 0)
    {
        errno = retstat;
        log_error("nanofuse_unlink: lookup parent dir failed");
    }
    else
    {
	    retstat = nanofs_rm(&nanofuse_CONTEXT->fs_hd, base_name, &parent_dir_hd);
		if (retstat != 0)
		    log_error("nanofuse_unlink: nanofs_rm failed");
    }

    free(dirname_buf);
    free(basename_buf);

    return -retstat;
}

/** Remove a directory */
int nanofuse_rmdir(const char *path)
{
    int retstat = 0;
    char *basename_buf = strdup(path);
    char *dirname_buf= strdup(path);
    char *base_name = basename(basename_buf);
    char *dir_name = dirname(dirname_buf);
    struct nanofs_filedir_handle parent_dir_hd;

    log_debug("nanofuse_rmdir: path='%s'", path);

	retstat = nanofs_lookup_absolute(&nanofuse_CONTEXT->fs_hd,
	        dir_name,&parent_dir_hd);
    if (retstat != 0)
        log_error("nanofuse_rmdir: chdir failed for path '%s'",path);
    else
    {
	    retstat = nanofs_rmdir(&nanofuse_CONTEXT->fs_hd,
	            base_name, &parent_dir_hd);
		if (retstat)
		    log_error("nanofuse_rmdir: rmdir failed for path '%s'",path);
    }

    free(dirname_buf);
    free(basename_buf);

    return -retstat;
}

/** Create a symbolic link */
// The parameters here are a little bit confusing, but do correspond
// to the symlink() system call.  The 'path' is where the link points,
// while the 'link' is the link itself.  So we need to leave the path
// unaltered, but insert the link into the mounted directory.
int nanofuse_symlink(const char *path, const char *link)
{
    log_debug("nanofuse_symlink: not implemented, path='%s', link='%s'",
	    path, link);

    return -1;
}

/** Rename a file
 * @TODO: This other day ...
 * */
int nanofuse_rename(const char *path, const char *newpath)
{
    int retstat = ENOTSUP;

    log_debug("nanofuse_rename: not implemented: path='%s', newpath='%s'",
	    path, newpath);


    return -retstat;
}

/** Create a hard link to a file */
int nanofuse_link(const char *path, const char *newpath)
{
    int retstat = ENOTSUP;

    log_debug("nanofuse_link: not supported path='%s', newpath='%s'",
	    path, newpath);

    return -retstat;
}

/** Change the permission bits of a file */
int nanofuse_chmod(const char *path, mode_t mode)
{
    int retstat = ENOTSUP;

    log_debug("nanofuse_chmod: not implemented, path='%s', mode=0%03o)",
	    path, mode);

    return -retstat;
}

/** Change the owner and group of a file */
int nanofuse_chown(const char *path, uid_t uid, gid_t gid)
{
    int retstat = ENOTSUP;

    log_debug("nanofuse_chown: not implemented, path='%s', uid=%d, gid=%d)",
	    path, uid, gid);


    return -retstat;
}

/** Change the size of a file  (essential method)
 *
 * */
int nanofuse_truncate(const char *path, off_t newsize)
{
    int retstat;
    struct nanofs_filedir_handle file_hd;

    log_debug("nanofuse_truncate: path='%s', newsize=%lld)",
	    path, newsize);

    retstat = nanofs_lookup_absolute(&nanofuse_CONTEXT->fs_hd,
            path, &file_hd);
    if(retstat == 0)
        retstat = nanofs_truncate(&nanofuse_CONTEXT->fs_hd,
                &file_hd, newsize);

    return -retstat;
}

/** Change the access and/or modification times of a file */
int nanofuse_utime(const char *path, struct utimbuf *ubuf)
{
    int retstat = 0;

    log_debug("nanofuse_utime: (not implemented) path='%s', ubuf=0x%08x)",
	    path, ubuf);

    return -retstat;
}

/** File open operation (essential method)
 *
 * No creation (O_CREAT, O_EXCL) and by default also no
 * truncation (O_TRUNC) flags will be passed to open(). If an
 * application specifies O_TRUNC, fuse first calls truncate()
 * and then open(). Only if 'atomic_o_trunc' has been
 * specified and kernel version is 2.6.24 or later, O_TRUNC is
 * passed on to open.
 *
 * Unless the 'default_permissions' mount option is given,
 * open should check if the operation is permitted for the
 * given flags. Optionally open may also return an arbitrary
 * filehandle in the fuse_file_info structure, which will be
 * passed to all file operations.
 *
 * new 'nanofs_filedir_handle *' is attached to 'fi->fh'
 *
 */
int nanofuse_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    struct nanofs_filedir_handle *file_hd =
            malloc(sizeof(struct nanofs_filedir_handle));

    log_debug("nanofuse_open: path'%s', fi=0x%08x",
	    path, fi);


    retstat = nanofs_lookup_absolute(&nanofuse_CONTEXT->fs_hd,path,file_hd);
    if (retstat != 0)
        log_error("nanofuse_open: lookup file error ");
    // Check if the path is a regular file
    if (retstat == 0 && !DN_ISREG(file_hd->f_dir_node))
        retstat = -1;

    if(retstat == 0 ) // returning file handle to use later in fuse operations
        fi->fh = (uint64_t)file_hd;

    if(retstat != 0) // On error free resources
        free(file_hd);

    return -retstat;
}

/** Read data from an open file (essential method)
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 */

int nanofuse_read(const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi)
{
    int retstat = 0;

    log_debug("nanofuse_read: path='%s' size=%d, offset=%lld ",
            path, size, offset );

    retstat = nanofs_read(&nanofuse_CONTEXT->fs_hd,
            (struct nanofs_filedir_handle *)fi->fh, buf, size, offset);


    return retstat;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * This call can happen after truncate and truncate cannot update 'file_info'
 * data. Nanofs dir_node may be reload
 *
 */

int nanofuse_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    size_t bytes_written;
    struct nanofs_filedir_handle *file_handle;

    log_debug("nanofuse_write: path='%s',size=%d, offset=%lld",
            path, size, offset);

    // no need to get path on this one, since I work from fi->fh
    // but 'dir_node' must be reloaded from device
    file_handle = (struct nanofs_filedir_handle *)fi->fh;

    if (nanofs_read_dir_node_b(&nanofuse_CONTEXT->fs_hd,
            file_handle->f_blk_no,&file_handle->f_dir_node) != 0)
        return -EIO;

    // Get dir_node from block_no stored in fi->fh
    bytes_written = nanofs_write(&nanofuse_CONTEXT->fs_hd, file_handle,
             buf, size, offset);
    if(bytes_written != size)
    {
        log_error("nanofuse_write: cannot write ");
    }
    return bytes_written;

}

/** Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 */
int nanofuse_statfs(const char *path, struct statvfs *statv)
{
    int retstat = 0;
    struct nanofs_superblock *sb =  &nanofuse_CONTEXT->fs_hd.h_sb;
    __s64 free_space = nanofs_free(&nanofuse_CONTEXT->fs_hd);

    log_debug("nanofuse_statfs: path='%s'",path);
    statv->f_bsize = 512;             // File system block size
    statv->f_blocks = sb->s_fs_size;  // Size of fs in f_frsize units

    // Number of free blocks
    statv->f_bfree = free_space >> (nanofuse_CONTEXT->fs_hd.h_block_bits);
    statv->f_bavail = statv->f_bfree; // Number of free blocks for
                                    // unprivileged users

    statv->f_namemax = NANOFS_MAXFILENAME; // Maximum filename length

    return retstat;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Nanofuse: nothing to do here ...
 */
int nanofuse_flush(const char *path, struct fuse_file_info *fi)
{
    log_debug("nanofuse_flush: path='%s'", path);
    return 0;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 */
int nanofuse_release(const char *path, struct fuse_file_info *fi)
{
    log_debug("nanofuse_release: path='%s'", path);

    // Free the allocated 'nanofs_filedir_handle' in open()
    free( (struct nanofs_filedir_handle *)(fi->fh) );

    return 0;
}

/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 */
int nanofuse_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    log_debug("nanofuse_fsync: path='%s', datasync=%d", path, datasync);
    return 0;
}

/** Set extended attributes */
int nanofuse_setxattr(const char *path, const char *name, const char *value,
        size_t size, int flags)
{
    int retstat = ENOTSUP;

    log_debug("nanofuse_setxattr: path='%s', name='%s', value='%s', size=%d, "
            "flags=0x%08x", path, name, value, size, flags);

    return -retstat;
}

/** Get extended attributes */
int nanofuse_getxattr(const char *path, const char *name, char *value,
        size_t size)
{
    int retstat = ENOTSUP;

    log_debug("nanofuse_getxattr: path=' %s', name='%s', value = 0x%08x, "
            "size = %d", path, name, value, size);

    return -retstat;
}

/** List extended attributes */
int nanofuse_listxattr(const char *path, char *list, size_t size)
{
    int retstat = ENOTSUP;

    log_debug("nanofuse_listxattr: path='%s', list=0x%08x, size=%d",
	    path, list, size);

    return -retstat;
}

/** Remove extended attributes */
int nanofuse_removexattr(const char *path, const char *name)
{
    int retstat = ENOTSUP;

    log_debug("nanofuse_removexattr: path='%s', name='%s'", path, name);

    return -retstat;
}

/** Open directory
 *
 * Unless the 'default_permissions' mount option is given,
 * this method should check if opendir is permitted for this
 * directory. Optionally opendir may also return an arbitrary
 * filehandle in the fuse_file_info structure, which will be
 * passed to readdir, closedir and fsyncdir.
 *
 * On success 'fi->fh' has a directory handle, it must be freed in 'releasedir'
 */

int nanofuse_opendir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    struct nanofs_filedir_handle *dir_handle =
            malloc(sizeof(struct nanofs_filedir_handle));

    log_debug("nanofuse_opendir: path='%s'", path);

    retstat = nanofs_lookup_absolute(&nanofuse_CONTEXT->fs_hd,path,dir_handle);
    // Check if it is a directory
    if(retstat == 0 && !DN_ISDIR(dir_handle->f_dir_node))
        retstat = ENOTDIR;

    if (retstat)
    {
        log_error("nanofuse_opendir: error");
        free(dir_handle);
    }
    else
        fi->fh = (uint64_t) (dir_handle);

    return -retstat;
}

/** Read directory

 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 *
 * 'readdir' is called after opendir, in 'fi->fh' is the 'dir_handle'
 * */

int nanofuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    int nitems , i;
    struct nanofs_filedir_handle fh_vector[MAX_DIRENTRIES];
    struct nanofs_filedir_handle *dir_handle;

    log_debug("nanofuse_readdir: path='%s'",path);

    dir_handle =(struct nanofs_filedir_handle *)fi->fh;

    nitems = nanofs_list_dir(&nanofuse_CONTEXT->fs_hd, dir_handle, fh_vector,
                MAX_DIRENTRIES);

    if(nitems >= 0 )
        for(i=0; i<nitems; i++)
        {
            log_debug("nanofuse_readdir: calling filler for '%s'",
                    fh_vector[i].f_dir_node.d_fname);
            if (filler(buf, (char *)fh_vector[i].f_dir_node.d_fname, NULL, 0) != 0) {
                log_debug(" nanofuse_readdir: filler returned buffer full");
                return -ENOMEM;
            }
        }
    return -retstat;
}

/** Release directory
 *  Free the allocated dir_handle in opendir
 */
int nanofuse_releasedir(const char *path, struct fuse_file_info *fi)
{

    log_debug("nanofuse_releasedir: path='%s'",path);
    // Release directory handle allocated in readdir()
    free((struct nanofs_filedir_handle *)fi->fh);

    return 0;
}

/** Synchronize directory contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data
 *
 */
int nanofuse_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{
    log_debug("nanofuse_fsyncdir: path='%s'",
	    path, datasync, fi);
    return 0;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 *
 * Undocumented but extraordinarily useful fact:  the fuse_context is
 * set up before this function is called, and
 * fuse_get_context()->private_data returns the user_data passed to
 * fuse_main().  Really seems like either it should be a third
 * parameter coming in here, or else the fact should be documented
 * (and this might as well return void, as it did in older versions of
 * FUSE).
 *
 */

void *nanofuse_init(struct fuse_conn_info *conn)
{
    log_debug("nanofuse_init: rootdir='%s'", nanofuse_CONTEXT->rootdir);

	if(nanofs_open_dev(nanofuse_CONTEXT->rootdir, & nanofuse_CONTEXT->fs_hd) != 0)
	    log_error("nanofuse_init: nanofs_open_dev failed");

	// filesystem can handle write size larger than 4kB
	conn->capable |= FUSE_CAP_BIG_WRITES;
	conn->max_write = 65536;

    return nanofuse_CONTEXT;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 */
void nanofuse_destroy(void *userdata)
{

	log_debug("nanofuse_destroy: closing device");

	nanofs_close_dev(&nanofuse_CONTEXT->fs_hd);

}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 */
int nanofuse_access(const char *path, int mask)
{
    int retstat = 0;
    struct nanofs_filedir_handle fd_hd;
    log_debug("nanofuse_access: path='%s', mask=0%o)", path, mask);

    retstat = nanofs_lookup_absolute(&nanofuse_CONTEXT->fs_hd,path,&fd_hd);

    if (retstat != 0)
        log_error("nanofuse_access: failed");

    return -retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 */
int nanofuse_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int retstat = 0;
    struct nanofs_filedir_handle *file_handle;

    file_handle = malloc(sizeof(struct nanofs_filedir_handle));

    retstat = nanofs_create_file(&nanofuse_CONTEXT->fs_hd, path, file_handle);

    if (retstat == 0)
    {
        log_debug("nanofuse_create: path='%s', mode=0%03o", path, mode);
        fi->fh = (uint64_t)file_handle;
    }
    else
    {
        log_error("nanofuse_create: cannot create file");
        free(file_handle);
    }

    return -retstat;
}

/**
 * Change the size of an open file
 *
 * This method is called instead of the truncate() method if the
 * truncation was invoked from an ftruncate() system call.
 *
 */
int nanofuse_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    int retstat = ENOTSUP;

    log_debug("nanofuse_ftruncate: path='%s', offset=%lld", path, offset);

    log_error("nanofuse_ftruncate: not implemented");

    return -retstat;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Since it's currently only called after nanofuse_create() and the file is
 * open, I ought to be able to just use the 'fi->fh'
 *
 */

int nanofuse_fgetattr(const char *path, struct stat *statbuf,
        struct fuse_file_info *fi)
{
    int retstat;

    log_debug("nanofuse_fgetattr: path='%s'", path);
    retstat = nanofuse_buildstatbuf((struct nanofs_filedir_handle *) fi->fh,
            statbuf);


    return -retstat;
}

struct fuse_operations nanofuse_oper = {
  .getattr = nanofuse_getattr,
  .readlink = nanofuse_readlink,
  .mknod = nanofuse_mknod,
  .mkdir = nanofuse_mkdir,
  .unlink = nanofuse_unlink,
  .rmdir = nanofuse_rmdir,
  .symlink = nanofuse_symlink,
  .rename = nanofuse_rename,
  .link = nanofuse_link,
  .chmod = nanofuse_chmod,
  .chown = nanofuse_chown,
  .truncate = nanofuse_truncate,
  .utime = nanofuse_utime,
  .open = nanofuse_open,
  .read = nanofuse_read,
  .write = nanofuse_write,
  /** Just a placeholder, don't set */ // huh???
  .statfs = nanofuse_statfs,
  .flush = nanofuse_flush,
  .release = nanofuse_release,
  .fsync = nanofuse_fsync,
  .setxattr = nanofuse_setxattr,
  .getxattr = nanofuse_getxattr,
  .listxattr = nanofuse_listxattr,
  .removexattr = nanofuse_removexattr,
  .opendir = nanofuse_opendir,
  .readdir = nanofuse_readdir,
  .releasedir = nanofuse_releasedir,
  .fsyncdir = nanofuse_fsyncdir,
  .init = nanofuse_init,
  .destroy = nanofuse_destroy,
  .access = nanofuse_access,
  .create = nanofuse_create,
  .ftruncate = nanofuse_ftruncate,
  .fgetattr = nanofuse_fgetattr
};


void nanofuse_usage()
{
    fprintf(stderr,
            "usage:  nanofuse [fuse-options] nanofs-image mount-point\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    int fuse_stat,i;
    struct nanofuse_state *nanofuse_data;
    char *new_argv[argc+1];

    // nanofuse doesn't do any access checking on its own (the comment
    // blocks in fuse.h mention some of the functions that need
    // accesses checked -- but note there are other functions, like
    // chown(), that also need checking!).  Since running nanofuse as root
    // will therefore open Metrodome-sized holes in the system
    // security, we'll check if root is trying to mount the filesystem
    // and refuse if it is.  The somewhat smaller hole of an ordinary
    // user doing it with the allow_other flag is still there because
    // I don't want to parse the options string.
    if ((getuid() == 0) || (geteuid() == 0)) {
    	fprintf(stderr, "Running nanofuse as root opens unacceptable"
    			" security holes\n");
	return 1;
    }




    // Perform some sanity checking on the command line
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
    	nanofuse_usage();

    nanofuse_data = malloc(sizeof(struct nanofuse_state));
    if (nanofuse_data == NULL) {
    	perror("main: nanofuse_data malloc error");
    	return 1;
    }

    // Pull the rootdir out of the argument list and save it in my
    // internal data
    nanofuse_data->rootdir = realpath(argv[argc-2], NULL);
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;

    // Force single thread in fuse operation. See doc about NanoFS
    new_argv[0] = argv[0];
    new_argv[1] = "-s";
    // Copy args
    for(i = 2;i < argc + 1 ;i++)
        new_argv[i] = argv[i-1];


    // turn over control to fuse
    log_debug("main: Call to fuse_main");
    fuse_stat = fuse_main(argc + 1, new_argv, &nanofuse_oper, nanofuse_data);
    log_debug("main: fuse_main returned %d\n", fuse_stat);

    return fuse_stat;
}
