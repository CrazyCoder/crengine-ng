/**
 *  crengine-ng
 *  Copyright (c) 2022 Aleksey Chernov
 *
 *  \file compare-two-binfiles.cpp
 *  \brief crengine-ng unit test module.
 *  Comparison of two binary files.
 *  One or both files may be a gzip archive.
 *  In this case, the contents of the archive are compared.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "compare-two-binfiles.h"

#include <crsetup.h>

//#ifdef USE_ZLIB
//#undef USE_ZLIB
//#endif

#if USE_ZLIB == 1
#include <zlib.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#include <string.h> // for memcmp()

namespace crengine_ng
{

namespace unittesting
{

#define READ_BUFF_SZ 4096

bool compareTwoBinFiles(const char* file1, const char* file2) {
#if USE_ZLIB == 1
    gzFile gzf1 = gzopen(file1, "rb");
    if (gzf1 == NULL)
        return false;
    gzFile gzf2 = gzopen(file2, "rb");
    if (gzf2 == NULL) {
        gzclose(gzf1);
        return false;
    }
    int eof1, eof2;
    int dw1, dw2;
#else
    int fd1 = open(file1, O_RDONLY);
    if (fd1 == -1)
        return false;
    int fd2 = open(file2, O_RDONLY);
    if (fd2 == -1) {
        close(fd1);
        return false;
    }
    ssize_t dw1, dw2;
#endif
    unsigned char buff1[READ_BUFF_SZ];
    unsigned char buff2[READ_BUFF_SZ];
    bool diff = false;
    bool error = false;
    for (;;) {
#if USE_ZLIB == 1
        eof1 = gzeof(gzf1);
        eof2 = gzeof(gzf2);
        if (eof1 != eof2) {
            diff = true;
            break;
        }
        if (eof1 || eof2)
            break;
        dw1 = gzread(gzf1, buff1, READ_BUFF_SZ);
        dw2 = gzread(gzf2, buff2, READ_BUFF_SZ);
        if (dw1 < 0 || dw2 < 0) {
            error = true;
            break;
        }
        if (dw1 != dw2) {
            diff = true;
            break;
        }
        if (memcmp(buff1, buff2, dw1) != 0) {
            diff = true;
            break;
        }
#else
        dw1 = read(fd1, buff1, READ_BUFF_SZ);
        dw2 = read(fd2, buff2, READ_BUFF_SZ);
        if (dw1 == -1 || dw2 == -1) {
            error = true;
            break;
        }
        if (dw1 != dw2) {
            diff = true;
            break;
        }
        if (memcmp(buff1, buff2, dw1) != 0) {
            diff = true;
            break;
        }
        if (dw1 < READ_BUFF_SZ) {
            // eof
            break;
        }
#endif
    }
#if USE_ZLIB == 1
    gzclose(gzf1);
    gzclose(gzf2);
#else
    close(fd1);
    close(fd2);
#endif
    return !error && !diff;
}

} // namespace unittesting

} // namespace crengine_ng
