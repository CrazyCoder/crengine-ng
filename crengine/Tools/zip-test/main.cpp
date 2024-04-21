/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2021,2022 Aleksey Chernov <valexlin@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

#include <lvstring.h>
#include <lvstring32collection.h>
#include <lvstream.h>
#include <lvstreamutils.h>
#include <lvcontaineriteminfo.h>
#include <crlog.h>

#include <stdio.h>

#define TEST_ZIP64_EXTRACT 0

int main(int argc, char* argv[]) {
    const char* fname;
#if TEST_ZIP64_EXTRACT == 1
    const char* inner_fname = 0;
#endif
    if (argc > 1) {
        fname = argv[1];
#if TEST_ZIP64_EXTRACT == 1
        if (argc > 2)
            inner_fname = argv[2];
#endif
    } else {
        printf("You must specify path to archive!\n");
        return 1;
    }

    CRLog::setStdoutLogger();
    CRLog::setLogLevel(CRLog::LL_TRACE);

    lString32Collection list;
    LVStreamRef stream = LVOpenFileStream(fname, LVOM_READ);
    LVContainerRef arc;
    if (!stream.isNull()) {
        arc = LVOpenArchieve(stream);
        if (!arc.isNull()) {
            // convert
            for (int i = 0; i < arc->GetObjectCount(); i++) {
                const LVContainerItemInfo* item = arc->GetObjectInfo(i);
                if (item->IsContainer())
                    continue;
                list.add(item->GetName());
                list.add(lString32::itoa((lUInt64)item->GetSize()));
                list.add(lString32::itoa((lUInt64)item->GetPackSize()));
            }
        } else {
            printf("Failed to open archive!\n");
            return 1;
        }
    } else {
        printf("Failed to open file!\n");
        return 1;
    }

#if TEST_ZIP64_EXTRACT == 1
    if (!arc.isNull()) {
        if (NULL != inner_fname) {
            printf("Open file inside archive...\n");
            lString32 inner_fname32 = LocalToUnicode(lString8(inner_fname));
            LVStreamRef inner_stream = arc->OpenStream(inner_fname32.c_str(), LVOM_READ);
            if (!inner_stream.isNull()) {
                printf("  ok\n");
                LVStreamRef out_stream = LVOpenFileStream(inner_fname32.c_str(), LVOM_WRITE);
                if (!out_stream.isNull()) {
                    printf("output file opened...\n");
                    lUInt8 buff[4096];
                    lvsize_t ReadSize;
                    lvsize_t WriteSize;
                    while (true) {
                        if (inner_stream->Read(buff, 4096, &ReadSize) != LVERR_OK) {
                            printf("Read error!\n");
                            break;
                        }
                        if (ReadSize > 0) {
                            if (out_stream->Write(buff, ReadSize, &WriteSize) != LVERR_OK) {
                                printf("Write error!\n");
                                break;
                            }
                        } else {
                            break;
                        }
                        if (ReadSize < 4096) {
                            break;
                        }
                    }
                    printf("  done.\n");
                } else {
                    printf("Failed top open output file!\n");
                }
            } else {
                printf("  failed\n");
            }
            fflush(stdout);
        } else {
            printf("Inner filename must be specified from command line!\n");
        }
    }
#endif // TEST_ZIP64_EXTRACT == 1

#if 1
    printf("Archive contents:\n");
    for (int i = 0; i < list.length() / 3; i++) {
        lString32 name = list[i * 3];
        lInt64 size;
        lInt64 pack_size;
        if (!list[i * 3 + 1].atoi(size))
            size = 0;
        if (!list[i * 3 + 2].atoi(pack_size))
            pack_size = 0;
        printf("  %s: size=%lld, pack_size=%lld\n", LCSTR(name), size, pack_size);
    }
#endif
    return 0;
}
