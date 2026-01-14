/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2026 Aleksey Chernov <valexlin@gmail.com>               *
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

#include "lvjpegxlimagesource.h"

#if (USE_LIBJXL == 1)

#include "lvimagedecodercallback.h"
#include "crlog.h"

#include <jxl/decode.h>
#include <jxl/resizable_parallel_runner.h>

#include <string.h>

// If reduce chunk size, the decoding speed drops significantly.
#define JXL_CHUNK_SIZE 524288

LVJpegXLImageSource::LVJpegXLImageSource(ldomNode* node, LVStreamRef stream)
        : LVNodeImageSource(node, stream) {
}

LVJpegXLImageSource::~LVJpegXLImageSource() {
}

void LVJpegXLImageSource::Compact() {
}

bool LVJpegXLImageSource::Decode(LVImageDecoderCallback* callback) {
    bool res = true;
    // Init decoder
    // Multi-threaded parallel runner.
    void* runner = JxlResizableParallelRunnerCreate(nullptr);
    JxlDecoder* decoder = JxlDecoderCreate(nullptr);
    JxlDecoderStatus status = JxlDecoderSubscribeEvents(decoder, JXL_DEC_BASIC_INFO |
                                                                         JXL_DEC_COLOR_ENCODING |
                                                                         JXL_DEC_FULL_IMAGE);
    if (JXL_DEC_SUCCESS != status) {
        CRLog::error("LVJpegXLImageSource: JxlDecoderSubscribeEvents failed!");
        res = false;
    }
    status = JxlDecoderSetParallelRunner(decoder, JxlResizableParallelRunner, runner);
    if (JXL_DEC_SUCCESS != status) {
        CRLog::error("LVJpegXLImageSource: JxlDecoderSetParallelRunner failed!");
        res = false;
    }
    lUInt8* chunk = nullptr;
    const lvsize_t total_sz = _stream->GetSize();
    lvsize_t chunk_size = (total_sz < JXL_CHUNK_SIZE) ? total_sz : JXL_CHUNK_SIZE;
    if (res) {
        chunk = (lUInt8*)malloc(chunk_size);
        if (!chunk) {
            CRLog::error("LVJpegXLImageSource: failed to allocate memory!");
            res = false;
        }
    }
    lvsize_t totalBytesRead = 0;
    lUInt8* read_ptr;
    lvsize_t req_bytes;
    lvsize_t bytesRead = 0;
    size_t chunk_data_sz = 0;
    if (res) {
        // Read initial data portion
        _stream->SetPos(0);
        read_ptr = chunk;
        req_bytes = chunk_size;
        bytesRead = 0;
        lverror_t ret = _stream->Read(read_ptr, req_bytes, &bytesRead);
        if (LVERR_OK != ret && LVERR_EOF != ret) {
            CRLog::error("LVJpegXLImageSource: stream read failed; ret=%d; rb=%u!", ret, bytesRead);
            res = false;
        } else {
            totalBytesRead += bytesRead;
            chunk_data_sz = bytesRead;
            JxlDecoderSetInput(decoder, chunk, chunk_data_sz);
        }
    }
    // Ultimately, in the image decoding loop, we only need
    //  to get one ready line/row of the image at a time
    //  to pass it to callback->OnLineDecoded().
    // However, we must first obtain a fully decompressed image,
    //  since the jxl library, when processing images progressively,
    //  produces the resulting image block-by-block rather than line-by-line,
    //  and the block width may be smaller than the image width.
    if (res) {
        JxlBasicInfo info;
        JxlPixelFormat format = { 4, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0 };
        lUInt32* pixelBuff = nullptr;
        lUInt32 pixelBuffSz = 0;

        bool loop_exit = false;
        while (true) {
            status = JxlDecoderProcessInput(decoder);
            switch (status) {
                case JXL_DEC_ERROR:
                    CRLog::error("LVJpegXLImageSource: decoder error");
                    res = false;
                    loop_exit = true;
                    break;
                case JXL_DEC_NEED_MORE_INPUT: {
                    if (totalBytesRead >= total_sz) {
                        CRLog::error("LVJpegXLImageSource: A request was received to retrieve the next piece of data after all data had already been read!");
                        res = false;
                        loop_exit = true;
                    } else {
                        // Read next data portion from stream & pass it to jxl
                        size_t jxl_unprocessed = JxlDecoderReleaseInput(decoder);
                        if (jxl_unprocessed > 0) {
                            // Move unprocessed data from tail of the buffer to it's head
                            memcpy(chunk, chunk + chunk_size - jxl_unprocessed, jxl_unprocessed);
                        }
                        read_ptr = chunk + jxl_unprocessed;
                        req_bytes = chunk_size - jxl_unprocessed;
                        JxlDecoderFlushImage(decoder);
                        lverror_t ret = _stream->Read(read_ptr, req_bytes, &bytesRead);
                        if (LVERR_OK == ret || LVERR_EOF == ret) {
                            if (bytesRead > 0) {
                                chunk_data_sz = bytesRead;
                                totalBytesRead += bytesRead;
                                JxlDecoderSetInput(decoder, chunk, chunk_data_sz);
                            }
                        } else {
                            CRLog::error("LVJpegXLImageSource: stream read failed; ret=%d; rb=%u!", ret, bytesRead);
                            res = false;
                            loop_exit = true;
                        }
                    }
                    break;
                }
                case JXL_DEC_BASIC_INFO:
                    if (JXL_DEC_SUCCESS == JxlDecoderGetBasicInfo(decoder, &info)) {
                        _width = (int)info.xsize;
                        _height = (int)info.ysize;
                        JxlResizableParallelRunnerSetThreads(
                                runner,
                                JxlResizableParallelRunnerSuggestThreads(info.xsize, info.ysize));
                    } else {
                        CRLog::error("LVJpegXLImageSource: JxlDecoderGetBasicInfo() failed");
                        loop_exit = true;
                        res = false;
                    }
                    if (!callback) {
                        // Early exit if no callback is provided.
                        // In this case, we only need the image size.
                        // There is no need to decode the image.
                        loop_exit = true;
                    }
                    break;
                case JXL_DEC_COLOR_ENCODING:
                    // For now just skip this...
                    break;
                case JXL_DEC_NEED_IMAGE_OUT_BUFFER: {
                    if (callback) {
                        size_t buffer_size;
                        JxlDecoderStatus t_status = JxlDecoderImageOutBufferSize(decoder, &format, &buffer_size);
                        if (JXL_DEC_SUCCESS == t_status) {
                            if (pixelBuff)
                                free(pixelBuff);
                            pixelBuff = (lUInt32*)malloc(buffer_size);
                            if (pixelBuff) {
                                pixelBuffSz = buffer_size;
                                t_status = JxlDecoderSetImageOutBuffer(decoder, &format, pixelBuff, pixelBuffSz);
                                if (JXL_DEC_SUCCESS == t_status) {
                                    callback->OnStartDecode(this);
                                } else {
                                    CRLog::error("LVJpegXLImageSource: JxlDecoderSetImageOutBuffer() failed!");
                                    res = false;
                                    loop_exit = true;
                                }
                            } else {
                                CRLog::error("LVJpegXLImageSource: failed to allocate memory!");
                                res = false;
                                loop_exit = true;
                            }
                        } else {
                            CRLog::error("LVJpegXLImageSource: JxlDecoderImageOutBufferSize() failed!");
                            res = false;
                            loop_exit = true;
                        }
                    } else {
                        CRLog::error("LVJpegXLImageSource: no callback is provided for image decoding!");
                        res = false;
                        loop_exit = true;
                    }
                    break;
                }
                case JXL_DEC_FULL_IMAGE:
                    // If the image is an animation we use only first frame,
                    // then break the loop
                    res = true;
                    loop_exit = true;
                    break;
                case JXL_DEC_SUCCESS:
                    // All decoding successfully finished.
                    // It's not required to call JxlDecoderReleaseInput(dec.get()) here since
                    // the decoder will be destroyed.
                    res = true;
                    loop_exit = true;
                    break;
                default:
                    CRLog::error("LVJpegXLImageSource: Unknown decoder status");
                    res = false;
                    loop_exit = true;
                    break;
            }
            if (loop_exit)
                break;
        }
        if (res) {
            if (callback) {
                // convert to crengine pixel format (from RGBA to BGRX)
                lUInt32* pdata = pixelBuff;
                int stride = _width; // no extra alignment
                lUInt32* endp = (lUInt32*)((lUInt8*)pdata + pixelBuffSz);
                int y = 0;
                while (pdata < endp) {
                    lUInt32* p = pdata;
                    lUInt32* row_endp = pdata + _width;
                    while (p < row_endp) {
                        // invert alpha, swap R & B
                        *p = (((*p) & 0xFF000000) ^ 0xFF000000) | (((*p) << 16) & 0x00FF0000) |
                             ((*p) & 0x0000FF00) | (((*p) >> 16) & 0x000000FF);
                        ++p;
                    }
                    // call callback for each line
                    callback->OnLineDecoded(this, y, pdata);
                    y++;
                    pdata += stride;
                }
            }
        }
        if (pixelBuff)
            free(pixelBuff);
        if (callback)
            callback->OnEndDecode(this, !res);
    }
    JxlDecoderDestroy(decoder);
    JxlResizableParallelRunnerDestroy(runner);
    if (chunk)
        free(chunk);
    return res;
}

bool LVJpegXLImageSource::CheckPattern(const lUInt8* buf, int len) {
    JxlSignature sig = JxlSignatureCheck(buf, (size_t)len);
    return JXL_SIG_CONTAINER == sig || JXL_SIG_CODESTREAM == sig;
}

#endif // (USE_LIBJXL == 1)
