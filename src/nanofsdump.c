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
    @date 2016-23-05
    @version 0.4

******************************************************************************/


#define _LARGEFILE64_SOURCE

#include<config.h>

#include <asm/types.h>
#include <sys/mount.h>
#include <mntent.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <libintl.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include "nanofs.h"
#include "nanofs_io.h"


int dump_nanofs(char *device);
int dump_directory(int fd_dev, int blk_bits, int dir_blkno, int level);
int dump_free_blocks(int fd, struct nanofs_superblock *sb, int blk_bits);
int dump_data_blocks(int fd_dev, int blk_bits, int blkno, int level);
int dump_file_contents(int fd_dev, int blk_bits, int data_blkno);


char *ltoh(__u64 bytes);
void dump_hex(void *buf,int size);
void print_version();
void print_tabs(int n);


/** Help str */

static const char *UsageStr = "Usage: nanofs-dump [OPTION...] file or device \n"
        "Options\n"
        "\t-c Dump file contents\n"
        "\t-V Version number\n"
        "\t-v Increase verbosity\n";




// Global options
int global_verbose = 0; // Global verbosity
int global_file_contents = 0; // Dump file contents

int main(int argc, char **argv)
{
    //int show_help = 0;
    int optc;

    char *basename_str = strdup(argv[0]);
    basename(argv[0]);

    setlocale(LC_ALL, "");
    bindtextdomain("nanofs", "/usr/share/locale");
    textdomain("nanofs");
    print_version(basename_str);
    while ((optc = getopt(argc, argv, "cb:v:V")) != -1)
    {
        switch (optc)
        {
        case 'c':
            global_file_contents = 1;
            break;
        case ':':
            printf("Argument missing, see usage.\n");
            break;
        case '?':
        default:
            printf("%s",UsageStr);
            return EXIT_FAILURE;
            break;
        case 'V':
            printf("nanofs-dump Version %s\n", VERSION);
            return EXIT_SUCCESS;
        case 'v':
            global_verbose = 1;
            break;
        }
    }
    // remaining non-option arguments
    if(optind >= argc )
    {
        printf("Missing argument in command line\n");
        return EXIT_FAILURE;
    }

    dump_nanofs(argv[optind]);
    free(basename_str);
    return EXIT_SUCCESS;
}

int dump_nanofs(char *device_str)
{
    int err = 0;
    unsigned long long dev_size = 1000000L;
    int fd;
    int blk_size = 0;
    int blk_bits;

    struct nanofs_superblock sb;
    struct nanofs_dir_node dn;

    fd = open(device_str, O_FSYNC | O_RDONLY, 0);
    if (fd < 0)
    {
        fprintf(stderr, "** Error cannot open device %s\n", device_str);
        return -1;
    }

    dev_size = lseek64(fd, (off_t) 0, SEEK_END);
    printf("Opened device/file of size %lld bytes (%s)\n", dev_size,
            ltoh(dev_size));

    if (nanofs_read_sb(fd, (off_t) 0, &sb) != 0)
    {
        fprintf(stderr, "** Error cannot read superblock %s\n", device_str);
        err = -1;
    }
    // Dump superblock
    if (err == 0)
    {
        printf("Superblock data:\n");
        printf(" - Magic number:        %4x", sb.s_magic);
        if (sb.s_magic != NANOFS_MAGIC)
            printf(" [ERROR]\n");
        else
            printf(" [OK]\n");
        printf(" - Block size:        ");
        if (sb.s_blocksize == 0)
        {
            blk_size = 1;
            blk_bits = 0;
        }
        else if (sb.s_blocksize == 1)
        {
            blk_size = 512;
            blk_bits = 9;
        }
        else
            blk_size = -1;
        if (blk_size > 0)
            printf(" %5d [OK]\n", blk_size);
        else
            printf(" %2x [ERROR]\n", sb.s_blocksize);
        printf(" - Filesystem revision: %4d", sb.s_revision);
        if (sb.s_revision != 0)
            printf(" [ERROR]\n");
        else
            printf(" [OK]\n");
        printf(" - Root on block:      0x%8.8X\n", sb.s_alloc_ptr);
        printf(" - Filesystem size:    0x%8.8X blocks, ", sb.s_fs_size);
        printf(" %llu Bytes (%s)\n", (unsigned long long)(sb.s_fs_size) * blk_size,
                ltoh((unsigned long long)(sb.s_fs_size) * blk_size));
        printf(" - Free blocks at block:0x%4x", sb.s_free_ptr);
        if (sb.s_free_ptr > sb.s_fs_size)
        {
            printf("[ERROR] ** Beyond of device\n");
        }
        else
            printf("[OK]\n");

        printf(" - Superblock extrasize:%4d Bytes\n", sb.s_extra_size);

    }

    // Dump free blocks
    if (err == 0)
        err = dump_free_blocks(fd, &sb, blk_bits);
    // Dump directories

    if (err == 0)
    {
        printf("Root directory         ");
        if (nanofs_read_dir_node(fd, sb.s_alloc_ptr << blk_bits, &dn) != 0)
        {
            printf(" [READ Error]\n");
            err = -2;
        }
        else
            printf(" [Read OK]\n");
    }
    if (err == 0)
    {
        if (DN_ISDIR(dn))
            printf(" - Entry type set as directory     [OK]\n");
        else
            printf(" - Entry type not set as directory [ERROR]\n");
        if (dn.d_next_ptr != 0)
            printf(" - Next entry on root dir          [ERROR]\n");
        else
            printf(" - Next entry on root dir          [OK]\n");
        printf(" - Root child dir entries at block: 0x%x\n", dn.d_data_ptr);
        printf(" - Volumen label: %s\n", dn.d_fname);
    }

    // Recursive dump dirs
    err = dump_directory(fd, blk_bits, sb.s_alloc_ptr, 0);


    close(fd);
    return err;
}



/** Recursive dump directory
 * @return number of errors found, 0 on success */
int dump_directory(int fd_dev, int blk_bits, int dir_blkno, int level)
{
    struct nanofs_dir_node dir_node;

    int current_blkno;
    int err = 0;
    print_tabs(level);
    printf(" - Dump directory on level %d,", level);
    if (nanofs_read_dir_node(fd_dev, (off_t)(dir_blkno) << blk_bits,
            &dir_node) != 0)
    {
        printf(" [READ Error]\n");
        return 1;
    }
    printf(" name '%s' \n", dir_node.d_fname);

    print_tabs(level);
    printf(" - Directory entry at block 0x%x (offset 0x%x), data:\n",
            dir_blkno, dir_blkno << blk_bits);

    if (DN_ISDIR(dir_node))
    {
        print_tabs(level);
        printf("   + Node type (flag):      DIR \n");
    }
    else
    {
        print_tabs(level);
        printf("   + Node type (flag):      REG_FILE [**Error]\n");
        err++;
    }
    print_tabs(level);
    printf("   + Next dir node at block: 0x%x\n", dir_node.d_next_ptr);
    print_tabs(level);
    printf("   + Metadata at block:      0x%x\n", dir_node.d_meta_ptr);
    print_tabs(level);
    printf("   + FName length:            %-d\n", dir_node.d_fname_len);
    print_tabs(level);
    printf("   + FName:                  '%s'\n", dir_node.d_fname);
    print_tabs(level);
    printf("   + Child nodes at block:   0x%x\n", dir_node.d_data_ptr);

    if (err)
        return err;

    // Listing dir contents
    current_blkno = dir_node.d_data_ptr; // First directory entry
    while (current_blkno != 0)
    {

        if (nanofs_read_dir_node(fd_dev,
                (off_t)(current_blkno) << blk_bits, &dir_node) != 0)
        {
            printf("** IO Error reading directory entry\n");
            return ++err;
        }

        if (DN_ISDIR(dir_node))
        {
            // Recursive call
            err += dump_directory(fd_dev, blk_bits, current_blkno, level + 1);
        }
        else if (DN_ISREG(dir_node))
        {
            print_tabs(level + 1);
            printf(" - Directory entry at block 0x%x (offset 0x%x), data:\n",
                    current_blkno, current_blkno << blk_bits);
            print_tabs(level + 1);
            printf("   + Node type (flag):       REG_FILE\n");
            print_tabs(level + 1);
            printf("   + Next dir node at block: 0x%x\n", dir_node.d_next_ptr);
            print_tabs(level + 1);
            printf("   + Metadata at block:      0x%x\n", dir_node.d_meta_ptr);
            print_tabs(level + 1);
            printf("   + FName length:            %-d\n", dir_node.d_fname_len);
            print_tabs(level + 1);
            printf("   + FName:                  '%s'\n", dir_node.d_fname);
            print_tabs(level + 1);
            printf("   + Data start at block:    0x%x (offset 0x%x)\n",
                    dir_node.d_data_ptr,dir_node.d_data_ptr << blk_bits);
            // Data blocks
            if (dir_node.d_data_ptr != 0)
            {
                if (global_file_contents)
                    err += dump_file_contents(fd_dev, blk_bits,
                            dir_node.d_data_ptr);
                else
                    err += dump_data_blocks(fd_dev, blk_bits,
                            dir_node.d_data_ptr,level+1);
            }
        }
        current_blkno = dir_node.d_next_ptr;
    }
    return err;
}

/** Dump list of data blocks
 * @return Number of errors found
 * **/

int dump_data_blocks(int fd_dev, int blk_bits, int blkno, int level)
{
    struct nanofs_data_node data_node;
    __u32 current_blk = blkno;
    while(current_blk !=0 )
    {
        if (nanofs_read_data_node(fd_dev, current_blk << blk_bits,
                &data_node) != 0)
        {
            printf("** IO Error reading data block, blk_no = 0x%8.8X\n",
                    current_blk);
            return 1;
        }
        print_tabs(level + 1);
        printf("     > Data block (next,len): 0x%8.8X, ", data_node.d_next_ptr);
        printf("%d Bytes (%s)\n", data_node.d_len, ltoh(data_node.d_len));
        current_blk = data_node.d_next_ptr;
    }

    return 0;
}

int dump_file_contents(int fd_dev, int blk_bits, int data_blkno)
{
    char buf[1024];
    int i;
    int err = 0;
    struct nanofs_data_node data_nd;
    int bytes;
    if (nanofs_read_data_node(fd_dev, (off_t)(data_blkno) << blk_bits,
            &data_nd) != 0)
    {
        printf("** Error reading data block\n");
        return ++err;
    }

    bytes = read(fd_dev, buf, 1024);
    for (i = 0; i < data_nd.d_len; i++)
    {
        if (i % 64 == 0)
            printf("\n         ");
        if (buf[i % 1024] > 32 && buf[i % 1024] < 126)
            printf("%c", buf[i % 1024]);
        else
            printf(".");
        bytes--;
        if (bytes == 0)
            bytes = read(fd_dev, buf, 1024);
    }
    printf("\n");
    return err;
}

int dump_free_blocks(int fd, struct nanofs_superblock *sb, int blk_bits)
{
    int err = 0;
    struct nanofs_data_node free_node;
    __u32 blk_no;
    __u64 free_bytes = 0;
    unsigned long long fragments = 0;
    blk_no = sb->s_free_ptr;
    printf("Reading free blocks\n");
    printf(" - First free block at block 0x%x (offset 0x%x),"
            " dump legend (blk_no,size):\n", blk_no, blk_no << blk_bits);

    while (blk_no > 0)
    {
        if (nanofs_read_data_node(fd, (off_t)(blk_no) << blk_bits,
                &free_node) != 0)
        {
            printf("** IO Error reading a free block at blk_no 0x%8.8X\n",
                    blk_no);
            break;
        }
        printf("   + Data node (next,len): 0x%8.8X, %u (%s)\n", blk_no,
                free_node.d_len, ltoh(free_node.d_len));
        blk_no = free_node.d_next_ptr;
        fragments++;
        free_bytes += free_node.d_len;

    }
    printf("\n");
    printf(" - Free bytes counted: %llu (%s)\n", free_bytes, ltoh(free_bytes));
    printf(" - Free fragments: %llu\n", fragments);
    return err;
}

/* Formating funcs */
void print_version()
{
    printf("nanofsdump (%s)\n",PACKAGE_STRING);
}

char *ltoh(__u64 bytes)
{
    static char str[256];

    if(bytes < (1L << 10))
        sprintf(str,"%llu Bytes",bytes);
    else if(bytes < (1L << 20))
        sprintf(str,"%.1f KiBytes",((float)bytes) / ((float)(1L << 10)));
    else if(bytes < (1L << 30))
        sprintf(str,"%.1f MiBytes",((float)bytes) / ((float)(1L << 20)));
    else if(bytes < (1L << 40))
        sprintf(str,"%.1f GiBytes",((float)bytes) / (float)(1LL << 30));

    return str;
}

void dump_hex(void *buf,int size)
{
    int i;
    for(i=0;i<size;i++)
        printf("%2.2x ",((unsigned char *)buf)[i]);
}

/** format util */
void print_tabs(int n)
{
    int t;
    for (t = 0; t < n; t++)
        printf("    ");
}
