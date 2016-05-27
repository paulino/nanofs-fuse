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

    @file nanofs_io.c
    @author Paulino Ruiz de Clavijo Vázquez <paulino@dte.us.es>
    @version 0.4
    @date 2016-05-20
    @brief Low level device functions

******************************************************************************/

/* This directive is mandatory to deal with large files
 * _LARGE_FILES must be used before the inclusion of any header files,
 * then the large-file programming environment is enabled and off_t is defined
 * to be a signed, 64-bit long long.
 * */

#define _LARGEFILE64_SOURCE

#include <asm/types.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "log.h"
#include "nanofs.h"
#include "nanofs_io.h"



/** Be carefully with memory aligment in C structs
 * @return  0 on success | 'errno' from write on error.
 * */
int nanofs_write_sb(int fd, off_t offset, struct nanofs_superblock *sb)
{
    int size = sizeof ( struct nanofs_superblock );
    int res = nanofs_write_dev ( fd, offset,  sb, size);
    // Superblock struct is mem aligned, do direct write
    if(res != size)
    {
      log_error("nanofs_write_sb: superblock write failed" );
      return -1;
    }
    return 0;
}

/**
 * @return 0 on success, -1 on fail
 */
int nanofs_read_sb(int fd, off_t offset,struct nanofs_superblock *sb)
{
    int size = sizeof ( struct nanofs_superblock );
    if ( lseek64 ( fd, offset, SEEK_SET ) != offset )
    {
        log_error("nanofs_read_sb: cannot read superblock, lseek failed: '%s'",
                strerror ( errno ));
        return -1;
    }
    if (read ( fd, sb, size) != size)
    {
        log_error("nanofs_read_sb: cannot read superblock");
        return -1;
    }
    return 0;
}

/** Write dir node to device
 * @return 0 on success | -1 on error, 'errno' can be used
 * */
int nanofs_write_dir_node(int fd, off_t offset, struct nanofs_dir_node *dn)
{
    __u8 buf[NANOFS_HEADER_DIR_NODE_SIZE];

    buf[0] = dn->d_flags;
    memcpy(&buf[1],&(dn->d_next_ptr),4);
    memcpy(&buf[5],&(dn->d_data_ptr),4);
    memcpy(&buf[9],&(dn->d_meta_ptr),4);
    buf[13] = dn->d_fname_len;

    if ( lseek64 ( fd, offset, SEEK_SET ) != offset )
        return -1;

    if ( write( fd, buf, NANOFS_HEADER_DIR_NODE_SIZE)
            != NANOFS_HEADER_DIR_NODE_SIZE)
        return -1;


    if(write ( fd, &(dn->d_fname), dn->d_fname_len) != dn->d_fname_len)
        return -1;

    return 0;
}

/** Read dir_node from device, low level method
 *
 * @return -1 on error | 0 on success
 */
int nanofs_read_dir_node(int fd, off_t offset,struct nanofs_dir_node *dn)
{
    __u8 buf[NANOFS_HEADER_DIR_NODE_SIZE];

    if ( lseek64 ( fd, offset, SEEK_SET ) != offset )
        return -1;

    if (read(fd, &buf, NANOFS_HEADER_DIR_NODE_SIZE )
            != NANOFS_HEADER_DIR_NODE_SIZE)
        return -1;

    dn->d_flags = buf[0];
    memcpy(&dn->d_next_ptr, &buf [1], 4);
    memcpy(&dn->d_data_ptr, &buf [5], 4);
    memcpy(&dn->d_meta_ptr, &buf [9], 4);
    dn->d_fname_len = buf[13];

    if (read(fd,dn->d_fname,dn->d_fname_len) != dn->d_fname_len)
        return -1;

    dn->d_fname[dn->d_fname_len] = 0;
    return 0;
}

/** Write the header of the 'data_node'
 * @return 0 on success | -1 on error
 * */
int nanofs_write_data_node(int fd, off_t offset, struct nanofs_data_node *dn)
{
    // Direct call to write, struct_data_node is mem aligned
    int res = nanofs_write_dev ( fd, offset,  dn,
            sizeof(struct nanofs_data_node));
    if(res != sizeof(struct nanofs_data_node))
    {
      log_error ("nanofs_write_data_node: data_node writing failed: '%s'",
              strerror ( errno ));
      return -1;
    }
    return 0;
}

/** Read data node header
 * @return -1 on fail, 0 on success
 * */

int nanofs_read_data_node(int fd, off_t offset,struct nanofs_data_node *dn)
{
    int res = nanofs_read_dev ( fd, offset,  dn,
            sizeof(struct nanofs_data_node));
    if(res != sizeof(struct nanofs_data_node))
    {
      log_error ("nanofs_read_data_node: data_node read failed: '%s'",
              strerror ( errno ));
      return -1;
    }
    return 0;

}


/** Writes size bytes from offset
*  @return bytes written on success | -1 on failure
*/
int nanofs_write_dev ( int fd, off_t offset,const void *buf, int size )
{
    if ( lseek64 ( fd, offset, SEEK_SET ) != offset )
        return -1;
    if ( write ( fd, buf, size ) != size )
        return -1;
    return size;
}

/** Read size bytes from offset
 *  @return bytes read on success | -1 on failure
 */
int nanofs_read_dev ( int fd, off_t offset,void *buf, int size )
{
    if ( lseek64 ( fd, offset, SEEK_SET ) != offset )
        return -1;
    if ( read ( fd, buf, size ) != size )
        return -1;
    return size;
}





