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

    @file nanofs.h
    @author Paulino Ruiz de Clavijo Vázquez <pruiz@us.es>
    @date 2012-05-12
    @version 0.3

******************************************************************************/
 
#ifndef __LINUX_NANOFS_FS_H
#define __LINUX_NANOFS_FS_H

#define NANOFS_MAGIC           0x4e61
#define NANOFS_MAXFILENAME     255
#define NANOFS_DEFAULT_BLKSIZE 512
#define NANOFS_REVISION        0


// Nodes size in bytes, required due C structs are not mem aligned:
// size_of (struct nanofs_dir_node) != (1+4+4+4+1+1)
#define NANOFS_SB_SIZE          32


/** Size of dir_node without fname field */
#define NANOFS_HEADER_DIR_NODE_SIZE    14

/** Header size of a data_node, without data field */
#define NANOFS_HEADER_DATA_NODE_SIZE  8

/* Flags for f_type field in directory node structure */
#define NANOFS_FLG_FTYPE  0 // bit 0: 1 for directory, 0 reg file

#define DN_ISREG(dn) (!(dn.d_flags & 0x01))  ///< Is dir node regular file?
#define DN_ISDIR(dn) (  dn.d_flags & 0x01 )  ///< Is dir node directory?


/**
 * The starting point of NanoFS is the superblock located at byte offset 0 of
 * memory/device. Superblock links with two data structures: data blocks
 * allocated and free data blocks.
 */
struct nanofs_superblock
{
    __u16 s_magic;
    __u8  s_blocksize;  ///< Block size. Set to 1 for 512 and fit in SD Cards
    __u8  s_revision;   ///< Filesystem revision, only 0 is valid
    __u32 s_alloc_ptr;  ///< Absolute blockNo of allocated root entry
    __u32 s_free_ptr;   ///< Absolute blockNo of start of free blocks list
    __u32 s_fs_size;    ///< Filesystem size in blocks
    __u16 s_extra_size; ///< Extra superblock size in bytes
};

/** Be carefully reading this struct from device
 * is not aligned to 8bits in memory. In disk must be aligned to 8bits
 * */

struct nanofs_dir_node
{
    __u8  d_flags;      ///< Directory entry flags
    __u32 d_next_ptr;   ///< Absolute blockNo of next directory entry
    __u32 d_data_ptr;   ///< Absolute blockNo of first child data block
    __u32 d_meta_ptr;   ///< Absolute blockNo of first metadata block
    __u8  d_fname_len;  ///< Length in bytes of filename
    __u8  d_fname[NANOFS_MAXFILENAME]; //< Name of file
};

/* This struct is aligned */

struct nanofs_data_node
{
    __u32 d_next_ptr; ///< Absolute blockNo of the next data_node
    __u32 d_len;      ///< Data length
};


#endif
