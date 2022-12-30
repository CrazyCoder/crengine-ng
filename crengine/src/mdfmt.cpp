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

#if USE_CMARK == 1

#include <ldomdocument.h>
#include <lvdocviewcallback.h>

#include "lvxml/lvtextparser.h"
#include "lvxml/lvhtmlparser.h"
#include "lvstream/lvmemorystream.h"
#include "lvtinydom/ldomdocumentwriter.h"

#include <cmark.h>
#include <string.h>
#include <crlog.h>

#define TEXT_PARSER_CHUNK_SIZE 16384

static const char md_gen_preamble[] = "<html><head></head><body>";
static const char md_gen_tail[] = "</body></html>";

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

bool ImportMarkdownDocument(LVStreamRef stream, ldomDocument* doc, LVDocViewCallback* progressCallback, CacheLoadingCallback* formatCallback) {
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
    cmark_parser_free(parser);
    // convert to html
    char* result = cmark_render_html(document, cmark_options);
    cmark_node_free(document);
    // Write document content to stream to parse them
    lvsize_t result_len = strlen(result);
    lvsize_t dw;
    LVMemoryStream* memStream = new LVMemoryStream;
    LVStreamRef memRef = LVStreamRef(memStream);
    res = LVERR_OK == memStream->Create();
    if (res)
        res = LVERR_OK == memStream->Write(md_gen_preamble, sizeof(md_gen_preamble), &dw);
    if (res)
        res = dw == sizeof(md_gen_preamble);
    if (res)
        res = LVERR_OK == memStream->Write(result, result_len, &dw);
    if (res)
        res = dw == result_len;
    if (res)
        res = LVERR_OK == memStream->Write(md_gen_tail, sizeof(md_gen_tail), &dw);
    if (res)
        res = dw == sizeof(md_gen_tail);
    // html result is no longer needed
    free(result);
    if (res) {
        // Parse stream to document
        ldomDocumentWriter writer(doc);
        LVHTMLParser parser(memRef, &writer);
        parser.setProgressCallback(progressCallback);
        res = parser.CheckFormat() && parser.Parse();
    }
    return res;
}

#endif // USE_CMARK == 1
