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

    @file nanofs_filedir.h
    @author Paulino Ruiz de Clavijo Vázquez <paulino@dte.us.es>
    @date 2016-05-20
    @version revision: 0.4
    @brief Nanofs file/dir functions

*****************************************************************************/

#ifndef __NANOFS_FILEDIR__
#define __NANOFS_FILEDIR__

#include<nanofs.h>

#define NANOFS_NODETYPEDIR  0
#define NANOFS_NODETYPEDATA 1

#define NANOFS_MAX_PATH (NANOFS_MAXFILENAME+1)*100

/** Handle for device operations */
struct nanofs_fs_handle {
    char *h_dev_name;
    int  h_fd;                      ///< Open device/file descriptor
    int  h_block_bits;              ///< Helper for shift bits
    struct nanofs_superblock h_sb;  ///< Copy of the device superblock
    int h_error;                    ///< Last operation error, 0 not error

};


/** Handle for files and dirs */
struct nanofs_filedir_handle {
    __u32 f_blk_no;                     ///< Block number of dir entry
    struct nanofs_dir_node f_dir_node;  ///< File copy of dir entry
};



/* File system operations */
int nanofs_open_dev(char *dev,struct nanofs_fs_handle *handle);
int nanofs_close_dev(struct nanofs_fs_handle *handle);
int nanofs_get_block_bits(struct nanofs_superblock *sb);
long int nanofs_free(struct nanofs_fs_handle *fs_hd);

/* File operations */
int nanofs_create_file(struct nanofs_fs_handle *fs_hd, const char *file_path,
        struct nanofs_filedir_handle * fh_out);
int nanofs_truncate(struct nanofs_fs_handle *fs_hd,
        struct nanofs_filedir_handle *fh, size_t size);
int nanofs_read(struct nanofs_fs_handle *fs_hd,struct nanofs_filedir_handle *fh,
        char *buf, size_t size, off_t offset);
int nanofs_write(struct nanofs_fs_handle *fs_hd, struct nanofs_filedir_handle *fh,
        const char *buf, size_t size, off_t offset);

long int nanofs_get_file_size(struct nanofs_fs_handle *hd,
        struct nanofs_filedir_handle *fh);



/* Directory operations */
int nanofs_list_dir(struct nanofs_fs_handle *hd,
        struct nanofs_filedir_handle *dir_hd,
        struct nanofs_filedir_handle fh_vec[], int vec_size);
int nanofs_lookup(struct nanofs_fs_handle *fs_hd, char *file_name,
        struct nanofs_filedir_handle *dir_hd,
        struct nanofs_filedir_handle *dir_hd_out);
int nanofs_mkdir(struct nanofs_fs_handle *fs_hd,
        char *dir_name,struct nanofs_filedir_handle *parent_dir_hd);

int nanofs_rmdir(struct nanofs_fs_handle *fs_hd,char *dir_name,
        struct nanofs_filedir_handle *parent_dir_hd);

int nanofs_lookup_absolute(struct nanofs_fs_handle *fs_hd,
        const char *absolute_path, struct nanofs_filedir_handle *hd_out);

int nanofs_rm(struct nanofs_fs_handle *fs_hd,char *file_name,
        struct nanofs_filedir_handle *parent_dir_hd);


/* Low level utilities */
inline int nanofs_read_dir_node_b(struct nanofs_fs_handle *fs_hd,__u32 blk_no,
        struct nanofs_dir_node *dn_out);
inline int nanofs_read_data_node_b(struct nanofs_fs_handle *fs_hd,__u32 blk_no,
        struct nanofs_data_node *dn_out);



#endif
