/*******************************************************

   CoolReader Engine

   lvstream.cpp

   (c) Vadim Lopatin, 2000-2009
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include <lvstream.h>
#include <crlog.h>

#include "lvdefstreambuffer.h"

#if (USE_SHASUM == 1)
#include <stdio.h>
extern "C" {
#include <sha.h>
}
#endif

#define CRC_BUF_SIZE 16384

/// Get read buffer - default implementation, with RAM buffer
LVStreamBufferRef LVStream::GetReadBuffer(lvpos_t pos, lvpos_t size) {
    LVStreamBufferRef res;
    res = LVDefStreamBuffer::create(LVStreamRef(this), pos, size, true);
    return res;
}

/// Get read/write buffer - default implementation, with RAM buffer
LVStreamBufferRef LVStream::GetWriteBuffer(lvpos_t pos, lvpos_t size) {
    LVStreamBufferRef res;
    res = LVDefStreamBuffer::create(LVStreamRef(this), pos, size, false);
    return res;
}

/// calculate crc32 code for stream, if possible
lverror_t LVStream::getcrc32(lUInt32& dst) {
    dst = 0;
    if (GetMode() == LVOM_READ || GetMode() == LVOM_APPEND) {
        CRLog::debug("LVStream: start to calc CRC32");
        lvpos_t savepos = GetPos();
        lvsize_t size = GetSize();
        lUInt8 buf[CRC_BUF_SIZE];
        SetPos(0);
        lvsize_t bytesRead = 0;
        for (lvpos_t pos = 0; pos < size; pos += CRC_BUF_SIZE) {
            lvsize_t sz = size - pos;
            if (sz > CRC_BUF_SIZE)
                sz = CRC_BUF_SIZE;
            Read(buf, sz, &bytesRead);
            if (bytesRead != sz) {
                SetPos(savepos);
                return LVERR_FAIL;
            }
            dst = lStr_crc32(dst, buf, sz);
        }
        SetPos(savepos);
        CRLog::debug("LVStream: done of CRC32 calculation");
        return LVERR_OK;
    } else {
        // not supported
        return LVERR_NOTIMPL;
    }
}

/// calculate sha256 hash for stream, if possible
lverror_t LVStream::getsha256(lString8& dst) {
#if (USE_SHASUM == 1)
    dst = lString8::empty_str;
    if (GetMode() == LVOM_READ || GetMode() == LVOM_APPEND) {
        CRLog::debug("LVStream: start to calc SHA256...");
        lverror_t res = LVERR_FAIL;
        SHA256Context sha256;
        uint8_t digest[SHA256HashSize] = {};
        // Init hash
        int sharet = SHA256Reset(&sha256);
        if (shaSuccess == sharet) {
            lvpos_t savepos = GetPos();
            lvsize_t size = GetSize();
            lUInt8 buf[CRC_BUF_SIZE];
            SetPos(0);
            lvsize_t bytesRead = 0;
            lverror_t ret = LVERR_OK;
            bool failed = false;
            lvpos_t pos = 0;
            while (pos < size) {
                ret = Read(buf, CRC_BUF_SIZE, &bytesRead);
                if (LVERR_OK != ret) {
                    failed = true;
                    break;
                }
                // Update hash
                sharet = SHA256Input(&sha256, buf, bytesRead);
                if (shaSuccess != sharet) {
                    failed = true;
                    break;
                }
                pos += bytesRead;
                if (bytesRead < CRC_BUF_SIZE)
                    break;
            }
            SetPos(savepos);
            if (!failed) {
                // Format resulting hash to string
                sharet = SHA256Result(&sha256, digest);
                if (shaSuccess == sharet) {
                    char str[SHA256HashSize * 2 + 1];
                    const uint8_t* pdigest = digest;
                    char* pstr = str;
                    for (int i = 0; i < SHA256HashSize; i++) {
                        snprintf(pstr, 3, "%02x", *pdigest);
                        pdigest++;
                        pstr += 2;
                    }
                    *pstr = 0;
                    dst = str;
                    res = LVERR_OK;
                }
            }
        }
        if (LVERR_OK == res)
            CRLog::debug("LVStream: successful calculation of SHA256");
        else
            CRLog::debug("LVStream: failed to calculate SHA256");
        return res;
    } else {
        // not supported
        return LVERR_NOTIMPL;
    }
#else
    return LVERR_NOTIMPL;
#endif
}
