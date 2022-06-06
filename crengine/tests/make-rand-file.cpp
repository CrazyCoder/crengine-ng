/**
 *  crengine-ng
 *  Copyright (c) 2022 Aleksey Chernov
 *
 *  \file make_rand_file.cpp
 *  \brief crengine-ng unit test module.
 *  Create file with random contents.
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

#include "make-rand-file.h"

#include <lvstreamutils.h>
#include <crlog.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

static void fill_rand_string(char* str, unsigned int size);

namespace crengine_ng
{

namespace unittesting
{

#ifdef _WIN32
#define EOL_CHARS_SZ 2
#define EOL_CHARS    "\r\n"
#else
#define EOL_CHARS_SZ 1
#define EOL_CHARS    "\n"
#endif

#define TEXT_LINE_SZ 80

bool makeRandomFile(const char* fname, bool text_mode, uint64_t size) {
    LVStreamRef stream = LVOpenFileStream(fname, LVOM_WRITE);
    if (stream.isNull()) {
        CRLog::error("Can't create file \"%s\"!", fname);
        return false;
    }
    srand(time(0));
    lverror_t ret;
    bool res = true; // successfull by default
    lvsize_t dw = 0;
    if (text_mode) {
        char line[TEXT_LINE_SZ + 1];
        char eolchars[EOL_CHARS_SZ + 1];
        strncpy(eolchars, EOL_CHARS, EOL_CHARS_SZ);
        eolchars[EOL_CHARS_SZ] = 0;
        uint64_t lines_count = size / (TEXT_LINE_SZ + EOL_CHARS_SZ);
        int last_line_sz = (int)(size - lines_count * (TEXT_LINE_SZ + EOL_CHARS_SZ));
        for (uint64_t i = 0; i < lines_count; i++) {
            fill_rand_string(line, TEXT_LINE_SZ);
            ret = stream->Write(line, TEXT_LINE_SZ, &dw);
            if (LVERR_OK == ret)
                ret = stream->Write(eolchars, EOL_CHARS_SZ, &dw);
            if (LVERR_OK != ret) {
                CRLog::error("Write error!");
                res = false;
                break;
            }
        }
        if (last_line_sz > 0) {
            fill_rand_string(line, last_line_sz);
            ret = stream->Write(line, last_line_sz, &dw);
            if (LVERR_OK != ret) {
                CRLog::error("Write error!");
                res = false;
            }
        }
    } else {
        uint64_t count = 0;
        unsigned char d;
        while (count < size) {
            d = (unsigned char)(255 * ((int64_t)rand()) / RAND_MAX);
            ret = stream->Write(&d, 1, &dw);
            if (LVERR_OK != ret) {
                CRLog::error("Write error!");
                res = false;
                break;
            }
            count++;
        }
    }
    stream.Clear();
    return res;
}

} // namespace unittesting

} // namespace crengine_ng

static void fill_rand_string(char* str, unsigned int size) {
    unsigned int i = 0;
    int c;
    while (i < size) {
        c = (int)(62 * ((int64_t)rand()) / RAND_MAX);
        // c in range [0..62]
        if (c == 0)
            // space
            c = ' ';
        else if (c >= 1 && c <= 10)
            // '0'..'9'
            c = '0' + (c - 1);
        else if (c >= 11 && c <= 36)
            // 'A'..'Z'
            c = 'A' + (c - 11);
        else // 37..62
            // 'a'..'z'
            c = 'a' + (c - 37);
        str[i] = (char)c;
        i++;
    }
    str[i] = 0;
}
