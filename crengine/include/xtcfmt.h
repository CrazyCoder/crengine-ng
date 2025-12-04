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

/**
 * @file xtcfmt.h
 * @brief XTC/XTG/XTH format structures for Xteink e-paper displays
 *
 * XTG - 1-bit monochrome bitmap format
 * XTH - 2-bit (4-level grayscale) bitmap format
 * XTC - Container format with multiple XTG pages
 * XTCH - Container format variant with XTH pages
 *
 * All multi-byte values are Little-Endian.
 */

#ifndef XTC_FMT_H
#define XTC_FMT_H

#include <stdint.h>

// =============================================================================
// Magic signatures (Little-Endian uint32_t)
// =============================================================================

#define XTG_MAGIC  0x00475458  // "XTG\0"
#define XTH_MAGIC  0x00485458  // "XTH\0"
#define XTC_MAGIC  0x00435458  // "XTC\0"
#define XTCH_MAGIC 0x48435458  // "XTCH"

// =============================================================================
// XTC version
// =============================================================================

#define XTC_VERSION 0x0100  // v1.0

// =============================================================================
// Reading direction values
// =============================================================================

#define XTC_DIR_LTR 0  // Left to right (normal)
#define XTC_DIR_RTL 1  // Right to left (manga)
#define XTC_DIR_TTB 2  // Top to bottom (vertical)

// =============================================================================
// Compiler-specific packing
// =============================================================================

#ifdef __GNUC__
#define XTC_PACKED __attribute__((packed))
#else
#define XTC_PACKED
#endif

#pragma pack(push, 1)

// =============================================================================
// XTG/XTH Image Header (22 bytes)
// =============================================================================

/**
 * @brief XTG/XTH image header structure
 *
 * Used for both standalone XTG/XTH files and pages within XTC containers.
 * The magic field determines the format (XTG or XTH).
 */
typedef struct
{
    uint32_t magic;        ///< File identifier: XTG_MAGIC or XTH_MAGIC
    uint16_t width;        ///< Image width in pixels (1-65535)
    uint16_t height;       ///< Image height in pixels (1-65535)
    uint8_t colorMode;     ///< Color mode (0 = monochrome/grayscale)
    uint8_t compression;   ///< Compression (0 = uncompressed)
    uint32_t dataSize;     ///< Image data size in bytes
    uint64_t md5;          ///< MD5 checksum (first 8 bytes, optional)
} XTC_PACKED xtg_header_t;

// Verify header size at compile time
static_assert(sizeof(xtg_header_t) == 22, "xtg_header_t must be 22 bytes");

// =============================================================================
// XTC Container Header (48 bytes)
// =============================================================================

/**
 * @brief XTC/XTCH container header structure
 *
 * The magic field determines the variant:
 * - XTC_MAGIC: Container with XTG (1-bit) pages
 * - XTCH_MAGIC: Container with XTH (2-bit) pages
 */
typedef struct
{
    uint32_t magic;           ///< File identifier: XTC_MAGIC or XTCH_MAGIC
    uint16_t version;         ///< Version number (XTC_VERSION)
    uint16_t pageCount;       ///< Total number of pages (1-65535)
    uint8_t readDirection;    ///< Reading direction (XTC_DIR_*)
    uint8_t hasMetadata;      ///< Has metadata section (0 or 1)
    uint8_t hasThumbnails;    ///< Has thumbnails section (0 or 1)
    uint8_t hasChapters;      ///< Has chapters section (0 or 1)
    uint32_t currentPage;     ///< Current/last read page (1-based, 0 = none)
    uint64_t metadataOffset;  ///< Offset to metadata section
    uint64_t indexOffset;     ///< Offset to page index table
    uint64_t dataOffset;      ///< Offset to page data area
    uint64_t thumbOffset;     ///< Offset to thumbnail area
} XTC_PACKED xtc_header_t;

static_assert(sizeof(xtc_header_t) == 48, "xtc_header_t must be 48 bytes");

// =============================================================================
// XTC Metadata Structure (256 bytes)
// =============================================================================

/**
 * @brief XTC metadata structure
 *
 * All string fields are UTF-8, null-terminated, with remaining space zero-filled.
 */
typedef struct
{
    char title[128];        ///< Book title (UTF-8, null-terminated)
    char author[64];        ///< Author name (UTF-8, null-terminated)
    char publisher[32];     ///< Publisher/source (UTF-8, null-terminated)
    char language[16];      ///< Language code (e.g., "en", "zh-CN")
    uint32_t createTime;    ///< Creation time (Unix timestamp)
    uint16_t coverPage;     ///< Cover page index (0-based, 0xFFFF = none)
    uint16_t chapterCount;  ///< Number of chapters
    uint64_t reserved;      ///< Reserved (zero-filled)
} XTC_PACKED xtc_metadata_t;

static_assert(sizeof(xtc_metadata_t) == 256, "xtc_metadata_t must be 256 bytes");

// =============================================================================
// XTC Chapter Structure (96 bytes per chapter)
// =============================================================================

/**
 * @brief XTC chapter entry structure
 */
typedef struct
{
    char chapterName[80];  ///< Chapter name (UTF-8, null-terminated)
    uint16_t startPage;    ///< Start page (0-based)
    uint16_t endPage;      ///< End page (0-based, inclusive)
    uint32_t reserved1;    ///< Reserved 1 (zero-filled)
    uint32_t reserved2;    ///< Reserved 2 (zero-filled)
    uint32_t reserved3;    ///< Reserved 3 (zero-filled)
} XTC_PACKED xtc_chapter_t;

static_assert(sizeof(xtc_chapter_t) == 96, "xtc_chapter_t must be 96 bytes");

// =============================================================================
// XTC Page Index Entry (16 bytes per page)
// =============================================================================

/**
 * @brief XTC page index entry
 *
 * Provides O(1) random access to any page in the container.
 */
typedef struct
{
    uint64_t offset;  ///< XTG/XTH data offset (absolute, from file start)
    uint32_t size;    ///< XTG/XTH data size (including 22-byte header)
    uint16_t width;   ///< Page width in pixels
    uint16_t height;  ///< Page height in pixels
} XTC_PACKED xtc_page_index_t;

static_assert(sizeof(xtc_page_index_t) == 16, "xtc_page_index_t must be 16 bytes");

#pragma pack(pop)

// =============================================================================
// Data Size Calculation Helpers
// =============================================================================

/**
 * @brief Calculate XTG (1-bit) image data size
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @return Data size in bytes
 */
static inline uint32_t xtg_data_size(uint16_t width, uint16_t height) {
    return ((width + 7) / 8) * height;
}

/**
 * @brief Calculate XTH (2-bit) image data size
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @return Data size in bytes (both bit planes combined)
 */
static inline uint32_t xth_data_size(uint16_t width, uint16_t height) {
    uint32_t pixelCount = (uint32_t)width * height;
    uint32_t planeSize = (pixelCount + 7) / 8;
    return planeSize * 2;  // Two bit planes
}

#endif // XTC_FMT_H