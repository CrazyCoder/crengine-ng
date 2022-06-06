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
 *  the Free Software Foundation, version 2 of the License.
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

#if USE_ZLIB != 1
#error This module does not work without zlib!
#endif

#include <zlib.h>

#include <string.h> // for memcmp()

namespace crengine_ng
{

namespace unittesting
{

#define READ_BUFF_SZ 4096

bool compareTwoBinaryFiles(const char* file1, const char* file2) {
    gzFile gzf1 = gzopen(file1, "rb");
    if (gzf1 == NULL)
        return false;
    gzFile gzf2 = gzopen(file2, "rb");
    if (gzf2 == NULL) {
        gzclose(gzf1);
        return false;
    }
    int eof1, eof2;
    unsigned char buf1[READ_BUFF_SZ];
    unsigned char buf2[READ_BUFF_SZ];
    bool diff = false;
    bool error = false;
    unsigned int read1_sz = READ_BUFF_SZ;
    unsigned int read2_sz = READ_BUFF_SZ;
    for (;;) {
        // read parts
        eof1 = gzeof(gzf1);
        eof2 = gzeof(gzf2);
        if (eof1 != eof2) {
            diff = true;
            break;
        }
        if (eof1 || eof2)
            break;
        int ret1 = gzread(gzf1, buf1, read1_sz);
        int ret2 = gzread(gzf2, buf2, read2_sz);
        if (ret1 < 0 || ret2 < 0) {
            error = true;
            break;
        }
        if (read1_sz != read2_sz) {
            diff = true;
            break;
        }
        // compare parts
        if (diff)
            break;
        if (memcmp(buf1, buf2, read1_sz) != 0) {
            diff = true;
            break;
        }
    }
    gzclose(gzf1);
    gzclose(gzf2);
    return !error && !diff;
}

} // namespace unittesting

} // namespace crengine_ng
