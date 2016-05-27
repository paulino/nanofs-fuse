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

#include <config.h>

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

#include "nanofs.h"
#include "nanofs_io.h"
#include <endian.h>

#ifdef __BYTE_ORDER
#if __BYTE_ORDER == __BIG_ENDIAN
#error "Big edian CPU not supported"
#elif __BYTE_ORDER == __LITTLE_ENDIAN

#elif __BYTE_ORDER == __PDP_ENDIAN
#error "3412 PDP/ARM endian not supported"
#else
#error "Endian determination failed"
#endif
#endif

#define _(x) gettext(x)

int check_mount(char *device_name);
int do_format(char* device, char* volname, int blk_size);

//getopt functions and vars
int getopt(int argc, char * const argv[], const char *optstring);

void print_version();

extern char *optarg;
extern int optind;

int global_verbose = 0;

/** Help str */
static const char *UsageStr = "Usage: %s [OPTION...] file or device \n"
        "Options\n"
        "\t-b <block-size in bytes>. Valid:1, 512, 1024\n"
        "\t-h Show help\n"
        "\t-r Nanofs revision number\n"
        "\t-S Write superblock\n"
        "\t-l <volumelabel> \n"
        "\t-V Version number\n"
        "\t-v Increase verbosity\n";

int main(int argc, char **argv) {

    setlocale( LC_ALL, "");
    bindtextdomain("nanofs", "/usr/share/locale");
    textdomain("nanofs");

    char volumelabel[NANOFS_MAXFILENAME] = "NanosFs Default Volume";
    char *devicestring = NULL;

    int error = EXIT_SUCCESS;
    int show_help = argc <= 1;
    int optc;
    int boption = 0;
    int loption = 0;
    int block_size = 512;

    while ((optc = getopt(argc, argv, "b:l:vV")) != -1) {
        switch (optc) {
        case ':':
            fprintf(stderr, "** Error: Argument missing, see usage.\n");
            break;
        case 'b':

            if (optarg) {
                if ((block_size = (unsigned int) strtol(optarg, NULL, 0))
                        == 0) {
                    fprintf(stderr, "** Error: Bad -b option argument\n");
                    error = 1;
                } else if (block_size && !(block_size & (block_size - 1))) {
                    boption = 1;
                } else if (block_size == 1) {
                    boption = 1;
                } else {
                    fprintf(stderr,
                      "** Error: -b option must be a a power of 2 or 1 byte\n");
                    error = 1;

                }
            } else {
                fprintf(stderr, "**Error: -b option argument is missing\n");
                error = 1;
            }

            break;

        case 'V':
            printf("mkfs.nanofs Version %s\n", VERSION);
            return EXIT_SUCCESS;
        case 'v':
            print_version();
            global_verbose = 1;
            break;
        case 'h':
            break;
        case 'r':
            break;

        case 'l':
            if (optarg) {
                if (strlen(optarg) > NANOFS_MAXFILENAME) {
                    fprintf( stderr, "** Error: Bad -l option argument, "
                            "Volume Label must be < %d chars\n",
                    NANOFS_MAXFILENAME);
                    error = EXIT_FAILURE;
                } else {
                    strcpy(volumelabel, optarg);
                    loption = 1;
                }

            } else {
                fprintf(stderr, "** Error: -l option argument is missing\n");
                error = EXIT_FAILURE;
            }
            break;
        case 'S':
            error = EXIT_FAILURE;

            fprintf(stderr, "** Warning: Write superblock not implemented\n");
            break;
        case '?':
            show_help = 1;
            break;
        default:
            error = EXIT_FAILURE;
            show_help = 1;
            break;
        }
    }

    if ((argc - optind) <= 0)  // Block device
            {
        error = EXIT_FAILURE;
        show_help = 1;
    }
    if (!error && !show_help) {
        if (global_verbose) {
            if (!boption)
                printf("Using default block size of 512 bytes\n");
            else
                printf("%d bytes block size will be used\n", block_size);
            if (!loption)
                printf("Empty Volume Label, default will be used\n");
            else
                printf("Custom Volume label will be used: %s", volumelabel);
        }
        devicestring = strdup(argv[optind]);

        if (check_mount(devicestring) != 0) // Check if filesystem is mounted
            error = EXIT_FAILURE;
        if (!error) {
            error = do_format(devicestring, volumelabel, block_size);
        }
    }
    if (show_help)
        printf(UsageStr, argv[0]);

    return error;

}

/* Check to see if the specified device is currently mounted - abort if it is */

int check_mount(char *device_name) {
    FILE *f;
    struct mntent *mnt;

    if ((f = setmntent( MOUNTED, "r")) == NULL)
        return -1;
    while ((mnt = getmntent(f)) != NULL)
        if (strcmp(device_name, mnt->mnt_fsname) == 0) {
            fprintf( stderr, "**Error: %s contains a mounted file system.",
                    device_name);
            return -1;
        }
    endmntent(f);
    return 0;
}

// NanosFS Format
int do_format(char* device, char* volname, int blk_size) {
    //int err;
    int fd;
    unsigned long long dev_size;
    int blk_bits;
    off_t current_off, next_off;

    struct nanofs_superblock sb;
    struct nanofs_dir_node dn;
    struct nanofs_data_node db;

    if (global_verbose) {
        printf("Creating new file system:\n");
        printf(" - Device/File: %s\n", device);
        printf(" - Block size: %d\n", blk_size);
        printf(" - Volname: %s\n", volname);
    }

    // fd = open(device, O_FSYNC  | O_DIRECT | O_EXLOCK | O_RDWR, 0);
    fd = open(device, O_FSYNC | O_RDWR, 0);
    if (fd < 0) {
        fprintf( stderr, "Open failed.\n");
        perror(device);
        return EXIT_FAILURE;
    }
    if (global_verbose)
        printf("Open device success.\n");

    //Get device Info

    dev_size = lseek64(fd, 0, SEEK_END);

    if (global_verbose)
        printf(" - Detected device size bytes %llu\n", dev_size);

    // Filling superblock

    sb.s_magic = NANOFS_MAGIC;
    if (blk_size == 512) {
        sb.s_blocksize = 1;
        blk_bits = 9; // 2^9=512
    } else {
        fprintf( stderr, "** Error: Block size of %d is not supported\n",
                blk_size);
        close(fd);
        return EXIT_FAILURE;
    }

    current_off = 0;
    sb.s_revision = NANOFS_REVISION;
    sb.s_alloc_ptr = 1; // Root on block 1
    sb.s_free_ptr = 2;
    sb.s_fs_size = dev_size >> blk_bits;
    sb.s_extra_size = 0; // Not implemented

    if (nanofs_write_sb(fd, current_off, &sb) == -1) {
        fprintf( stderr, "** Error writing superblock\n");
        close(fd);
        return EXIT_FAILURE;
    }
    if (global_verbose)
        printf("Superblock written\n");
    // Root dir entry
    current_off += 1 << blk_bits;
    dn.d_flags = (1 << NANOFS_FLG_FTYPE); // Set directory entry as directory (root)
    dn.d_next_ptr = 0;
    dn.d_data_ptr = 0;
    dn.d_meta_ptr = 0;
    dn.d_fname_len = strlen(volname);
    memset(dn.d_fname, 0, NANOFS_MAXFILENAME);
    strcpy((char *)dn.d_fname, volname);
    if (nanofs_write_dir_node(fd, current_off, &dn) != 0) {
        fprintf( stderr, "** Error: Fail while writing root directory\n");
        close(fd);
        return EXIT_FAILURE;
    }
    if (global_verbose)
        printf("Root directory and label written\n");

    // Free blocks
    if (global_verbose)
        printf("Free blocks:\n");
    current_off += 1 << blk_bits;
    while (current_off < dev_size) {
        if (dev_size - current_off <= 0xFFFFFFFFL - NANOFS_HEADER_DATA_NODE_SIZE) // Free space fits on one free node
        {
            db.d_next_ptr = 0;
            db.d_len = (dev_size - current_off) - NANOFS_HEADER_DATA_NODE_SIZE;
            next_off = current_off + db.d_len + NANOFS_HEADER_DATA_NODE_SIZE;
        } else // Split free space in several nodes
        {
            next_off = current_off + (1L << 32); // next 4G bytes
            db.d_next_ptr = next_off >> blk_bits;
            db.d_len = (1L << 32) - NANOFS_HEADER_DATA_NODE_SIZE;
        }

        if (nanofs_write_data_node(fd, current_off, &db) != 0) {
            close(fd);
            fprintf( stderr, "** Error writing free blocks\n");
            return EXIT_FAILURE;
        }
        if (global_verbose)
            printf("- free node offset 0x%16.16lx size %u bytes\n", current_off,
                    db.d_len);
        current_off = next_off;
    }

    if (global_verbose)
        printf("Free blocks written\n");
    close(fd);
    if (global_verbose)
        printf("Nanofs filesystem successfully created\n");
    return EXIT_SUCCESS;

}

void print_version() {
    printf("mknanofs (%s)\n", PACKAGE_STRING);
}

