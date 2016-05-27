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

    @author Paulino Ruiz de Clavijo Vázquez <paulino@dte.us.es>
    @version 0.4
    @date 2012-05-12
    @brief Low level device functions

******************************************************************************/
 
#ifndef __NANOFS_IO_H__
#define __NANOFS_IO_H__

struct nanofs_superblock;
struct nanofs_dir_node;
struct nanofs_data_node;

/* Low level device access 
 * All functions return 0 on success, < 0 on fail */

int nanofs_read_sb(int fd, off_t offset,struct nanofs_superblock *sb);
int nanofs_write_sb(int fd, off_t offset, struct nanofs_superblock *sb);

int nanofs_read_dir_node(int fd, off_t offset, struct nanofs_dir_node *dn);
int nanofs_read_data_node(int fd, off_t offset, struct nanofs_data_node *db);


int nanofs_write_dir_node(int fd, off_t offset, struct nanofs_dir_node *dn);
int nanofs_write_data_node(int fd, off_t offset, struct nanofs_data_node *db);

/* Raw write/read to device*/
int nanofs_write_dev( int fd, off_t offset, const void *buf, int size );
int nanofs_read_dev ( int fd, off_t offset, void *buf, int size );

#endif
