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
 * @file xtcexport.h
 * @brief XTC/XTCH format exporter for e-book to e-paper format conversion
 *
 * Provides a builder-pattern API for exporting documents to XTC (1-bit) or
 * XTCH (2-bit) container formats for Xteink e-paper displays.
 */

#ifndef XTC_EXPORT_H
#define XTC_EXPORT_H

#include <xtcfmt.h>
#include <lvgraydrawbuf.h>
#include <lvstream.h>
#include <lvstring.h>
#include <lvptrvec.h>

class LVDocView;
class LVTocItem;

// =============================================================================
// XTC Export Format Selection
// =============================================================================

enum XtcExportFormat
{
    XTC_FORMAT_XTC = 0,   ///< XTC with XTG (1-bit) pages
    XTC_FORMAT_XTCH = 1   ///< XTCH with XTH (2-bit) pages
};

// =============================================================================
// Gray to Monochrome Conversion Policy
// =============================================================================

/**
 * @brief Policy for converting 2-bit grayscale to 1-bit monochrome
 *
 * When exporting to XTG (1-bit) format, grayscale pixels must be converted
 * to black or white. This policy controls how intermediate gray values are mapped.
 *
 * In LVGrayDrawBuf 2-bit mode: 0=black, 1=dark gray, 2=light gray, 3=white
 */
enum GrayToMonoPolicy
{
    GRAY_SPLIT_LIGHT_DARK = 0,  ///< 0,1 -> black, 2,3 -> white (threshold at mid-gray)
    GRAY_ALL_TO_WHITE = 1,      ///< Only 0 stays black; 1,2,3 -> white (preserve light details)
    GRAY_ALL_TO_BLACK = 2       ///< Only 3 stays white; 0,1,2 -> black (preserve dark details)
};

// =============================================================================
// Progress Callback Interface
// =============================================================================

class XtcExportCallback
{
public:
    virtual ~XtcExportCallback() {}
    /// Called during export with progress percentage (0-100)
    virtual void onProgress(int percent) = 0;
};

// =============================================================================
// XtgWriter - Writes 1-bit XTG bitmap data
// =============================================================================

/**
 * @brief Converts LVGrayDrawBuf to XTG (1-bit) format
 *
 * XTG stores 1-bit per pixel monochrome bitmaps.
 * Pixel storage: rows top to bottom, 8 pixels per byte, MSB first.
 * Bit value: 0 = black, 1 = white
 */
class XtgWriter
{
public:
    /**
     * @brief Write XTG image to stream
     * @param stream Output stream
     * @param buf Source grayscale buffer (will be converted to 1-bit)
     * @param grayPolicy Policy for converting grayscale to monochrome (default: GRAY_SPLIT_LIGHT_DARK)
     * @return true on success
     */
    static bool write(LVStream* stream, LVGrayDrawBuf& buf,
                      GrayToMonoPolicy grayPolicy = GRAY_SPLIT_LIGHT_DARK);

    /**
     * @brief Get XTG data size (header + image data)
     * @param width Image width
     * @param height Image height
     * @return Total size in bytes
     */
    static uint32_t getTotalSize(uint16_t width, uint16_t height);
};

// =============================================================================
// XthWriter - Writes 2-bit XTH bitmap data
// =============================================================================

/**
 * @brief Converts LVGrayDrawBuf to XTH (2-bit, 4-level grayscale) format
 *
 * XTH stores 2-bit per pixel using two separate bit planes.
 * Pixel value: 0=white, 1=light gray, 2=dark gray, 3=black
 * Storage: First all bit1 values packed, then all bit2 values packed.
 */
class XthWriter
{
public:
    /**
     * @brief Write XTH image to stream
     * @param stream Output stream
     * @param buf Source grayscale buffer (2-bit preferred, others converted)
     * @return true on success
     */
    static bool write(LVStream* stream, LVGrayDrawBuf& buf);

    /**
     * @brief Get XTH data size (header + image data)
     * @param width Image width
     * @param height Image height
     * @return Total size in bytes
     */
    static uint32_t getTotalSize(uint16_t width, uint16_t height);
};

// =============================================================================
// XtcExporter - Builder pattern for XTC/XTCH export
// =============================================================================

/**
 * @brief XTC/XTCH container exporter with builder pattern
 *
 * Usage example:
 * @code
 *   XtcExporter exporter;
 *   exporter.setFormat(XTC_FORMAT_XTCH)
 *           .setDimensions(480, 800)
 *           .setMetadata("Book Title", "Author Name")
 *           .setReadingDirection(XTC_DIR_LTR);
 *   exporter.exportDocument(docView, "output.xtch");
 * @endcode
 */
class XtcExporter
{
public:
    XtcExporter();
    ~XtcExporter();

    // =========================================================================
    // Builder methods (return *this for chaining)
    // =========================================================================

    /// Set export format (XTC or XTCH)
    XtcExporter& setFormat(XtcExportFormat format);

    /// Set output dimensions in pixels
    XtcExporter& setDimensions(uint16_t width, uint16_t height);

    /// Set basic metadata (title and author)
    XtcExporter& setMetadata(const lString8& title, const lString8& author);

    /// Set extended metadata
    XtcExporter& setMetadata(const lString8& title, const lString8& author,
                             const lString8& publisher, const lString8& language);

    /// Set reading direction
    XtcExporter& setReadingDirection(uint8_t direction);

    /// Enable chapter markers from document TOC
    XtcExporter& enableChapters(bool enable = true);

    /// Set maximum TOC depth to include (default: 2, 0 = unlimited)
    XtcExporter& setChapterDepth(int maxDepth);

    /// Enable thumbnail generation
    XtcExporter& enableThumbnails(bool enable = true, uint16_t thumbWidth = 120, uint16_t thumbHeight = 160);

    /// Set grayscale to monochrome conversion policy (only affects XTC/XTG output)
    XtcExporter& setGrayPolicy(GrayToMonoPolicy policy);

    /// Set page range to export (0-based page numbers, -1 means no limit)
    /// Exported file will have pages numbered from 0 to (endPage - startPage)
    XtcExporter& setPageRange(int startPage, int endPage = -1);

    /// Set progress callback
    XtcExporter& setProgressCallback(XtcExportCallback* callback);

    /// Enable debug image dumping (saves pages as BMP files alongside export)
    /// @param limit Number of pages to dump: 0 = disabled (default), -1 = all pages, N = first N pages
    XtcExporter& dumpImages(int limit);

    // =========================================================================
    // Export methods
    // =========================================================================

    /**
     * @brief Export document to XTC/XTCH file
     * @param docView Document view to export
     * @param filename Output filename
     * @return true on success
     */
    bool exportDocument(LVDocView* docView, const lChar32* filename);

    /**
     * @brief Export document to XTC/XTCH stream
     * @param docView Document view to export
     * @param stream Output stream
     * @return true on success
     */
    bool exportDocument(LVDocView* docView, LVStreamRef stream);

    /**
     * @brief Export a single page buffer to XTG/XTH format (for testing)
     * @param buf Grayscale buffer
     * @param filename Output filename
     * @return true on success
     */
    bool exportSinglePage(LVGrayDrawBuf& buf, const lChar32* filename);

private:
    // Configuration
    XtcExportFormat m_format;
    uint16_t m_width;
    uint16_t m_height;
    uint8_t m_readDirection;
    bool m_enableChapters;
    int m_chapterDepth;
    bool m_enableThumbnails;
    uint16_t m_thumbWidth;
    uint16_t m_thumbHeight;
    GrayToMonoPolicy m_grayPolicy;
    int m_startPage;  ///< First page to export (0-based, -1 = from beginning)
    int m_endPage;    ///< Last page to export (0-based, inclusive, -1 = to end)
    int m_dumpImagesLimit;  ///< Number of pages to dump as BMP: 0 = disabled, -1 = all, N = first N
    lString8 m_dumpDir;     ///< Directory for BMP dump files (set from output filename)

    // Metadata
    lString8 m_title;
    lString8 m_author;
    lString8 m_publisher;
    lString8 m_language;

    // Callback
    XtcExportCallback* m_callback;

    // Internal helpers
    bool writeHeader(LVStream* stream, uint16_t pageCount, bool hasChapters);
    bool writeMetadata(LVStream* stream, LVDocView* docView, uint16_t chapterCount);
    int collectChapters(LVTocItem* item, LVArray<xtc_chapter_t>& chapters, int maxDepth, int currentDepth);
    bool writeChapters(LVStream* stream, const LVArray<xtc_chapter_t>& chapters);
    bool writePageIndex(LVStream* stream, const LVArray<xtc_page_index_t>& pageIndex);
    bool writePage(LVStream* stream, LVGrayDrawBuf& buf, xtc_page_index_t& indexEntry);
};

#endif // XTC_EXPORT_H