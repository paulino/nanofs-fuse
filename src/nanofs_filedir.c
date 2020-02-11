/*****************************************************************************
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

    @file nanofs_filedir.c
    @author Paulino Ruiz de Clavijo Vázquez <pruiz@us.es>
    @date 2016-05-26
    @version revision: 0.4
    @brief Nanofs file/dir functions

*****************************************************************************/


/* This directive is mandatory to deal with large files
  _LARGE_FILES must be used before the inclusion of any header files,
  then the large-file programming environment is enabled and off_t is defined
  to be a signed, 64-bit long long.
 */
#define _LARGEFILE64_SOURCE

/** Mandatory to use O_DIRECT flag, before including 'fnctl.h' */
#define _GNU_SOURCE

#include <asm/types.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>

#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


#include "nanofs.h"
#include "nanofs_io.h"
#include "nanofs_filedir.h"
#include "log.h"





static __u32 nanofs_alloc_data_node(struct nanofs_fs_handle *hd,
        struct nanofs_filedir_handle *fh, __u32 size,
        struct nanofs_data_node *dn_out);

static int nanofs_alloc_dir_node(struct nanofs_fs_handle *fs_hd,
        struct nanofs_filedir_handle *parent_dir_hd,
        struct nanofs_filedir_handle *dir_node_io);

static int nanofs_free_dir_node( struct nanofs_fs_handle *fs_hd,
        struct nanofs_filedir_handle *parent_hd,
        struct nanofs_filedir_handle *fd_hd);




/** Open device/file and init the filesystem handle
 * @return -1 on error | 0 on success
 * */
int nanofs_open_dev(char *dev_name, struct nanofs_fs_handle *hd)
{
    //hd->h_fd=open ( dev_name, O_FSYNC  | O_RDWR, 0 );
    hd->h_fd = open(dev_name, O_RDWR);
    hd->h_error = 0;
    if (hd->h_fd < 0)
    {
        log_error("nanofs_open_dev: Cannot open device for R/W");
        hd->h_error = errno;
        return -1;
    }
    if (hd->h_error == 0) // Ok
    {
        hd->h_dev_name = strdup(dev_name);
        // Reading superblock
        if (nanofs_read_sb(hd->h_fd, (off_t) 0, &hd->h_sb) != 0)
        {
            log_error("nanofs_open_dev: Error reading superblock");
            hd->h_error = EIO;
        }
    }

    // Check superblock
    if (hd->h_error == 0 && (hd->h_sb).s_magic != NANOFS_MAGIC)
    {
        log_error("nanofs_open_dev: Error in superblock, bad magic number");
        hd->h_error = EIO;
    }
    // Get block bits
    if (hd->h_error == 0)
    {
        hd->h_block_bits = nanofs_get_block_bits(&(hd->h_sb));
        if (hd->h_block_bits < 0)
        {
            log_error("nanofs_open_dev: Error in superblock getting blockbits");
            hd->h_error = EIO;
        }
    }

    if (hd->h_error != 0)
    {
        nanofs_close_dev(hd);
        return -1;
    }
    return 0;
}
/** Close the device associated to the filesystem handle
 *
 * */
int nanofs_close_dev(struct nanofs_fs_handle *hd)
{
    if (hd->h_fd < 0)
        return -1;
    close(hd->h_fd);
    free(hd->h_dev_name);
    hd->h_fd = -1;
    return 0;

}

/** Get bits from block size
 * @return -1 on error
 * */
int nanofs_get_block_bits(struct nanofs_superblock *sb)
{
    if (sb->s_blocksize == 0)
        return 0;
    else if (sb->s_blocksize == 1)
        return 9;
    else
        return -1;
}




/** Read dir_node given a blk_no
 * @return 0 on success | on error return -1 and set fs_hd->error
 * */
int nanofs_read_dir_node_b(struct nanofs_fs_handle *fs_hd,__u32 blk_no,
        struct nanofs_dir_node *dn_out)
{
    if( nanofs_read_dir_node(fs_hd->h_fd, blk_no << fs_hd->h_block_bits,
            dn_out) != 0 )
    {
        fs_hd->h_error = EIO;
        return -1;
    }
    return 0;
}
/** Read data_node given a blk_no
 * @return 0 on success | on error return -1 and set fs_hd->errorned
 * */

int nanofs_read_data_node_b(struct nanofs_fs_handle *fs_hd,__u32 blk_no,
        struct nanofs_data_node *dn_out)
{
    if(nanofs_read_data_node(fs_hd->h_fd,
            (off_t)(blk_no) << fs_hd->h_block_bits, dn_out) != 0)
    {
         fs_hd->h_error = EIO;
         return -1;
    }
     return 0;
}

/** Write dir_node given a blk_no
 * @return 0 on success | on error return -1 and set fs_hd->errorned
 *  */

inline int nanofs_write_dir_node_b(struct nanofs_fs_handle *fs_hd,__u32 blk_no,
        struct nanofs_dir_node *dn)
{
    if(nanofs_write_dir_node(fs_hd->h_fd, blk_no << fs_hd->h_block_bits,dn) !=0 )
    {
        fs_hd->h_error = EIO;
        return -1;
    }
    return 0;
}
/** Write data_node given a blk_no
 * @return 0 on success | on error return -1 and set fs_hd->errorned
 * */
inline int nanofs_write_data_node_b(struct nanofs_fs_handle *fs_hd,__u32 blk_no,
        struct nanofs_data_node *dn)
{

    if( nanofs_write_data_node(fs_hd->h_fd,
            (off_t)(blk_no) << fs_hd->h_block_bits, dn) != 0)
    {
        fs_hd->h_error = EIO;
        return -1;
    }
    return 0;
}

/** Util to calc the blocks required for a give size */
inline __u32 nanofs_blocks_for_size(struct nanofs_fs_handle *fs_hd,__u32 size)
{
    // Test if need an extra block
    __u32 mask = (1 << fs_hd->h_block_bits ) - 1;
    if ( (mask & size) != 0 )
        return (size >> fs_hd->h_block_bits) + 1;
    else
        return (size >> fs_hd->h_block_bits);
}


/** Util to get bytes from a data node
 * */
/*
inline int nanofs_read_data_node_data(struct nanofs_fs_handle *fs_hd,
        __u32 blk_no, char *buf, __u32 size, __u32 offset)
{
    return nanofs_read_dev(fs_hd->h_fd,
            (blk_no << fs_hd->h_block_bits) + NANOFS_HEADER_DATA_NODE_SIZE + offset,
            buf,size);
}
*/




/** Returns free space available on file system
 * @return Free space in bytes, -1 on fail and hd->h_error is set
 */

long int nanofs_free(struct nanofs_fs_handle *hd)
{
    long int free_size = 0;
    __u32 blk_no = hd->h_sb.s_free_ptr;
    struct nanofs_data_node data_node;
    while (blk_no > 0)
    {
        if (nanofs_read_data_node_b(hd, blk_no, &data_node) < 0)
        {
            log_error("nanofs_free: error reading free space structure");
            hd->h_error = EIO;
            return -1;
        }
        free_size += data_node.d_len;
        blk_no = data_node.d_next_ptr;
    }
    return free_size;
}

/** Get a handle for a given path, returned handle may be a file or a directory
 *
 * Util for navigate an absolute path and get a file/dir handle
 *
 * @param absolute_path It must start by '/' and not end with '/'
 * @param hd_out Out param with the handle of the dir or file found
 * @return 0 on success | EIO when IO error
 *      | ENOENT not valid directory or file in path
 *      | EINVAL invalid path
 */

int nanofs_lookup_absolute(struct nanofs_fs_handle *fs_hd,
        const char *absolute_path, struct nanofs_filedir_handle *fdh_out)
{
    int retstat;
    struct nanofs_filedir_handle dir_handle;
    int pos,i;
    char dir_name[NANOFS_MAXFILENAME + 1];
    if (absolute_path[0] != '/')
    {
        log_error("nanofs_chpath: dir name must be absolute path");
        return EINVAL;
    }
    // Start at root dir
    fdh_out->f_blk_no = fs_hd->h_sb.s_alloc_ptr;
    if(nanofs_read_dir_node_b(fs_hd,fdh_out->f_blk_no,
           &(fdh_out->f_dir_node)) != 0)
    {
        log_error("nanofs_chpath: cannot read root dir");
        return EIO;
    }

    pos = 1;
    i = 0 ;
    if ( absolute_path[1] == 0) // Return root dir_entry
        return 0;
    while(i < NANOFS_MAXFILENAME)  // loop for navigate dir by dir in path
    {
        dir_name[i] = 0;
        if (absolute_path[pos] == '/' || absolute_path[pos] == 0)
        {
            // change to next dir
            memcpy(&dir_handle,fdh_out,sizeof(struct nanofs_filedir_handle));
            retstat = nanofs_lookup(fs_hd, dir_name, &dir_handle,  fdh_out);
            if (retstat != 0)
                return retstat;
            else if (absolute_path[pos] == 0)
                return 0; // Success

            // else follow the path
            i = 0;
            pos++;
            // the current dir_entry must be a directory
            if (!DN_ISDIR(fdh_out->f_dir_node))
                return ENOENT;
        }
        dir_name[i] = absolute_path[pos];
        pos++;
        i++;
    }
    return ENOENT;
}
/**
 * @return 0 on success
 *      | ENOSPC when no free space is available
 *      | EIO on error
 *      | EISDIR Already exists
 */

int nanofs_mkdir(struct nanofs_fs_handle *fs_hd,
        char *dir_name,struct nanofs_filedir_handle *parent_dir_hd)
{
    int retstat = 0;
    struct nanofs_filedir_handle new_dir_hd;

    retstat = nanofs_lookup(fs_hd, dir_name, parent_dir_hd, &new_dir_hd);

    // Check if directory already exists
    if (retstat == 0) // Found
        return EISDIR;
    else if(retstat == -1)
        return EIO;

    new_dir_hd.f_dir_node.d_flags = 1; // Is directory
    new_dir_hd.f_dir_node.d_data_ptr = 0;
    new_dir_hd.f_dir_node.d_meta_ptr = 0;
    new_dir_hd.f_dir_node.d_next_ptr = 0;

    if (strlen(dir_name) > NANOFS_MAXFILENAME)
    {
        log_error("nanofs_mkdir: dir name truncated, max filename length reached");
        new_dir_hd.f_dir_node.d_fname_len = 255;
    }
    else
        new_dir_hd.f_dir_node.d_fname_len = strlen(dir_name);
    strncpy((char *)new_dir_hd.f_dir_node.d_fname, dir_name, NANOFS_MAXFILENAME);
    retstat = nanofs_alloc_dir_node(fs_hd, parent_dir_hd, &new_dir_hd);
    return retstat;
}

/** Remove dir entry in a given directory
 *  @param dir_name dir name to look up in parent_dir_hd
 *  @return 0 on success | ENOENT dir not found |
 *      EIO when IO error | ESTALE file system error
 * */

int nanofs_rmdir(struct nanofs_fs_handle *fs_hd,char *dir_name,
        struct nanofs_filedir_handle *parent_dir_hd)
{
    int retstat;
    struct nanofs_filedir_handle fd_hd;
    retstat = nanofs_lookup(fs_hd,dir_name,parent_dir_hd, &fd_hd);
    if(retstat !=0 )
        return retstat;
    return nanofs_free_dir_node(fs_hd,parent_dir_hd,&fd_hd);
}

/** Remove file in the given directory
 * @param file_name
 * @param parent_dir_hd parent directory
 * @return 0 on success | EIO |  ENOENT lookup error
 *      | ESTALE free dir node error
 */
int nanofs_rm(struct nanofs_fs_handle *fs_hd,char *file_name,
        struct nanofs_filedir_handle *parent_dir_hd)
{
    int retstat;
    struct nanofs_filedir_handle fd_hd;
    //struct nanofs_data_node data_nd;
    //__u32 data_blkno;

    retstat = nanofs_lookup(fs_hd,file_name,parent_dir_hd, &fd_hd);

    if(retstat != 0)
        return retstat;

    // Free data nodes calling truncate to 0

    retstat = nanofs_truncate(fs_hd, &fd_hd, 0);
    if(retstat != 0)
        return retstat;

    /*// File found, try to free data nodes
    data_blkno = fd_hd.f_dir_node.d_data_ptr;
    while(data_blkno != 0) // File has data blocks
    {
        if (nanofs_read_data_node_b(fs_hd, data_blkno, &data_nd) != 0)
        {
           log_error("nanofs_rm: IO error freeing data nodes of a file");
           return EIO;
        }
        if(nanofs_free_data_node(fs_hd, data_blkno,&data_nd) != 0 )
           return EIO;
        data_blkno=data_nd.d_next_ptr;  // Next block
    }*/

    // Free dir entry
    if(nanofs_free_dir_node(fs_hd,parent_dir_hd,&fd_hd) !=0 )
        return ESTALE;
    return 0;
}


/**
 * List current dir filling fh_vec
 *
 * @param dir_hd Directory handle to be read
 * @param fh_vec Output array to fill
 * @param vec_size Size of the output array
 * @return number of items | -1 on error
 * */
int nanofs_list_dir(struct nanofs_fs_handle *hd,
        struct nanofs_filedir_handle *dir_hd,
        struct nanofs_filedir_handle fh_vec[], int vec_size)
{
    struct nanofs_dir_node dir_n;
    int items = 0;
    //int block_bits = hd->h_block_bits;
    __u32 blk_no;
    //int errs = 0;

    // Read parent dir node
    /*if (nanofs_read_dir_node_b(hd, dir_hd->f_blk_no, &dir_n) != 0)
    {
        log_error(stderr, "nanofs_list_dir: cannot read directory node");
        return -1;
    }*/
    // Reading childs dir nodes
    blk_no = dir_hd->f_dir_node.d_data_ptr;
    while (blk_no != 0 && items < vec_size)
    {
        if (nanofs_read_dir_node_b(hd,blk_no, &dir_n) != 0)
        {
            log_error("nanofs_list_dir: Cannot read one child directory node");
            return -1;
        }
        memcpy(&(fh_vec[items].f_dir_node), &dir_n,
                sizeof(struct nanofs_dir_node));
        fh_vec[items].f_blk_no = blk_no;
        //fi_vec[items].f_size=0;
        //errs -= nanofs_get_file_size(&fi_vec[items], hd);
        items++;
        blk_no = dir_n.d_next_ptr;
    }
    return items;
}

/** Lookup for a dir entry into a given directory
 * @param file_name Name of the file to search in the directory
 * @param dir_hd Directory where the search takes place
 * @param dir_hd_out On success is filled
 * @return -1 on error | 0 on success | ENOENT if not found
 */

int nanofs_lookup(struct nanofs_fs_handle *fs_hd, char *file_name,
        struct nanofs_filedir_handle *dir_hd,
        struct nanofs_filedir_handle *dir_hd_out)
{
    int blk_no;
    blk_no = dir_hd->f_dir_node.d_data_ptr;
    while (blk_no != 0)
    {
        if (nanofs_read_dir_node_b(fs_hd,blk_no,&(dir_hd_out->f_dir_node)) != 0)
        {
            log_error("nanofs_lookup: 'list_dir' error reading directory node");
            return -1;
        }
        if (strcmp(file_name, (char *)dir_hd_out->f_dir_node.d_fname) == 0) // Found
        {
            dir_hd_out->f_blk_no = blk_no;
            return 0;
        }
        blk_no = dir_hd_out->f_dir_node.d_next_ptr;
    }
    // Not found
    return ENOENT;
}


/** Create an empty file
 * @param file_path Full file path
 * @param fh_out File handle out parameter
 * @return 0 on success | EIO when IO error
 *      | ENOENT not valid directory or in path
 *      | EINVAL invalid path
 *      | ENOSPC when no free space is available
 */

int nanofs_create_file(struct nanofs_fs_handle *fs_hd, const char *file_path,
        struct nanofs_filedir_handle * fh_out)
{
    int retstat = 0;
    struct nanofs_filedir_handle parent_dir_hd;

    char *dirc = strdup(file_path);
    char *basec = strdup(file_path);
    char *dir_name = dirname( dirc );
    char *base_name = basename( basec );


    retstat = nanofs_lookup_absolute(fs_hd,dir_name,&parent_dir_hd);

    if(retstat == 0 && !DN_ISDIR(parent_dir_hd.f_dir_node))
        retstat = ENOENT;
        // Check if parent dir it is a directory
    if(retstat == 0)
    {
        fh_out->f_dir_node.d_flags = 0; // Regular file
        fh_out->f_dir_node.d_data_ptr = 0;
        fh_out->f_dir_node.d_meta_ptr = 0;
        fh_out->f_dir_node.d_next_ptr = 0;
        fh_out->f_dir_node.d_fname_len = strlen(base_name);
        strncpy((char *)(fh_out->f_dir_node.d_fname), base_name, NANOFS_MAXFILENAME);
        retstat = nanofs_alloc_dir_node(fs_hd, &parent_dir_hd, fh_out);
    }

    free(dirc);
    free(basec);
    return retstat;
}

/** Calc file size
 * @TODO: A directory has zero bytes?
 * @param fi_fh File handle
 * @return file size | on error 'hd->h_error' is set
 */
long int nanofs_get_file_size(struct nanofs_fs_handle *hd,
        struct nanofs_filedir_handle *fh)
{
    long int size;
    struct nanofs_data_node data_nd;
    if (DN_ISDIR(fh->f_dir_node))
        return 0;

    if (fh->f_dir_node.d_data_ptr == 0)
        return 0;
    else if (nanofs_read_data_node_b(hd, fh->f_dir_node.d_data_ptr, &data_nd) != 0)
    {
        log_error("nanofs_get_file_size: IO error getting file size");
        return 0;
    }
    size = data_nd.d_len;
    while (data_nd.d_next_ptr != 0)
    {
        if (nanofs_read_data_node_b(hd,data_nd.d_next_ptr, &data_nd) != 0)
        {
            log_error("nanofs_get_file_size: IO error getting file size");
            return 0;
        }
        size += data_nd.d_len;
    }
    return size;
}

/** Calc space available on file system in bytes
 * @return -1 on error
 * */
/*__s64 nanofs_get_free_size(struct nanofs_fs_handle *hd)
{
    __s64 size = 0;
    __u32 blkno;
    struct nanofs_data_node free_node;

    blkno = hd->h_sb.s_free_ptr;
    while (blkno != 0)
    {
        if (nanofs_read_data_node_b(hd,blkno, &free_node) != 0)
        {
            log_error("nanofs_get_free_size: IO error getting file size");
            return -1;
        }
        //size += NANOFS_HEADER_DIR_NODE_SIZE + free_node.d_len;
        size += free_node.d_len;
        blkno = free_node.d_next_ptr;
    }
    return size;
}*/



/**
 * Allocate and write new dir_node.
 * fd_handle_io->dir_node_in data is written at the end of parent_dir_hd
 * Used to create new dirs and files.
 *
 *
 * @param fd_handle_io 'fd_handle_io->f_dir_node' contains input dir_node
 *      to be written to the device. On success 'fd_handle_io->f_blk_no'
 *      it set to the new block_no allocated
 *      Data in 'fd_handle_io->f_dir_node' must be
 *      initialized at least with 'd_next_ptr=0'
 *
 * @return 0 on success | ENOSPC when no free space is available | EIO on error
 */

static int nanofs_alloc_dir_node(struct nanofs_fs_handle *fs_hd,
        struct nanofs_filedir_handle *parent_dir_hd,
        struct nanofs_filedir_handle *fd_handle_io)
{
    struct nanofs_data_node data_nd;
    struct nanofs_dir_node dir_node;
    __u32 new_blkno, current_blkno;

    if(fs_hd->h_sb.s_free_ptr == 0) // No space left on device
        return ENOSPC;

    // Read free list
    if (nanofs_read_data_node_b(fs_hd,fs_hd->h_sb.s_free_ptr,&data_nd) != 0)
    {
        log_error("nanofs_alloc_dir_node: fail getting free blocks");
        return EIO;
    }

    if ( (data_nd.d_len + NANOFS_HEADER_DATA_NODE_SIZE ) <
                ( (__u32)1 << fs_hd->h_block_bits))
    {
        // This may not happen due block size is 512, the free_node
        // uses at least one 512 block

        // Not enough free space on device to alloc new directory entry
        return ENOSPC;
    }

    if((data_nd.d_len + NANOFS_HEADER_DATA_NODE_SIZE ) ==
                ( (__u32)1 << fs_hd->h_block_bits ))
    {
        // data_node size = 1 block, convert data_node to dir_node
        new_blkno = fs_hd->h_sb.s_free_ptr;
        fs_hd->h_sb.s_free_ptr = data_nd.d_next_ptr;
    }
    else
    {
        // Take part of a data_node, 1 block
        new_blkno = fs_hd->h_sb.s_free_ptr;
        // FIXME: Only allowed 512 bytes block
        fs_hd->h_sb.s_free_ptr = fs_hd->h_sb.s_free_ptr + 1;
        // Reduce free list by one block
        data_nd.d_len = data_nd.d_len - (__u32 ) (1 << fs_hd->h_block_bits);
        // Write the updated free block
        if (nanofs_write_data_node_b(fs_hd,fs_hd->h_sb.s_free_ptr,&data_nd) != 0)
        {
            log_error("nanofs_alloc_dir_node: cannot update freeblocks list, filesystem may be corrupted");
            return EIO;
        }
    }

    // Update superblock
    if (nanofs_write_sb(fs_hd->h_fd, (off_t) 0, &fs_hd->h_sb) != 0)
    {
        log_error("nanofs_alloc_dir_node:nanofs_alloc_dir_node:cannot update superblock, filesystem may be corrupted");
        fs_hd->h_error = EIO;
        return EIO;
    }

    // Add new dir_node at the end of list of parent dir
    if (parent_dir_hd->f_dir_node.d_data_ptr == 0)
    {   // Empty child list parent_dir is updated
        parent_dir_hd->f_dir_node.d_data_ptr = new_blkno;
        if( nanofs_write_dir_node_b(fs_hd,parent_dir_hd->f_blk_no,
                &parent_dir_hd->f_dir_node) != 0 )
        {
            log_error("nanofs_alloc_dir_node: cannot update parent dir node");
            return EIO;
        }
    }
    else
    {
        // follow list starting at first dir_node
        current_blkno = parent_dir_hd->f_dir_node.d_data_ptr;
        if (nanofs_read_dir_node_b(fs_hd,current_blkno,&dir_node) != 0)
        {
            log_error("nanofs_alloc_dir_node: reading directory fails");
            return EIO;
        }
        while (dir_node.d_next_ptr != 0)
        {
            current_blkno = dir_node.d_next_ptr;
            if (nanofs_read_dir_node_b(fs_hd, current_blkno, &dir_node) != 0)
            {
                log_error("nanofs_alloc_dir_node: reading directory node fails");
                return EIO;
            }
        }
        // Update the last dir_node for add the new dir_node
        dir_node.d_next_ptr = new_blkno;
        if (nanofs_write_dir_node_b(fs_hd,current_blkno,&dir_node) != 0)
        {
            log_error("nanofs_alloc_dir_node: cannot update last dir node");
            return EIO;
        }
    }
    // Write the added dir_node
    fd_handle_io->f_blk_no=new_blkno;
    fd_handle_io->f_dir_node.d_next_ptr = 0;
    if (nanofs_write_dir_node_b(fs_hd,new_blkno,
            &fd_handle_io->f_dir_node) != 0)
    {
        log_error("nanofs_alloc_dir_node: cannot write new dir node");
        return EIO;
    }

    return 0;
}




/** Free dir node
 *  Dir node block is added at ahead of list of free nodes. The linked list of
 *  parent dir is updated.
 *
 *  @param parent_hd    Parent directory of the file
 *  @param fd_hd        File handle to be deleted
 *
 *  @TODO: Add free nodes at the end of free nodes list
 *
 *  @return 0 on success | EIO when IO error |  ESTALE file system error
 * */
int nanofs_free_dir_node( struct nanofs_fs_handle *fs_hd,
        struct nanofs_filedir_handle *parent_hd,
        struct nanofs_filedir_handle *fd_hd)
{
    struct nanofs_filedir_handle prev_fd_hd;
    struct nanofs_data_node data_nd;
    //struct nanofs_dir_node dir_nd;
    //__u32 blk_no;


    if( parent_hd->f_dir_node.d_data_ptr == fd_hd->f_blk_no )
    {
        // The dir_node is the first directory list
        // Update parent dir to next dir_node
        parent_hd->f_dir_node.d_data_ptr = fd_hd->f_dir_node.d_next_ptr;
        if(nanofs_write_dir_node_b(fs_hd,parent_hd->f_blk_no,
                &parent_hd->f_dir_node) != 0)
            return EIO;
    }
    else
    {
        // Search fd_hd in the dir_nodes linked list
        prev_fd_hd.f_blk_no = parent_hd->f_dir_node.d_data_ptr;
        while (prev_fd_hd.f_blk_no !=0 )
        {
            if (nanofs_read_dir_node_b(fs_hd,
                    prev_fd_hd.f_blk_no,&prev_fd_hd.f_dir_node) != 0)
                return EIO;
            if (prev_fd_hd.f_dir_node.d_next_ptr == fd_hd->f_blk_no) // Found
                break;
            prev_fd_hd.f_blk_no = prev_fd_hd.f_dir_node.d_next_ptr;
        }

        if (prev_fd_hd.f_blk_no == 0)
        {
            // Reached end of list, the block is not in list !
            log_error("nanofs_free_dir_node: internal error in directory list");
            return ESTALE;
        }
        // Update previous node dir in linked list
        prev_fd_hd.f_dir_node.d_next_ptr = fd_hd->f_dir_node.d_next_ptr;
        if( nanofs_write_dir_node_b(fs_hd, prev_fd_hd.f_blk_no,
                &prev_fd_hd.f_dir_node) != 0 )
        {
            log_error("nanofs_free_dir_node: free node failed");
            return EIO;
        }
    }

    // Convert dir_node into data_node and add it to free nodes list
    data_nd.d_next_ptr = fs_hd->h_sb.s_free_ptr;
    data_nd.d_len = (1 << fs_hd->h_block_bits) - NANOFS_HEADER_DATA_NODE_SIZE;
    if (nanofs_write_data_node_b(fs_hd,fd_hd->f_blk_no,&data_nd) < 0)
        return EIO;
    // Update superblock
    fs_hd->h_sb.s_free_ptr = fd_hd->f_blk_no;
    if (nanofs_write_sb(fs_hd->h_fd, (off_t) 0, &fs_hd->h_sb) != 0)
    {
        log_error("nanofs_free_dir_node: cannot update superblock, filesystem may be corrupted");
        return EIO;
    }

    return 0;
}



/** Read data from a file
 * @param size It can be greater than the file size
 * @return the number of bytes read, or -1 on error
 * */
int nanofs_read(struct nanofs_fs_handle *fs_hd,
        struct nanofs_filedir_handle *fh, char *buf, size_t size, off_t offset)
{
    struct nanofs_data_node data_node;
    off_t buf_pos,  file_pos, i_offset;
    size_t bytes_left;
    __u32 blk_no;
    int   bytes_to_read;

    blk_no = fh->f_dir_node.d_data_ptr;
    file_pos = 0;
    buf_pos = 0;
    bytes_left = size;

    // When 'blk_no' is 0 end of file has been reached
    while(blk_no != 0 && bytes_left > 0 )
    {

        if(nanofs_read_data_node_b( fs_hd, blk_no , &data_node ) != 0)
            return -1;

        if(file_pos >= offset)
        {
            // Read data from data blocks, file_pos is not used anymore
            if(bytes_left <= data_node.d_len)
                bytes_to_read = bytes_left;
            else
                bytes_to_read = data_node.d_len;

            // Read from device
            if(nanofs_read_dev(fs_hd->h_fd,
                        (blk_no << fs_hd->h_block_bits) +
                        NANOFS_HEADER_DATA_NODE_SIZE,
                        &buf[buf_pos], bytes_to_read) != bytes_to_read)
                return -1;

            bytes_left -= bytes_to_read;
            buf_pos += bytes_to_read;
        }
        else if(data_node.d_len + file_pos > offset)
        {
            // The read start in this data_node
            // Internal offset in this node
            i_offset = offset - file_pos;

            if(data_node.d_len - (__u32)i_offset >= bytes_left)
                bytes_to_read = bytes_left;
            else
                bytes_to_read = data_node.d_len - i_offset;

            if(nanofs_read_dev(fs_hd->h_fd,
                        (blk_no << fs_hd->h_block_bits) +
                        NANOFS_HEADER_DATA_NODE_SIZE + i_offset,
                        &buf[buf_pos], bytes_to_read) != bytes_to_read)
                return -1;

            bytes_left -= bytes_to_read;
            buf_pos += bytes_to_read;
        }

        // go forward through data_nodes list
        blk_no = data_node.d_next_ptr;
        file_pos += data_node.d_len;
    }
    return size - bytes_left;

}

/** Write data to a file
 * @return the number of bytes written | -1 on error
 * */
int nanofs_write(struct nanofs_fs_handle *fs_hd,
        struct nanofs_filedir_handle *fh, const char *buf, size_t size,
        off_t offset)
{
    struct nanofs_data_node data_node;
    __u32 blk_no, bytes_available;
    size_t bytes_written, bytes_left;
    off_t i_offset;

    bytes_left = size;
    i_offset = 0;

    blk_no = fh->f_dir_node.d_data_ptr;
    if(blk_no == 0 && offset != 0)
        // This may not happen. File size = 0 and offset != 0 ?
        return -1;
    while(blk_no != 0)
    {
        // go forward trough linked list
        if(nanofs_read_data_node_b(fs_hd,blk_no,&data_node) != 0)
            return -1;

        if( i_offset + data_node.d_len >= offset)
            break; // write in this data_node
        i_offset += data_node.d_len;
        blk_no = data_node.d_next_ptr;
    }
    if(blk_no != 0 ) // End of file not reached
    {
        // Internal offset in this data_node
        i_offset = offset - i_offset;
        // Bytes in current data_node + spare size
        bytes_available = ( nanofs_blocks_for_size(
                fs_hd, data_node.d_len + NANOFS_HEADER_DATA_NODE_SIZE)
                        << fs_hd->h_block_bits ) - NANOFS_HEADER_DATA_NODE_SIZE
                                - i_offset;
        // Write data in this data_node

        if(bytes_available  >= size)
            // All data fits in this data_node
            bytes_written = size;
        else
            bytes_written = bytes_available;

        // write data
        if( nanofs_write_dev(fs_hd->h_fd,
                (blk_no << fs_hd->h_block_bits) + NANOFS_HEADER_DATA_NODE_SIZE +
                i_offset , buf, bytes_written) != (int)bytes_written)
            return size - bytes_left - bytes_written; // Error

        // update data_node
        data_node.d_len = i_offset + bytes_written;
        if(nanofs_write_data_node_b(fs_hd,blk_no,&data_node) !=0 )
            return -1;

        bytes_left -= bytes_written;

    }

    // Add new data_nodes to file
    while (bytes_left > 0)
    {
        // i_offset is set to buf position to continue writing
        i_offset = size - bytes_left;

        // Appends new block to the end of the file and
        // returns the 'block_no' allocated
        blk_no = nanofs_alloc_data_node(fs_hd,fh, bytes_left, &data_node);
        if ( blk_no == 0) // No block
        {
            log_error("nanofs_write: cannot allocate free space for write a file");
            return -1;
        }
        // Now writes data. The header of the block
        // was written by nanofsmgr_alloc_data_node
        bytes_written = bytes_left > data_node.d_len ?
                data_node.d_len : bytes_left;

        if( nanofs_write_dev(fs_hd->h_fd,
                (blk_no << fs_hd->h_block_bits) +
                NANOFS_HEADER_DATA_NODE_SIZE,
                &buf[i_offset], bytes_written) != (int)bytes_written)
            return -1;

        //  Update data_node. FIXME: Not always required, only when
        //    bytes_written < data_node.d_len
        data_node.d_len = bytes_written;
        if(nanofs_write_data_node_b(fs_hd,blk_no,&data_node) !=0 )
            return -1;


        bytes_left -= bytes_written;
    }

    return size - bytes_left;
}

/** Try to alloc one new block of a given size from free space and
 *  add it at the end of file.
 *  The new empty 'data_node' is appended to the end of file.
 *
 *  The size of the allocated block is <= than the required size,
 *  'dn_out' have the size of the block allocated, this block already has been
 *  written to the device.
 *
 * Required from write()
 *
 * @param fh File handle
 * @param size Required size
 * @param dn_out Datablock allocated, d_out->d_len has the size allocated
 * @return block_no of the data node allocated | 0 on fail, field hd->h_error
 *          is set.
 * */

static __u32 nanofs_alloc_data_node(struct nanofs_fs_handle *hd,
        struct nanofs_filedir_handle *fh, __u32 size,
        struct nanofs_data_node *dn_out)
{
    struct nanofs_data_node free_data_node,data_node;
    __u32 new_blkno,blocks_required,blk_no,blocks;

    if(hd->h_sb.s_free_ptr == 0)
    {
        // No free space on device
        hd->h_error = ENOSPC;
        return 0;
    }

    blocks_required = nanofs_blocks_for_size(hd,NANOFS_HEADER_DATA_NODE_SIZE
            + size);

    // Read first data node of the free list
    new_blkno = hd->h_sb.s_free_ptr;
    if (nanofs_read_data_node_b(hd, new_blkno, &free_data_node) != 0)
        return 0;

    blocks = nanofs_blocks_for_size(hd,NANOFS_HEADER_DATA_NODE_SIZE
            + free_data_node.d_len);

    // Return this node
    if (blocks <= blocks_required )
    {
        hd->h_sb.s_free_ptr = free_data_node.d_next_ptr;
        // Updating superblock
        if (nanofs_write_sb(hd->h_fd, (off_t) 0, &(hd->h_sb)) != 0)
            return 0;
        dn_out->d_len = (blocks << hd->h_block_bits) -
                NANOFS_HEADER_DATA_NODE_SIZE;
        dn_out->d_next_ptr = 0;

        // Add new data_node to the end of the file
        blk_no = fh->f_dir_node.d_data_ptr;
        if(blk_no == 0 )
        {
            // File was empty, update dir node
            fh->f_dir_node.d_data_ptr = new_blkno;
            if(nanofs_write_dir_node_b(hd,fh->f_blk_no,&fh->f_dir_node) !=0 )
                return 0;
        }
        else
        {
            // File was not empty, navigate along data list
            if(nanofs_read_data_node_b(hd,blk_no,&data_node) != 0)
                return 0;
            while (data_node.d_next_ptr != 0 )
            {
                blk_no = data_node.d_next_ptr;
                if(nanofs_read_data_node_b(hd,blk_no,&data_node) != 0)
                {
                    hd->h_error = EIO;
                    return 0;
                }
            }
            // Update the last data_node if the file
            data_node.d_next_ptr = new_blkno;
            if(nanofs_write_data_node_b(hd,blk_no,&data_node) != 0)
                return 0;

        }
        // Write the new data_node
        if(nanofs_write_data_node_b(hd,new_blkno,dn_out) != 0)
            return 0;

        return new_blkno;
    }

    // At this point the free space node is greater than the required size
    // 1. Split free space
    // 2. Update file


    // 1. Split free space, calc where new free block starts
    //    New free space starts at:

    hd->h_sb.s_free_ptr = hd->h_sb.s_free_ptr + blocks_required;
    //    Create new data node for free space
    free_data_node.d_len -= (blocks_required << hd->h_block_bits);
    //    Update superblock
    if (nanofs_write_sb(hd->h_fd, (off_t) 0, &(hd->h_sb)) != 0)
    {
        log_error("nanofs_alloc_data_node: IO Error updating superblock");
        hd->h_error = EIO;
        return 0;
    }
    //  Write new freespace data node
    if(nanofs_write_data_node_b(hd,hd->h_sb.s_free_ptr,&free_data_node) != 0)
        return 0;

    // 2. Create new data node and add it to the end of the file

    dn_out->d_len = size;
    dn_out->d_next_ptr = 0;

    // Update file entry, add data_node to the end of file
    blk_no = fh->f_dir_node.d_data_ptr;
    if(blk_no == 0 )
    {
        // File was empty, only update dir node
        fh->f_dir_node.d_data_ptr = new_blkno;
        if(nanofs_write_dir_node_b(hd,fh->f_blk_no,&fh->f_dir_node) !=0 )
            return 0;
    }
    else
    {
        // File was not empty
        if(nanofs_read_data_node_b(hd,blk_no,&data_node) != 0)
            return 0;

        while (data_node.d_next_ptr != 0 )
        {
            blk_no = data_node.d_next_ptr;
            if(nanofs_read_data_node_b(hd,blk_no,&data_node) != 0)
                return 0;
        }
        // Reached end of file, update last node to add the new node
        data_node.d_next_ptr = new_blkno;
        if(nanofs_write_data_node_b(hd,blk_no,&data_node) != 0)
            return 0;

    }
    // Write new data node
    if(nanofs_write_data_node_b(hd,new_blkno,dn_out) != 0)
         return 0;

    return new_blkno;
}

/** Free data_node, data node is added at ahead of list of free nodes
 *
 * @TODO: Add free nodes to the end of the list
 * @TODO: Make on-fly defragmentation for free nodes
 *
 * @param blkno block number of the data node
 * @param dn data node info
 * @return 0 on success | -1 on fail
 * */

int nanofs_free_data_node(struct nanofs_fs_handle *hd,int blkno,
        struct nanofs_data_node *dn)
{
    struct nanofs_data_node free_nd;
    __u32 blocks;

    // Link the node ahead of free nodes
    blocks = nanofs_blocks_for_size(hd,dn->d_len + NANOFS_HEADER_DATA_NODE_SIZE);

    free_nd.d_next_ptr = hd->h_sb.s_free_ptr;
    // Calc length: expand len to fit block size
    free_nd.d_len = (blocks << hd->h_block_bits) -  NANOFS_HEADER_DATA_NODE_SIZE;

    if (nanofs_write_data_node_b(hd, blkno, &free_nd) != 0)
        return -1;

    hd->h_sb.s_free_ptr = blkno;
    // Update superblock
    if (nanofs_write_sb(hd->h_fd, (off_t) 0, &hd->h_sb) != 0)
    {
        log_error("nanofs_free_data_node: Cannot update superblock, "
                " filesystem may be corrupted");
        return -1;
    }
    return 0;
}


/** Resize file
 * @TODO: Only supported param size = 0
 * @return 0 on succes,  EIO on fail, or ENOTSUP if size != 0
 * */
int nanofs_truncate(struct nanofs_fs_handle *fs_hd,
        struct nanofs_filedir_handle *fh, size_t size)
{
    struct nanofs_data_node dn;
    __u32 blk_no;

    if(size !=0 )
    {
        log_error("nanofs_truncate: not implented trunctate to size %lld",size);
        return ENOTSUP;
    }
    blk_no = fh->f_dir_node.d_data_ptr;
    while(blk_no != 0)
    {
        if(nanofs_read_data_node_b( fs_hd, blk_no , &dn ) != 0)
            return EIO;
        if(nanofs_free_data_node(fs_hd,blk_no,&dn) != 0)
            return EIO;
        blk_no = dn.d_next_ptr;
    }
    // Update file
    fh->f_dir_node.d_data_ptr = 0;

    if(nanofs_write_dir_node_b(fs_hd,fh->f_blk_no,&fh->f_dir_node) != 0)
        return EIO;;
    return 0;

}



