/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2022 Aleksey Chernov <valexlin@gmail.com>               *
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

#include "mdfmt.h"

#if (USE_CMARK == 1) || (USE_CMARK_GFM == 1)

#include <ldomdocument.h>
#include <lvdocviewcallback.h>
#include <lvdocprops.h>
#include <lvstreamutils.h>

#include "lvxml/lvtextparser.h"
#include "lvxml/lvhtmlparser.h"
#include "lvstream/lvmemorystream.h"
#include "lvtinydom/ldomdocumentwriter.h"

#if USE_CMARK == 1
#include <cmark.h>
#elif USE_CMARK_GFM == 1
#include <cmark-gfm.h>
#include <cmark-gfm-core-extensions.h>
#include <parser.h>
#endif
#include <string.h>
#include <crlog.h>

#define TEXT_PARSER_CHUNK_SIZE 16384

bool DetectMarkdownFormat(LVStreamRef stream, const lString32& fileName) {
    // Check file extension
    lString32 nm = fileName;
    nm = nm.lowercase();
    if (!nm.endsWith(".md"))
        return false;
    // Check file size
    lvsize_t sz = stream->GetSize();
    if (sz < 5 || sz > MARKDOWN_MAX_FILE_SIZE)
        return false;
    // Checking for compliance with the text format
    LVTextParser textParser(stream, NULL, true);
    bool res = textParser.CheckFormat();
    stream->SetPos(0);
    return res;
}

bool ImportMarkdownDocument(LVStreamRef stream, const lString32& fileName, ldomDocument* doc, LVDocViewCallback* progressCallback, CacheLoadingCallback* formatCallback) {
    if (doc->openFromCache(formatCallback)) {
        if (progressCallback) {
            progressCallback->OnLoadFileEnd();
        }
        return true;
    }
    bool res = false;
    int cmark_options = CMARK_OPT_DEFAULT |
                        CMARK_OPT_VALIDATE_UTF8 | // Validate UTF-8 in the input before parsing, replacing illegal sequences with the replacement character U+FFFD
                        CMARK_OPT_UNSAFE;         // Render raw HTML and unsafe links
    cmark_node* document = NULL;
    cmark_parser* parser = cmark_parser_new(cmark_options);
#if USE_CMARK_GFM == 1
    cmark_gfm_core_extensions_ensure_registered();
    static const char* cmark_gfm_ext_names[] = {
        "strikethrough",
        "autolink",
        "table",
        "tagfilter",
        "tasklist",
        NULL
    };
    cmark_syntax_extension* extension;
    for (const char** ext_name = &cmark_gfm_ext_names[0]; NULL != *ext_name; ext_name++) {
        extension = cmark_find_syntax_extension(*ext_name);
        if (NULL != extension) {
            cmark_parser_attach_syntax_extension(parser, extension);
        }
    }
#endif
    // Read stream & feed to parser
    char buffer[TEXT_PARSER_CHUNK_SIZE];
    lvsize_t bytesRead = 0;
    stream->SetPos(0);
    while (stream->Read(buffer, TEXT_PARSER_CHUNK_SIZE, &bytesRead) == LVERR_OK) {
        cmark_parser_feed(parser, buffer, bytesRead);
        if (bytesRead < TEXT_PARSER_CHUNK_SIZE)
            break;
    }
    document = cmark_parser_finish(parser);
    // convert to html
#if USE_CMARK_GFM == 1
    char* result = cmark_render_html(document, cmark_options, parser->syntax_extensions);
#else
    char* result = cmark_render_html(document, cmark_options);
#endif
    cmark_parser_free(parser);
    cmark_node_free(document);
    // Write document content to stream to parse them
    lvsize_t result_len = strlen(result);
    lString32 title = LVExtractFilenameWithoutExtension(fileName);
    lString8 gen_preamble = cs8("<html><head><title>") + UnicodeToUtf8(title) + cs8("</title></head><body>");
    lString8 gen_tail = cs8("</body></html>");
    lvsize_t dw;
    LVMemoryStream* memStream = new LVMemoryStream;
    LVStreamRef memRef = LVStreamRef(memStream);
    res = LVERR_OK == memStream->Create();
    if (res)
        res = LVERR_OK == memStream->Write(gen_preamble.c_str(), gen_preamble.length(), &dw);
    if (res)
        res = dw == (lvsize_t)gen_preamble.length();
    if (res)
        res = LVERR_OK == memStream->Write(result, result_len, &dw);
    if (res)
        res = dw == result_len;
    if (res)
        res = LVERR_OK == memStream->Write(gen_tail.c_str(), gen_tail.length(), &dw);
    if (res)
        res = dw == (lvsize_t)gen_tail.length();
    // html result is no longer needed
    free(result);
    if (res) {
        // Parse stream to document
        ldomDocumentWriter writer(doc);
        LVHTMLParser parser(memRef, &writer);
        parser.setProgressCallback(progressCallback);
        res = parser.CheckFormat() && parser.Parse();
    }
    if (res) {
        doc->getProps()->setString(DOC_PROP_TITLE, title);
        doc->buildTocFromHeadings();
    }
    return res;
}

#endif // (USE_CMARK == 1) || (USE_CMARK_GFM == 1)
