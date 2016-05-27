/******************************************************************************

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

******************************************************************************/

#ifndef _PARAMS_H_
#define _PARAMS_H_

// The FUSE API has been changed a number of times.  So, our code
// needs to define the version of the API that we assume.  As of this
// writing, the most current API version is 26

//#define FUSE_USE_VERSION 26


// X/Open and POSIX standards.
// 500 - X/Open 5, incorporating POSIX 1995
// 600 - X/Open 6, incorporating POSIX 2004
// 700 - X/Open 7, incorporating POSIX 2008

#define _XOPEN_SOURCE 500

#include <limits.h>
#include <stdio.h>

/** Used to keep state */
struct nanofuse_state {
    char *rootdir;
    struct nanofs_fs_handle fs_hd; ///< File system handle
};

#define nanofuse_CONTEXT ((struct nanofuse_state *) fuse_get_context()->private_data)

#endif
