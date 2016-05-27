# Nano File System - Prerelease Version

NanoFS is a lightweight file system intended to be used with microcontrollers
or directly from a HDL design. You can get detailed information at 
[doc]/doc directory.
 
## Compiling and installing on GNU/Linux:

Get a copy of sources from git or a tarball distribution

Install dependencies. Here's an example for Debian/Ubuntu:

    sudo apt-get install build-essential autoconf libtool pkg-config libfuse-dev

After installing dependencies, compile and install in the usual way:

    ./configure
    make
    make install

To uninstall all components:

    make uninstall
    
## Usage

For a reference of all available commands, see the 'nanofuse' man page, or
follow this Example of use:

Making a new file system into a file or device:
 
    mkfs.nanofs -v /dev/sdXX

Mounting the new file system on some place:

    mkdir nanofsimage
    nanofuse /dev/sdxx nanofsimage
    
Now do something with the mounted file system, like tests to check it works:
    
    cd nanofsimage
    cp -r /home/user/test .
    diff -rq /home/user/test test

To unmount the file system, the command is:

    fusermount -u nanofsimage
    
For debug purpose an extra tool `nanofs.dump` is available to check the 
internal layout of the filesystem. The internal layout is described in
documentation at [doc]/doc
 
## Limitations 

NanoFuse is a fuse implementation of NanoFS. Current version is only for test 
purpose and has some limitations:

- File metadata not implemented: UID,GID, date stamp ...
- Defragmentation on fly not implemented
- fsck tool is not finished
- nanofuse runs in single thread mode

## License

GNU GENERAL PUBLIC LICENSE, Version 3, 29 June 2007.
See the [COPYING]/COPYING for the complete text.


## Authors

Paulino Ruiz de Clavijo Vázquez <paulino@dte.us.es>

Enrique Ostúa Aragüena <ostua@dte.us.es>


