/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007-2010,2012 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2012 Daniel Savard <daniels@xsoli.com>                  *
 *   Copyright (C) 2020 poire-z <poire-z@users.noreply.github.com>         *
 *   Copyright (C) 2020 NiLuJe <ninuje@gmail.com>                          *
 *   Copyright (C) 2020,2023 Aleksey Chernov <valexlin@gmail.com>          *
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

#include <lvtinydomutils.h>
#include <lvtinydom_common.h>
#include <ldomxpointer.h>
#include <ldomdocument.h>

const int gDOMVersionCurrent = DOM_VERSION_CURRENT;

//=====================================================
// Document data caching parameters
//=====================================================

#include <fb2def.h>

#include "ldomdocumentwriterfilter.h"

#include "../lvxml/lvxmlutils.h"
#include "../lvxml/lvhtmlparser.h"

//#define INDEX1 94
//#define INDEX2 96

//#define INDEX1 105
//#define INDEX2 106

/*

  Struct Node
  { document, nodeid&type, address }

  Data Offset format

  Chunk index, offset, type.

  getDataPtr( lUInt32 address )
  {
     return (address & TYPE_MASK) ? textStorage.get( address & ~TYPE_MASK ) : elementStorage.get( address & ~TYPE_MASK );
  }

  index->instance, data
  >
  [index] { vtable, doc, id, dataptr } // 16 bytes per node


 */

lString32 extractDocAuthors(ldomDocument* doc, lString32 delimiter, bool shortMiddleName) {
    if (delimiter.empty())
        delimiter = ", ";
    // We are using a specific delimiter here, so if you change it here you must also
    // change the authors parsing code in the client application.
    lString32 authors;
    for (int i = 0; i < 16; i++) {
        lString32 path = cs32("/FictionBook/description/title-info/author[") + fmt::decimal(i + 1) + "]";
        ldomXPointer pauthor = doc->createXPointer(path);
        if (!pauthor) {
            //CRLog::trace( "xpath not found: %s", UnicodeToUtf8(path).c_str() );
            break;
        }
        lString32 firstName = pauthor.relative(U"/first-name").getText().trim();
        lString32 lastName = pauthor.relative(U"/last-name").getText().trim();
        lString32 middleName = pauthor.relative(U"/middle-name").getText().trim();
        lString32 author = firstName;
        if (!middleName.empty()) {
            if (!author.empty())
                author += " ";
            author += shortMiddleName ? lString32(middleName, 0, 1) + "." : middleName;
        }
        if (!lastName.empty()) {
            if (!author.empty())
                author += " ";
            author += lastName;
        }
        if (!authors.empty())
            authors += delimiter;
        authors += author;
    }
    return authors;
}

lString32 extractDocTitle(ldomDocument* doc) {
    return doc->createXPointer(U"/FictionBook/description/title-info/book-title").getText().trim();
}

lString32 extractDocLanguage(ldomDocument* doc) {
    return doc->createXPointer(U"/FictionBook/description/title-info/lang").getText().trim();
}

lString32 extractDocSeries(ldomDocument* doc, int* pSeriesNumber) {
    lString32 res;
    ldomNode* series = doc->createXPointer(U"/FictionBook/description/title-info/sequence").getNode();
    if (series) {
        lString32 sname = lString32(series->getAttributeValue(attr_name)).trim();
        lString32 snumber = series->getAttributeValue(attr_number);
        if (!sname.empty()) {
            if (pSeriesNumber) {
                *pSeriesNumber = snumber.atoi();
                res = sname;
            } else {
                res << "(" << sname;
                if (!snumber.empty())
                    res << " #" << snumber << ")";
            }
        }
    }
    return res;
}

lString32 extractDocKeywords(ldomDocument* doc, lString32 delimiter) {
    lString32 res;
    if (delimiter.empty())
        delimiter = ", ";
    // Genres
    // We are using a specific delimiter here, so if you change it here you must also
    // change the keyword parsing code in the client application.
    for (int i = 0; i < 16; i++) {
        lString32 path = cs32("/FictionBook/description/title-info/genre[") + fmt::decimal(i + 1) + "]";
        ldomXPointer genre = doc->createXPointer(path);
        if (!genre) {
            break;
        }
        lString32 text = genre.getText().trim();
        if (!text.empty()) {
            if (!res.empty())
                res << delimiter;
            res << text;
        }
    }
    return res;
}

lString32 extractDocDescription(ldomDocument* doc) {
    // We put all other FB2 meta info in this description
    lString32 res;

    // Annotation (description)
    res << doc->createXPointer(U"/FictionBook/description/title-info/annotation").getText().trim();

    // Translators
    lString32 translators;
    int nbTranslators = 0;
    for (int i = 0; i < 16; i++) {
        lString32 path = cs32("/FictionBook/description/title-info/translator[") + fmt::decimal(i + 1) + "]";
        ldomXPointer ptranslator = doc->createXPointer(path);
        if (!ptranslator) {
            break;
        }
        lString32 firstName = ptranslator.relative(U"/first-name").getText().trim();
        lString32 lastName = ptranslator.relative(U"/last-name").getText().trim();
        lString32 middleName = ptranslator.relative(U"/middle-name").getText().trim();
        lString32 translator = firstName;
        if (!translator.empty())
            translator += " ";
        if (!middleName.empty())
            translator += middleName;
        if (!lastName.empty() && !translator.empty())
            translator += " ";
        translator += lastName;
        if (!translators.empty())
            translators << "\n";
        translators << translator;
        nbTranslators++;
    }
    if (!translators.empty()) {
        if (!res.empty())
            res << "\n\n";
        if (nbTranslators > 1)
            res << "Translators:\n"
                << translators;
        else
            res << "Translator: " << translators;
    }

    // Publication info & publisher
    ldomXPointer publishInfo = doc->createXPointer(U"/FictionBook/description/publish-info");
    if (!publishInfo.isNull()) {
        lString32 publisher = publishInfo.relative(U"/publisher").getText().trim();
        lString32 pubcity = publishInfo.relative(U"/city").getText().trim();
        lString32 pubyear = publishInfo.relative(U"/year").getText().trim();
        lString32 isbn = publishInfo.relative(U"/isbn").getText().trim();
        lString32 bookName = publishInfo.relative(U"/book-name").getText().trim();
        lString32 publication;
        if (!publisher.empty() || !pubcity.empty()) {
            if (!publisher.empty()) {
                publication << publisher;
            }
            if (!pubcity.empty()) {
                if (!!publisher.empty()) {
                    publication << ", ";
                }
                publication << pubcity;
            }
        }
        if (!pubyear.empty() || !isbn.empty()) {
            if (!publication.empty())
                publication << "\n";
            if (!pubyear.empty()) {
                publication << pubyear;
            }
            if (!isbn.empty()) {
                if (!pubyear.empty()) {
                    publication << ", ";
                }
                publication << isbn;
            }
        }
        if (!bookName.empty()) {
            if (!publication.empty())
                publication << "\n";
            publication << bookName;
        }
        if (!publication.empty()) {
            if (!res.empty())
                res << "\n\n";
            res << "Publication:\n"
                << publication;
        }
    }

    // Document info
    ldomXPointer pDocInfo = doc->createXPointer(U"/FictionBook/description/document-info");
    if (!pDocInfo.isNull()) {
        lString32 docInfo;
        lString32 docAuthors;
        int nbAuthors = 0;
        for (int i = 0; i < 16; i++) {
            lString32 path = cs32("/FictionBook/description/document-info/author[") + fmt::decimal(i + 1) + "]";
            ldomXPointer pdocAuthor = doc->createXPointer(path);
            if (!pdocAuthor) {
                break;
            }
            lString32 firstName = pdocAuthor.relative(U"/first-name").getText().trim();
            lString32 lastName = pdocAuthor.relative(U"/last-name").getText().trim();
            lString32 middleName = pdocAuthor.relative(U"/middle-name").getText().trim();
            lString32 docAuthor = firstName;
            if (!docAuthor.empty())
                docAuthor += " ";
            if (!middleName.empty())
                docAuthor += middleName;
            if (!lastName.empty() && !docAuthor.empty())
                docAuthor += " ";
            docAuthor += lastName;
            if (!docAuthors.empty())
                docAuthors << "\n";
            docAuthors << docAuthor;
            nbAuthors++;
        }
        if (!docAuthors.empty()) {
            if (nbAuthors > 1)
                docInfo << "Authors:\n"
                        << docAuthors;
            else
                docInfo << "Author: " << docAuthors;
        }
        lString32 docPublisher = pDocInfo.relative(U"/publisher").getText().trim();
        lString32 docId = pDocInfo.relative(U"/id").getText().trim();
        lString32 docVersion = pDocInfo.relative(U"/version").getText().trim();
        lString32 docDate = pDocInfo.relative(U"/date").getText().trim();
        lString32 docHistory = pDocInfo.relative(U"/history").getText().trim();
        lString32 docSrcUrl = pDocInfo.relative(U"/src-url").getText().trim();
        lString32 docSrcOcr = pDocInfo.relative(U"/src-ocr").getText().trim();
        lString32 docProgramUsed = pDocInfo.relative(U"/program-used").getText().trim();
        if (!docPublisher.empty()) {
            if (!docInfo.empty())
                docInfo << "\n";
            docInfo << "Publisher: " << docPublisher;
        }
        if (!docId.empty()) {
            if (!docInfo.empty())
                docInfo << "\n";
            docInfo << "Id: " << docId;
        }
        if (!docVersion.empty()) {
            if (!docInfo.empty())
                docInfo << "\n";
            docInfo << "Version: " << docVersion;
        }
        if (!docDate.empty()) {
            if (!docInfo.empty())
                docInfo << "\n";
            docInfo << "Date: " << docDate;
        }
        if (!docHistory.empty()) {
            if (!docInfo.empty())
                docInfo << "\n";
            docInfo << "History: " << docHistory;
        }
        if (!docSrcUrl.empty()) {
            if (!docInfo.empty())
                docInfo << "\n";
            docInfo << "URL: " << docSrcUrl;
        }
        if (!docSrcOcr.empty()) {
            if (!docInfo.empty())
                docInfo << "\n";
            docInfo << "OCR: " << docSrcOcr;
        }
        if (!docProgramUsed.empty()) {
            if (!docInfo.empty())
                docInfo << "\n";
            docInfo << "Application: " << docProgramUsed;
        }
        if (!docInfo.empty()) {
            if (!res.empty())
                res << "\n\n";
            res << "Document:\n"
                << docInfo;
        }
    }

    return res;
}

ldomDocument* LVParseXMLStream(LVStreamRef stream,
                               const elem_def_t* elem_table,
                               const attr_def_t* attr_table,
                               const ns_def_t* ns_table) {
    if (stream.isNull())
        return NULL;
    bool error = true;
    ldomDocument* doc;
    doc = new ldomDocument();
    doc->setDocFlags(0);

    ldomDocumentWriter writer(doc);
    doc->setNodeTypes(elem_table);
    doc->setAttributeTypes(attr_table);
    doc->setNameSpaceTypes(ns_table);

    /// FB2 format
    LVFileFormatParser* parser = new LVXMLParser(stream, &writer);
    if (parser->CheckFormat()) {
        if (parser->Parse()) {
            error = false;
        }
    }
    delete parser;
    if (error) {
        delete doc;
        doc = NULL;
    }
    return doc;
}

ldomDocument* LVParseHTMLStream(LVStreamRef stream,
                                const elem_def_t* elem_table,
                                const attr_def_t* attr_table,
                                const ns_def_t* ns_table) {
    if (stream.isNull())
        return NULL;
    bool error = true;
    ldomDocument* doc;
    doc = new ldomDocument();
    doc->setDocFlags(0);

    ldomDocumentWriterFilter writerFilter(doc, false, HTML_AUTOCLOSE_TABLE);
    doc->setNodeTypes(elem_table);
    doc->setAttributeTypes(attr_table);
    doc->setNameSpaceTypes(ns_table);

    /// FB2 format
    LVFileFormatParser* parser = new LVHTMLParser(stream, &writerFilter);
    if (parser->CheckFormat()) {
        if (parser->Parse()) {
            error = false;
        }
    }
    delete parser;
    if (error) {
        delete doc;
        doc = NULL;
    }
    return doc;
}
