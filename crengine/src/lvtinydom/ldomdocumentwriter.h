/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007-2011,2014 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
 *   Copyright (C) 2018,2020,2021 poire-z <poire-z@users.noreply.github.com>
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

#ifndef __LDOMDOCUMENTWRITER_H_INCLUDED__
#define __LDOMDOCUMENTWRITER_H_INCLUDED__

#include <lvxmlparsercallback.h>
#include <lvstring32collection.h>

#if MATHML_SUPPORT == 1
#include "../mathml.h"
#endif

class ldomElementWriter;
class ldomDocument;

/** 
 *  \brief callback object to fill DOM tree
 *  To be used with XML parser as callback object.
 *  Creates document according to incoming events.
 */
class ldomDocumentWriter: public LVXMLParserCallback
{
#if MATHML_SUPPORT == 1
    friend class MathMLHelper;
#endif
protected:
    //============================
    ldomDocument* _document;
    //ldomElement * _currNode;
    ldomElementWriter* _currNode;
    bool _errFlag;
    bool _headerOnly;
    bool _popStyleOnFinish;
    lUInt16 _stopTagId;
    //============================
    lUInt32 _flags;
    bool _inHeadStyle;
    lString32 _headStyleText;
    lString32Collection _stylesheetLinks;
#if MATHML_SUPPORT == 1
    MathMLHelper _mathMLHelper;
#endif
    virtual void ElementCloseHandler(ldomNode* node);
public:
    /// returns flags
    virtual lUInt32 getFlags() {
        return _flags;
    }
    /// sets flags
    virtual void setFlags(lUInt32 flags) {
        _flags = flags;
    }
    // overrides
    /// called when encoding directive found in document
    virtual void OnEncoding(const lChar32* name, const lChar32* table);
    /// called on parsing start
    virtual void OnStart(LVFileFormatParser* parser);
    /// called on parsing end
    virtual void OnStop();
    /// called on opening tag
    virtual ldomNode* OnTagOpen(const lChar32* nsname, const lChar32* tagname);
    /// called after > of opening tag (when entering tag body)
    virtual void OnTagBody();
    /// called on closing tag
    virtual void OnTagClose(const lChar32* nsname, const lChar32* tagname, bool self_closing_tag = false);
    /// called on attribute
    virtual void OnAttribute(const lChar32* nsname, const lChar32* attrname, const lChar32* attrvalue);
    /// close tags
    ldomElementWriter* pop(ldomElementWriter* obj, lUInt16 id);
    /// called on text
    virtual void OnText(const lChar32* text, int len, lUInt32 flags);
    /// add named BLOB data to document
    virtual bool OnBlob(lString32 name, const lUInt8* data, int size);
    /// set document property
    virtual void OnDocProperty(const char* name, lString8 value);

    /// constructor
    ldomDocumentWriter(ldomDocument* document, bool headerOnly = false);
    /// destructor
    virtual ~ldomDocumentWriter();
};

#endif // __LDOMDOCUMENTWRITER_H_INCLUDED__
