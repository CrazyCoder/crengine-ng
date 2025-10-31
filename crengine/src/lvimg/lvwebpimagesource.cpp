/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2025 Aleksey Chernov <valexlin@gmail.com>               *
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

#include "lvwebpimagesource.h"

#if (USE_LIBWEBP == 1)

#include "lvimagedecodercallback.h"
#include "crlog.h"

#include <webp/decode.h>
#include <webp/demux.h>

LVWebPImageSource::LVWebPImageSource(ldomNode* node, LVStreamRef stream)
        : LVNodeImageSource(node, stream) {
}

LVWebPImageSource::~LVWebPImageSource() { }

void LVWebPImageSource::Compact() { }

bool LVWebPImageSource::Decode(LVImageDecoderCallback* callback) {
    bool res = true;
    lUInt8* data = nullptr;
    lvsize_t data_sz = 0;
    // Init decoder
    WebPDecoderConfig config;
    if (!WebPInitDecoderConfig(&config)) {
        CRLog::error("LVWebPImageSource: decoder init failed!");
        res = false;
    }
    // Read entire image data
    if (res) {
        const lvsize_t sz = _stream->GetSize();
        data = static_cast<lUInt8*>(malloc(sz));
        if (!data) {
            CRLog::error("LVWebPImageSource: failed to allocate memory!");
            res = false;
        }
        lvsize_t bytesRead = 0;
        _stream->SetPos(0);
        if (_stream->Read(data, sz, &bytesRead) != LVERR_OK || bytesRead != sz) {
            CRLog::error("LVWebPImageSource: failed to read image data!");
            res = false;
        } else {
            data_sz = bytesRead;
        }
    }
    if (res) {
        // Get image info
        res = WebPGetFeatures(data, data_sz, &config.input) == VP8_STATUS_OK;
        if (res) {
            _width = config.input.width;
            _height = config.input.height;
        } else {
            CRLog::error("LVWebPImageSource: retrieve the bitstream's features failed!");
        }
    }
    if (res) {
        // Decode webp image data
        if (callback) {
            callback->OnStartDecode(this);
            // Set BGRA pixel order
            config.output.colorspace = MODE_BGRA;
            // Decode
            if (config.input.has_animation) {
                // Fetch only first frame
                WebPData webp_data;
                webp_data.bytes = data;
                webp_data.size = (size_t)data_sz;
                WebPDemuxer* demux = WebPDemux(&webp_data);
                if (demux) {
                    WebPIterator iter;
                    if (WebPDemuxGetFrame(demux, 1, &iter)) {
                        res = WebPDecode(iter.fragment.bytes, iter.fragment.size, &config) == VP8_STATUS_OK;
                        WebPDemuxReleaseIterator(&iter);
                    } else {
                        res = false;
                        CRLog::error("LVWebPImageSource: get first frame failed!");
                    }
                    WebPDemuxDelete(demux);
                } else {
                    res = false;
                    CRLog::error("LVWebPImageSource: demuxer creation failed!");
                }
            } else {
                res = WebPDecode(data, data_sz, &config) == VP8_STATUS_OK;
            }
            if (res) {
                // enum each image line
                int stride = config.output.u.RGBA.stride;
                lUInt8* pdata = config.output.u.RGBA.rgba;
                lUInt8* endp = pdata + config.output.u.RGBA.size;
                int y = 0;
                while (pdata < endp) {
                    // Invert alpha for each pixel
                    lUInt8* palpha = pdata;
                    lUInt8* row_endp = pdata + stride;
                    while (palpha < row_endp) {
                        *(lUInt32*)palpha ^= 0xFF000000;
                        palpha += 4;
                    }
                    // call callback for each line
                    callback->OnLineDecoded(this, y, reinterpret_cast<lUInt32*>(pdata));
                    y++;
                    pdata += stride;
                }
            } else {
                CRLog::error("LVWebPImageSource: image decode failed!");
            }
            WebPFreeDecBuffer(&config.output);
            callback->OnEndDecode(this, !res);
        }
    }
    if (data)
        free(data);
    return res;
}

bool LVWebPImageSource::CheckPattern(const lUInt8* buf, int len) {
    return WebPGetInfo(buf, len, nullptr, nullptr) != 0;
}

#endif // (USE_LIBWEBP == 1)
