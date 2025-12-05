/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2025 Xteink                                             *
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

#include <xtcexport.h>
#include <lvstreamutils.h>
#include <lvdocview.h>
#include <lvtocitem.h>
#include <crlog.h>
#include <cstring>
#include <cstdio>
#include <ctime>

// =============================================================================
// Helper: Write little-endian values
// =============================================================================

static inline void writeLE16(uint8_t* buf, uint16_t val) {
    buf[0] = (uint8_t)(val & 0xFF);
    buf[1] = (uint8_t)((val >> 8) & 0xFF);
}

static inline void writeLE32(uint8_t* buf, uint32_t val) {
    buf[0] = (uint8_t)(val & 0xFF);
    buf[1] = (uint8_t)((val >> 8) & 0xFF);
    buf[2] = (uint8_t)((val >> 16) & 0xFF);
    buf[3] = (uint8_t)((val >> 24) & 0xFF);
}

static inline void writeLE64(uint8_t* buf, uint64_t val) {
    for (int i = 0; i < 8; i++) {
        buf[i] = (uint8_t)((val >> (i * 8)) & 0xFF);
    }
}

// =============================================================================
// XtgWriter Implementation
// =============================================================================

uint32_t XtgWriter::getTotalSize(uint16_t width, uint16_t height) {
    return sizeof(xtg_header_t) + xtg_data_size(width, height);
}

bool XtgWriter::write(LVStream* stream, LVGrayDrawBuf& buf, GrayToMonoPolicy grayPolicy) {
    if (!stream)
        return false;

    uint16_t width = (uint16_t)buf.GetWidth();
    uint16_t height = (uint16_t)buf.GetHeight();
    int srcBpp = buf.GetBitsPerPixel();
    uint32_t dataSize = xtg_data_size(width, height);

    // Write XTG header
    xtg_header_t header;
    memset(&header, 0, sizeof(header));
    header.magic = XTG_MAGIC;
    header.width = width;
    header.height = height;
    header.colorMode = 0;
    header.compression = 0;
    header.dataSize = dataSize;
    header.md5 = 0;  // Optional, not computed

    if (stream->Write(&header, sizeof(header), NULL) != LVERR_OK)
        return false;

    // Calculate row size in bytes (1 bit per pixel, packed)
    int rowBytes = (width + 7) / 8;
    uint8_t* rowBuffer = new uint8_t[rowBytes];

    // Write image data row by row (top to bottom)
    for (int y = 0; y < height; y++) {
        memset(rowBuffer, 0, rowBytes);
        const uint8_t* srcRow = buf.GetScanLine(y);

        // Convert source pixels to 1-bit
        for (int x = 0; x < width; x++) {
            int mono = 0;

            if (srcBpp == 1) {
                // Source is already 1-bit packed - no policy needed
                int shift = 7 - (x & 7);
                mono = (srcRow[x >> 3] >> shift) & 1;
            } else if (srcBpp == 2) {
                // 2-bit: 4 pixels per byte, MSB first
                // LVGrayDrawBuf values: 0=black, 1=dark gray, 2=light gray, 3=white
                int shift = 6 - ((x & 3) << 1);
                uint8_t v2 = (srcRow[x >> 2] >> shift) & 0x03;
                // Apply gray conversion policy
                switch (grayPolicy) {
                    case GRAY_SPLIT_LIGHT_DARK:
                        // light gray + white -> 1 (white), dark gray + black -> 0 (black)
                        mono = (v2 >= 2) ? 1 : 0;
                        break;
                    case GRAY_ALL_TO_WHITE:
                        // everything except pure black becomes white
                        mono = (v2 != 0) ? 1 : 0;
                        break;
                    case GRAY_ALL_TO_BLACK:
                        // only pure white becomes white
                        mono = (v2 == 3) ? 1 : 0;
                        break;
                }
            } else if (srcBpp == 4) {
                // 4-bit: high nibble per byte (0-15 range)
                uint8_t v4 = (srcRow[x] >> 4) & 0x0F;
                // Apply policy-equivalent thresholds for 4-bit
                switch (grayPolicy) {
                    case GRAY_SPLIT_LIGHT_DARK:
                        // Threshold at mid-gray (7-8)
                        mono = (v4 >= 8) ? 1 : 0;
                        break;
                    case GRAY_ALL_TO_WHITE:
                        // Only pure black (0) stays black
                        mono = (v4 != 0) ? 1 : 0;
                        break;
                    case GRAY_ALL_TO_BLACK:
                        // Only pure white (15) stays white
                        mono = (v4 == 15) ? 1 : 0;
                        break;
                }
            } else if (srcBpp == 8) {
                // 8-bit: full byte (0-255 range)
                uint8_t v8 = srcRow[x];
                // Apply policy-equivalent thresholds for 8-bit
                switch (grayPolicy) {
                    case GRAY_SPLIT_LIGHT_DARK:
                        // Threshold at mid-gray (128)
                        mono = (v8 >= 128) ? 1 : 0;
                        break;
                    case GRAY_ALL_TO_WHITE:
                        // Only pure black (0) stays black
                        mono = (v8 != 0) ? 1 : 0;
                        break;
                    case GRAY_ALL_TO_BLACK:
                        // Only pure white (255) stays white
                        mono = (v8 == 255) ? 1 : 0;
                        break;
                }
            }

            // Pack into destination: MSB = leftmost pixel
            if (mono) {
                rowBuffer[x >> 3] |= (uint8_t)(0x80 >> (x & 7));
            }
        }

        if (stream->Write(rowBuffer, rowBytes, NULL) != LVERR_OK) {
            delete[] rowBuffer;
            return false;
        }
    }

    delete[] rowBuffer;
    return true;
}

// =============================================================================
// XthWriter Implementation
// =============================================================================

uint32_t XthWriter::getTotalSize(uint16_t width, uint16_t height) {
    return sizeof(xtg_header_t) + xth_data_size(width, height);
}

bool XthWriter::write(LVStream* stream, LVGrayDrawBuf& buf) {
    if (!stream)
        return false;

    uint16_t width = (uint16_t)buf.GetWidth();
    uint16_t height = (uint16_t)buf.GetHeight();
    int srcBpp = buf.GetBitsPerPixel();
    uint32_t dataSize = xth_data_size(width, height);
    uint32_t pixelCount = (uint32_t)width * height;
    uint32_t planeSize = (pixelCount + 7) / 8;

    // Write XTH header
    xtg_header_t header;
    memset(&header, 0, sizeof(header));
    header.magic = XTH_MAGIC;
    header.width = width;
    header.height = height;
    header.colorMode = 0;
    header.compression = 0;
    header.dataSize = dataSize;
    header.md5 = 0;

    if (stream->Write(&header, sizeof(header), NULL) != LVERR_OK)
        return false;

    // Allocate bit planes
    uint8_t* plane1 = new uint8_t[planeSize];
    uint8_t* plane2 = new uint8_t[planeSize];
    memset(plane1, 0, planeSize);
    memset(plane2, 0, planeSize);

    // XTH format uses vertical scan order (column-major):
    // - Columns are scanned from RIGHT to LEFT (x = width-1 down to 0)
    // - Within each column, 8 vertical pixels are packed per byte
    // - This matches the Xteink device's display refresh pattern
    //
    // Reference: XthSample.html JavaScript implementation

    // XTH grayscale mapping lookup table
    // The Xteink e-paper LUT has swapped middle values:
    //   XTH 0 = White, XTH 1 = Dark gray, XTH 2 = Light gray, XTH 3 = Black
    // This matches the JavaScript reference implementation in XthSample.html
    // LVGrayDrawBuf: 0=black, 1=dark, 2=light, 3=white
    // Mapping: 0->3, 1->1, 2->2, 3->0
    static const uint8_t grayToXth[4] = {3, 1, 2, 0};

    // Helper lambda to get 2-bit XTH pixel value at (x, y)
    auto getPixelValue = [&](int x, int y) -> uint8_t {
        const uint8_t* srcRow = buf.GetScanLine(y);
        uint8_t v2 = 0;

        if (srcBpp == 1) {
            // 1-bit: expand to 2-bit (0 -> 3=black, 1 -> 0=white)
            int shift = 7 - (x & 7);
            int mono = (srcRow[x >> 3] >> shift) & 1;
            v2 = mono ? 0 : 3;
        } else if (srcBpp == 2) {
            // 2-bit: 4 pixels per byte, MSB first
            int shift = 6 - ((x & 3) << 1);
            uint8_t srcVal = (srcRow[x >> 2] >> shift) & 0x03;
            v2 = grayToXth[srcVal];
        } else if (srcBpp == 4) {
            // 4-bit: quantize to 4 levels then apply LUT
            uint8_t v4 = (srcRow[x] >> 4) & 0x0F;
            uint8_t gray4;
            if (v4 < 4) gray4 = 0;       // black
            else if (v4 < 8) gray4 = 1;  // dark gray
            else if (v4 < 12) gray4 = 2; // light gray
            else gray4 = 3;              // white
            v2 = grayToXth[gray4];
        } else if (srcBpp == 8) {
            // 8-bit: quantize to 4 levels then apply LUT
            uint8_t v8 = srcRow[x];
            uint8_t gray4;
            if (v8 < 64)                gray4 = 0; // black
            else if (v8 < 128)
                gray4 = 1; // dark gray
            else if (v8 < 192)
                gray4 = 2; // light gray
            else
                gray4 = 3; // white
            v2 = grayToXth[gray4];
        }
        return v2;
    };

    // Vertical scan: columns from right to left
    uint32_t byteIdx = 0;
    for (int x = width - 1; x >= 0; x--) {
        // Process 8 vertical pixels at a time
        for (int y = 0; y < height; y += 8) {
            uint8_t byte1 = 0;
            uint8_t byte2 = 0;

            for (int i = 0; i < 8; i++) {
                if (y + i < height) {
                    uint8_t v2 = getPixelValue(x, y + i);

                    // Split into bit planes: pixelValue = (bit1 << 1) | bit2
                    uint8_t bit1 = (v2 >> 1) & 1;
                    uint8_t bit2 = v2 & 1;

                    // Pack bits: MSB first (bit 7 = first pixel in group)
                    byte1 |= (uint8_t)(bit1 << (7 - i));
                    byte2 |= (uint8_t)(bit2 << (7 - i));
                }
            }

            plane1[byteIdx] = byte1;
            plane2[byteIdx] = byte2;
            byteIdx++;
        }
    }

    // Write bit planes
    bool ok = true;
    if (stream->Write(plane1, planeSize, NULL) != LVERR_OK)
        ok = false;
    if (ok && stream->Write(plane2, planeSize, NULL) != LVERR_OK)
        ok = false;

    delete[] plane1;
    delete[] plane2;
    return ok;
}

// =============================================================================
// XtcExporter Implementation
// =============================================================================

XtcExporter::XtcExporter()
    : m_format(XTC_FORMAT_XTC)
    , m_width(480)
    , m_height(800)
    , m_readDirection(XTC_DIR_LTR)
    , m_enableChapters(false)
    , m_chapterDepth(2)
    , m_enableThumbnails(false)
    , m_thumbWidth(120)
    , m_thumbHeight(160)
    , m_grayPolicy(GRAY_SPLIT_LIGHT_DARK)
    , m_startPage(-1)
    , m_endPage(-1)
    , m_dumpImagesLimit(0)
    , m_callback(nullptr) {
}

XtcExporter::~XtcExporter() {
}

XtcExporter& XtcExporter::setFormat(XtcExportFormat format) {
    m_format = format;
    return *this;
}

XtcExporter& XtcExporter::setDimensions(uint16_t width, uint16_t height) {
    m_width = width;
    m_height = height;
    return *this;
}

XtcExporter& XtcExporter::setMetadata(const lString8& title, const lString8& author) {
    m_title = title;
    m_author = author;
    return *this;
}

XtcExporter& XtcExporter::setMetadata(const lString8& title, const lString8& author,
                                      const lString8& publisher, const lString8& language) {
    m_title = title;
    m_author = author;
    m_publisher = publisher;
    m_language = language;
    return *this;
}

XtcExporter& XtcExporter::setReadingDirection(uint8_t direction) {
    m_readDirection = direction;
    return *this;
}

XtcExporter& XtcExporter::enableChapters(bool enable) {
    m_enableChapters = enable;
    return *this;
}

XtcExporter& XtcExporter::setChapterDepth(int maxDepth) {
    m_chapterDepth = maxDepth;
    return *this;
}

XtcExporter& XtcExporter::enableThumbnails(bool enable, uint16_t thumbWidth, uint16_t thumbHeight) {
    m_enableThumbnails = enable;
    m_thumbWidth = thumbWidth;
    m_thumbHeight = thumbHeight;
    return *this;
}

XtcExporter& XtcExporter::setGrayPolicy(GrayToMonoPolicy policy) {
    m_grayPolicy = policy;
    return *this;
}

XtcExporter& XtcExporter::setPageRange(int startPage, int endPage) {
    m_startPage = startPage;
    m_endPage = endPage;
    return *this;
}

XtcExporter& XtcExporter::setProgressCallback(XtcExportCallback* callback) {
    m_callback = callback;
    return *this;
}

XtcExporter& XtcExporter::dumpImages(int limit) {
    m_dumpImagesLimit = limit;
    return *this;
}

// =============================================================================
// BMP Debug Dump Helper
// =============================================================================

/// Helper function to write a 32-bit little-endian value to a buffer at offset
static inline void writeBmpLE32(unsigned char* buf, int offset, uint32_t value) {
    buf[offset + 0] = value & 0xFF;
    buf[offset + 1] = (value >> 8) & 0xFF;
    buf[offset + 2] = (value >> 16) & 0xFF;
    buf[offset + 3] = (value >> 24) & 0xFF;
}

/// Helper function to write a 16-bit little-endian value to a buffer at offset
static inline void writeBmpLE16(unsigned char* buf, int offset, uint16_t value) {
    buf[offset + 0] = value & 0xFF;
    buf[offset + 1] = (value >> 8) & 0xFF;
}

/// Save LVGrayDrawBuf as BMP file for debugging
/// @param filename Output filename (UTF-8)
/// @param buf Source grayscale buffer
/// @param desiredBpp 0 = use buf bpp; otherwise force output bpp (1 or buf bpp/4bpp)
/// @param grayPolicy Used only when converting 2bpp -> 1bpp
static bool saveBmpFile(const char* filename, LVGrayDrawBuf& buf, int desiredBpp = 0,
                        GrayToMonoPolicy grayPolicy = GRAY_SPLIT_LIGHT_DARK) {
    int width = buf.GetWidth();
    int height = buf.GetHeight();
    int bpp = buf.GetBitsPerPixel();

    // For 2-bit images, use 4-bit indexed BMP for broad compatibility (BMP doesn't support true 2-bpp)
    int outputBpp;
    if (desiredBpp == 1) {
        outputBpp = 1;
    } else {
        outputBpp = (bpp == 2) ? 4 : bpp;
    }

    // Calculate row size (must be multiple of 4 bytes)
    int rowSize = ((width * outputBpp + 31) / 32) * 4;
    int imageSize = rowSize * height;

    // Calculate palette size for grayscale images
    int paletteSize = 0;
    if (outputBpp <= 8) {
        paletteSize = (1 << outputBpp) * 4; // 4 bytes per color (BGRA)
    }

    int fileSize = 54 + paletteSize + imageSize;
    int pixelDataOffset = 54 + paletteSize;

    // BMP file header (14 bytes)
    unsigned char fileHeader[14] = {
        'B', 'M',           // Signature
        0, 0, 0, 0,         // File size (filled below)
        0, 0, 0, 0,         // Reserved
        0, 0, 0, 0          // Offset to pixel data (filled below)
    };

    // BMP info header (40 bytes)
    unsigned char infoHeader[40] = {
        40, 0, 0, 0,        // Info header size
        0, 0, 0, 0,         // Width (filled below)
        0, 0, 0, 0,         // Height (filled below)
        1, 0,               // Planes
        0, 0,               // Bits per pixel (filled below)
        0, 0, 0, 0,         // Compression (0 = none)
        0, 0, 0, 0,         // Image size (0 for uncompressed)
        0, 0, 0, 0,         // X pixels per meter
        0, 0, 0, 0,         // Y pixels per meter
        0, 0, 0, 0,         // Colors used
        0, 0, 0, 0          // Important colors
    };

    // Fill in file header fields
    writeBmpLE32(fileHeader, 2, fileSize);
    writeBmpLE32(fileHeader, 10, pixelDataOffset);

    // Fill in info header fields
    writeBmpLE32(infoHeader, 4, width);
    writeBmpLE32(infoHeader, 8, height);
    writeBmpLE16(infoHeader, 14, outputBpp);

    // Open file for writing
    LVStreamRef stream = LVOpenFileStream(filename, LVOM_WRITE);
    if (!stream)
        return false;

    // Write headers
    if (stream->Write(fileHeader, 14, NULL) != LVERR_OK)
        return false;
    if (stream->Write(infoHeader, 40, NULL) != LVERR_OK)
        return false;

    // Write palette for grayscale images
    if (paletteSize > 0) {
        int numColors = 1 << outputBpp;
        unsigned char palette[1024] = {0}; // Max 256 colors * 4 bytes
        for (int i = 0; i < numColors; i++) {
            int gray = (i * 255) / (numColors - 1);
            palette[i * 4 + 0] = gray; // Blue
            palette[i * 4 + 1] = gray; // Green
            palette[i * 4 + 2] = gray; // Red
            palette[i * 4 + 3] = 0;    // Reserved
        }
        if (stream->Write(palette, paletteSize, NULL) != LVERR_OK)
            return false;
    }

    // Write pixel data (BMP stores rows bottom-to-top)
    unsigned char* rowBuffer = new unsigned char[rowSize];
    for (int y = height - 1; y >= 0; y--) {
        // Clear destination row to avoid carrying over nibbles from previous rows
        memset(rowBuffer, 0, rowSize);
        const unsigned char* srcRow = buf.GetScanLine(y);

        if (bpp == 2 && outputBpp == 1) {
            // Convert 2-bit packed row to 1-bit packed row, with policy handling
            // 2-bit pixels packed MSB-first: x-> shift2bit = 6 - ((x & 3) * 2)
            // 1-bit pixels packed MSB-first: set bit (7 - (x & 7)) in destination byte
            for (int x = 0; x < width; x++) {
                int shift2bit = 6 - ((x & 3) << 1);
                uint8_t v2 = (srcRow[x >> 2] >> shift2bit) & 0x03; // 0..3
                int mono = 0;
                switch (grayPolicy) {
                    case GRAY_SPLIT_LIGHT_DARK:
                        // light gray + white -> 1 (white), dark gray + black -> 0 (black)
                        mono = (v2 >= 2) ? 1 : 0;
                        break;
                    case GRAY_ALL_TO_WHITE:
                        // everything except pure black becomes white
                        mono = (v2 != 0) ? 1 : 0;
                        break;
                    case GRAY_ALL_TO_BLACK:
                        // only pure white becomes white
                        mono = (v2 == 3) ? 1 : 0;
                        break;
                }
                if (mono) {
                    rowBuffer[x >> 3] |= (uint8_t)(0x80 >> (x & 7));
                }
            }
        } else if (bpp == 2 && outputBpp == 4) {
            // Convert 2-bit to 4-bit: expand each 2-bit pixel to 4-bit
            // 2-bit pixels are packed 4 per byte: pixel 0 at bits 6-7, pixel 1 at bits 4-5, etc.
            // 4-bit pixels are packed 2 per byte: pixel 0 at bits 4-7, pixel 1 at bits 0-3
            for (int x = 0; x < width; x++) {
                int shift2bit = 6 - ((x & 3) << 1);  // shift: 6, 4, 2, 0 for pixels 0, 1, 2, 3
                uint8_t pixel2bit = (srcRow[x >> 2] >> shift2bit) & 0x03;

                // Expand 2-bit (0-3) to 4-bit (0-15) by duplicating bits
                // 0b00 -> 0b0000, 0b01 -> 0b0101, 0b10 -> 0b1010, 0b11 -> 0b1111
                uint8_t pixel4bit = pixel2bit | (pixel2bit << 2);

                // Pack into 4-bit format
                int shift4bit = (x & 1) ? 0 : 4;
                unsigned char& dstByte = rowBuffer[x >> 1];
                if (shift4bit == 4) {
                    // Set high nibble, preserve low nibble written by previous pixel in this row
                    dstByte = (unsigned char)((dstByte & 0x0F) | (pixel4bit << 4));
                } else {
                    // Set low nibble, preserve high nibble
                    dstByte = (unsigned char)((dstByte & 0xF0) | pixel4bit);
                }
            }
        } else if (bpp == 4 && outputBpp == 4) {
            // Source buffer stores one byte per pixel with the meaningful value in the high nibble (0xF0)
            // BMP 4bpp expects two pixels per byte: first pixel in high nibble, second in low nibble.
            // So, repack two source pixels (high nibbles) into one destination byte.
            int dstIndex = 0;
            int x = 0;
            for (; x + 1 < width; x += 2) {
                // Extract high nibbles and combine
                uint8_t hi = (uint8_t)(srcRow[x] & 0xF0);          // already high nibble
                uint8_t lo = (uint8_t)((srcRow[x + 1] & 0xF0) >> 4); // move to low nibble
                rowBuffer[dstIndex++] = (uint8_t)(hi | lo);
            }
            if (x < width) {
                // Odd width: last pixel goes to high nibble, low nibble stays 0
                rowBuffer[dstIndex] = (uint8_t)(srcRow[x] & 0xF0);
            }
        } else {
            // For other bit depths, copy directly
            int srcRowBytes = (bpp <= 2) ? (width * bpp + 7) / 8 : width;
            memcpy(rowBuffer, srcRow, srcRowBytes);
        }

        if (stream->Write(rowBuffer, rowSize, NULL) != LVERR_OK) {
            delete[] rowBuffer;
            return false;
        }
    }

    delete[] rowBuffer;
    return true;
}

bool XtcExporter::exportSinglePage(LVGrayDrawBuf& buf, const lChar32* filename) {
    LVStreamRef stream = LVOpenFileStream(filename, LVOM_WRITE);
    if (!stream)
        return false;

    if (m_format == XTC_FORMAT_XTC) {
        return XtgWriter::write(stream.get(), buf, m_grayPolicy);
    } else {
        return XthWriter::write(stream.get(), buf);
    }
}

bool XtcExporter::writeHeader(LVStream* stream, uint16_t pageCount, bool hasChapters) {
    xtc_header_t header;
    memset(&header, 0, sizeof(header));

    header.magic = (m_format == XTC_FORMAT_XTC) ? XTC_MAGIC : XTCH_MAGIC;
    header.version = XTC_VERSION;
    header.pageCount = pageCount;
    header.readDirection = m_readDirection;
    header.hasMetadata = (!m_title.empty() || !m_author.empty()) ? 1 : 0;
    header.hasThumbnails = 0; // Not implemented yet
    header.hasChapters = hasChapters ? 1 : 0;
    header.currentPage = 0;

    // Calculate offsets (will be updated later)
    uint64_t offset = sizeof(xtc_header_t);

    // Metadata
    if (header.hasMetadata) {
        header.metadataOffset = offset;
        offset += sizeof(xtc_metadata_t);
    } else {
        header.metadataOffset = 0;
    }

    // Chapters are written after metadata, but we don't know count yet
    // The actual chapter offset calculation is done in exportDocument

    // Page index
    header.indexOffset = offset;
    offset += pageCount * sizeof(xtc_page_index_t);

    // Data area starts after index
    header.dataOffset = offset;

    // Thumbnails (not implemented)
    header.thumbOffset = 0;

    return stream->Write(&header, sizeof(header), NULL) == LVERR_OK;
}

bool XtcExporter::writeMetadata(LVStream* stream, LVDocView* docView, uint16_t chapterCount) {
    xtc_metadata_t meta;
    memset(&meta, 0, sizeof(meta));

    // Copy strings with bounds checking
    if (!m_title.empty()) {
        strncpy(meta.title, m_title.c_str(), sizeof(meta.title) - 1);
    }
    if (!m_author.empty()) {
        strncpy(meta.author, m_author.c_str(), sizeof(meta.author) - 1);
    }
    if (!m_publisher.empty()) {
        strncpy(meta.publisher, m_publisher.c_str(), sizeof(meta.publisher) - 1);
    }
    if (!m_language.empty()) {
        strncpy(meta.language, m_language.c_str(), sizeof(meta.language) - 1);
    }

    // Set creation timestamp to current time
    meta.createTime = (uint32_t)std::time(nullptr);

    // Detect cover page: if document shows cover, it's page 0
    meta.coverPage = (docView && docView->getShowCover()) ? 0 : 0xFFFF;

    // Chapter count
    meta.chapterCount = chapterCount;

    return stream->Write(&meta, sizeof(meta), NULL) == LVERR_OK;
}

int XtcExporter::collectChapters(LVTocItem* item, LVArray<xtc_chapter_t>& chapters, int maxDepth, int currentDepth) {
    if (!item)
        return 0;

    int count = 0;

    // Process this item if it's not the root (level > 0)
    if (item->getLevel() > 0) {
        // Check depth limit (0 = unlimited)
        if (maxDepth > 0 && currentDepth > maxDepth)
            return 0;

        xtc_chapter_t chapter;
        memset(&chapter, 0, sizeof(chapter));

        // Get chapter name and convert to UTF-8
        lString8 name8 = UnicodeToUtf8(item->getName());
        strncpy(chapter.chapterName, name8.c_str(), sizeof(chapter.chapterName) - 1);

        // Get page number (already calculated by LVDocView)
        chapter.startPage = (uint16_t)item->getPage();
        chapter.endPage = chapter.startPage; // Simplified: same as startPage

        chapters.add(chapter);
        count++;
    }

    // Process children recursively
    int nextDepth = (item->getLevel() > 0) ? currentDepth + 1 : currentDepth;
    for (int i = 0; i < item->getChildCount(); i++) {
        count += collectChapters(item->getChild(i), chapters, maxDepth, nextDepth);
    }

    return count;
}

bool XtcExporter::writeChapters(LVStream* stream, const LVArray<xtc_chapter_t>& chapters) {
    for (int i = 0; i < chapters.length(); i++) {
        xtc_chapter_t chapter = chapters[i];
        if (stream->Write(&chapter, sizeof(xtc_chapter_t), NULL) != LVERR_OK)
            return false;
    }
    return true;
}

bool XtcExporter::writePageIndex(LVStream* stream, const LVArray<xtc_page_index_t>& pageIndex) {
    for (int i = 0; i < pageIndex.length(); i++) {
        xtc_page_index_t entry = pageIndex[i];
        if (stream->Write(&entry, sizeof(xtc_page_index_t), NULL) != LVERR_OK)
            return false;
    }
    return true;
}

bool XtcExporter::writePage(LVStream* stream, LVGrayDrawBuf& buf, xtc_page_index_t& indexEntry) {
    // Record current position
    indexEntry.offset = stream->GetPos();
    indexEntry.width = (uint16_t)buf.GetWidth();
    indexEntry.height = (uint16_t)buf.GetHeight();

    bool ok;
    if (m_format == XTC_FORMAT_XTC) {
        indexEntry.size = XtgWriter::getTotalSize(indexEntry.width, indexEntry.height);
        ok = XtgWriter::write(stream, buf, m_grayPolicy);
    } else {
        indexEntry.size = XthWriter::getTotalSize(indexEntry.width, indexEntry.height);
        ok = XthWriter::write(stream, buf);
    }

    return ok;
}

bool XtcExporter::exportDocument(LVDocView* docView, const lChar32* filename) {
    LVStreamRef stream = LVOpenFileStream(filename, LVOM_WRITE);
    if (!stream) {
        CRLog::error("XtcExporter: Failed to open file for writing");
        return false;
    }

    // Extract directory path from output filename for BMP dumps
    if (m_dumpImagesLimit != 0) {
        lString8 outPath = UnicodeToUtf8(lString32(filename));
        int pathEnd = -1;
        for (int i = outPath.length() - 1; i >= 0; i--) {
            char c = outPath[i];
            if (c == '/' || c == '\\') {
                pathEnd = i;
                break;
            }
        }
        if (pathEnd >= 0) {
            m_dumpDir = outPath.substr(0, pathEnd + 1);  // Include trailing separator
        } else {
            m_dumpDir = lString8("./");  // Use current directory
        }
    }

    return exportDocument(docView, stream);
}

bool XtcExporter::exportDocument(LVDocView* docView, LVStreamRef stream) {
    if (!docView || !stream) {
        return false;
    }
    // Save current view state
    int save_dx = docView->GetWidth();
    int save_dy = docView->GetHeight();

    CRLog::info("XtcExporter: Sawing original width/height [%d, %d]", save_dx, save_dy);

    int save_page = docView->getCurPage();
    lUInt32 old_flags = docView->getPageHeaderInfo();
    docView->setPageHeaderInfo(old_flags & ~(PGHDR_CLOCK | PGHDR_BATTERY));

    // Resize to target dimensions
    docView->Resize(m_width, m_height);

    // CRITICAL: Re-render document for new size before accessing pages.
    // Resize() marks document as needing re-render but doesn't render immediately.
    // If we access pages without re-rendering, and something triggers checkRender()
    // during drawing (e.g., GetFullHeight() called from drawPageHeader when status
    // bar is enabled), the page list will be cleared mid-iteration, causing crashes.
    docView->checkRender();

    // Get page list
    LVRendPageList* pages = docView->getPageList();
    if (!pages) {
        docView->Resize(save_dx, save_dy);
        return false;
    }

    int totalPageCount = pages->length();

    // Calculate actual page range (0-based)
    int actualStartPage = (m_startPage >= 0) ? m_startPage : 0;
    int actualEndPage = (m_endPage >= 0) ? m_endPage : totalPageCount - 1;

    // Clamp to valid range
    if (actualStartPage < 0)
        actualStartPage = 0;
    if (actualEndPage >= totalPageCount)
        actualEndPage = totalPageCount - 1;
    if (actualStartPage > actualEndPage) {
        CRLog::error("XtcExporter: Invalid page range [%d, %d]", actualStartPage, actualEndPage);
        docView->Resize(save_dx, save_dy);
        return false;
    }

    int exportPageCount = actualEndPage - actualStartPage + 1;
    bool hasMetadata = !m_title.empty() || !m_author.empty();

    CRLog::info("XtcExporter: Exporting pages %d-%d (total %d pages)",
                actualStartPage, actualEndPage, exportPageCount);

    // Collect chapters if enabled
    LVArray<xtc_chapter_t> chapters;
    bool hasChapters = false;
    if (m_enableChapters) {
        LVTocItem* toc = docView->getToc();
        if (toc && toc->getChildCount() > 0) {
            // Ensure page numbers are updated
            docView->updatePageNumbers(toc);
            collectChapters(toc, chapters, m_chapterDepth, 1);

            // Filter chapters to only include those within the exported page range
            // and adjust page numbers to be relative to the export start
            LVArray<xtc_chapter_t> filteredChapters;
            for (int i = 0; i < chapters.length(); i++) {
                int chapterPage = chapters[i].startPage;
                // Include chapter if its start page is within the exported range
                if (chapterPage >= actualStartPage && chapterPage <= actualEndPage) {
                    xtc_chapter_t adjustedChapter = chapters[i];
                    // Adjust page numbers to be relative to exported range (0-based in output)
                    adjustedChapter.startPage = (uint16_t)(chapterPage - actualStartPage);
                    adjustedChapter.endPage = (uint16_t)(chapterPage - actualStartPage);
                    filteredChapters.add(adjustedChapter);
                }
            }
            chapters = filteredChapters;

            hasChapters = chapters.length() > 0;
            if (hasChapters) {
                CRLog::info("XtcExporter: Collected %d chapters from TOC (filtered for page range)",
                            chapters.length());
            }
        }
    }

    // Calculate offsets
    uint64_t headerSize = sizeof(xtc_header_t);
    uint64_t metadataSize = hasMetadata ? sizeof(xtc_metadata_t) : 0;
    uint64_t chapterSize = hasChapters ? chapters.length() * sizeof(xtc_chapter_t) : 0;
    uint64_t indexSize = exportPageCount * sizeof(xtc_page_index_t);

    // Write placeholder header (will be updated at the end)
    xtc_header_t header;
    memset(&header, 0, sizeof(header));
    header.magic = (m_format == XTC_FORMAT_XTC) ? XTC_MAGIC : XTCH_MAGIC;
    header.version = XTC_VERSION;
    header.pageCount = (uint16_t)exportPageCount;
    header.readDirection = m_readDirection;
    header.hasMetadata = hasMetadata ? 1 : 0;
    header.hasThumbnails = 0;
    header.hasChapters = hasChapters ? 1 : 0;
    header.currentPage = 0;
    header.metadataOffset = hasMetadata ? headerSize : 0;
    // Chapters are stored after metadata (chapter offset is in metadata.chapterCount as count)
    // Page index follows metadata and chapters
    header.indexOffset = headerSize + metadataSize + chapterSize;
    header.dataOffset = headerSize + metadataSize + chapterSize + indexSize;
    header.thumbOffset = 0;

    if (stream->Write(&header, sizeof(header), NULL) != LVERR_OK) {
        docView->Resize(save_dx, save_dy);
        return false;
    }

    // Write metadata if present
    if (hasMetadata) {
        uint16_t chapterCount = hasChapters ? (uint16_t)chapters.length() : 0;
        if (!writeMetadata(stream.get(), docView, chapterCount)) {
            docView->Resize(save_dx, save_dy);
            return false;
        }
    }

    // Write chapters if present
    if (hasChapters) {
        if (!writeChapters(stream.get(), chapters)) {
            CRLog::error("XtcExporter: Failed to write chapters");
            docView->Resize(save_dx, save_dy);
            return false;
        }
    }

    // Write placeholder page index (will be updated after pages are written)
    LVArray<xtc_page_index_t> pageIndex(exportPageCount, xtc_page_index_t());
    uint64_t indexPos = stream->GetPos();
    for (int i = 0; i < exportPageCount; i++) {
        xtc_page_index_t entry;
        memset(&entry, 0, sizeof(entry));
        if (stream->Write(&entry, sizeof(entry), NULL) != LVERR_OK) {
            docView->Resize(save_dx, save_dy);
            return false;
        }
    }

    // Render and write each page
    int lastPercent = -1;
    int bpp = (m_format == XTC_FORMAT_XTC) ? 1 : 2;
    // For 1-bit output, render at 2-bit for antialiasing, then convert
    int renderBpp = (bpp == 1) ? 2 : bpp;
    int dumpedCount = 0;  // Track number of BMP images dumped

    for (int i = 0; i < exportPageCount; i++) {
        // Progress callback
        int percent = (i * 100) / exportPageCount;
        if (percent != lastPercent && m_callback) {
            m_callback->onProgress(percent);
            lastPercent = percent;
        }

        // Calculate source page index
        int srcPageIdx = actualStartPage + i;

        // Render page - account for rotation swapping internal dimensions
        // When rotation is 90/270, Resize() swaps m_dx/m_dy internally,
        // so we need to create the buffer at the swapped dimensions to match
#if CR_INTERNAL_PAGE_ORIENTATION == 1
        cr_rotate_angle_t angle = docView->GetRotateAngle();
        bool swapped = (angle == CR_ROTATE_ANGLE_90 || angle == CR_ROTATE_ANGLE_270);
        // Skip rotation for cover page (srcPageIdx==0) at 90/270 angles
        // to preserve portrait cover images at full screen size
        bool isCoverPage = (srcPageIdx == 0 && docView->getShowCover());
        bool skipRotation = isCoverPage && swapped;
        int bufWidth = (swapped && !skipRotation) ? m_height : m_width;
        int bufHeight = (swapped && !skipRotation) ? m_width : m_height;
#else
        int bufWidth = m_width;
        int bufHeight = m_height;
#endif
        LVGrayDrawBuf drawbuf(bufWidth, bufHeight, renderBpp);
        drawbuf.Clear(0xFFFFFF);
        drawbuf.setDitherImages(true);
        // Set position for correct page header progress bar
        docView->SetPos((*pages)[srcPageIdx]->start, false);
        docView->drawPageTo(&drawbuf, *(*pages)[srcPageIdx], NULL, exportPageCount, 0);

        // Apply rotation to get final target dimensions
#if CR_INTERNAL_PAGE_ORIENTATION == 1
        if (angle != CR_ROTATE_ANGLE_0 && !skipRotation) {
            drawbuf.Rotate(angle);
        }
#endif

        // Debug: dump page as BMP if enabled
        if (m_dumpImagesLimit != 0 && (m_dumpImagesLimit < 0 || dumpedCount < m_dumpImagesLimit)) {
            // Generate filename: page_000.bmp, page_001.bmp, etc.
            char bmpFilename[512];
            snprintf(bmpFilename, sizeof(bmpFilename), "%spage_%03d.bmp", m_dumpDir.c_str(), i);
            if (saveBmpFile(bmpFilename, drawbuf, bpp, m_grayPolicy)) {
                CRLog::info("XtcExporter: Dumped page %d to %s", i, bmpFilename);
                dumpedCount++;
            } else {
                CRLog::warn("XtcExporter: Failed to dump page %d to %s", i, bmpFilename);
            }
        }

        // Write page data and record index entry
        if (!writePage(stream.get(), drawbuf, pageIndex[i])) {
            CRLog::error("XtcExporter: Failed to write page %d (source page %d)", i, srcPageIdx);
            docView->Resize(save_dx, save_dy);
            return false;
        }
    }

    // Update page index with actual offsets
    stream->SetPos(indexPos);
    for (int i = 0; i < exportPageCount; i++) {
        if (stream->Write(&pageIndex[i], sizeof(xtc_page_index_t), NULL) != LVERR_OK) {
            docView->Resize(save_dx, save_dy);
            return false;
        }
    }

    // Final progress
    if (m_callback) {
        m_callback->onProgress(100);
    }

    // Restore view state
    docView->setPageHeaderInfo(old_flags);
    docView->Resize(save_dx, save_dy);
    docView->clearImageCache();
    docView->goToPage(save_page);

    CRLog::info("XtcExporter: Successfully exported %d pages (source pages %d-%d), %d chapters",
                exportPageCount, actualStartPage, actualEndPage, chapters.length());
    return true;
}
