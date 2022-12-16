/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2008-2011 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2018,2020 poire-z <poire-z@users.noreply.github.com>    *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
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

#ifndef __LDOMDOCUMENTWRITERFILTER_H_INCLUDED__
#define __LDOMDOCUMENTWRITERFILTER_H_INCLUDED__

#include "ldomdocumentwriter.h"

#include <lxmldocbase.h>

/** 
 * \brief callback object to fill DOM tree
 * To be used with XML parser as callback object.
 * Creates document according to incoming events.
 * Autoclose HTML tags.
 */
class ldomDocumentWriterFilter: public ldomDocumentWriter
{
protected:
    bool _libRuDocumentToDetect;
    bool _libRuDocumentDetected;
    bool _libRuParagraphStart;
    bool _libRuParseAsPre;
    lUInt16 _styleAttrId;
    lUInt16 _classAttrId;
    lUInt16* _rules[MAX_ELEMENT_TYPE_ID];
    bool _tagBodyCalled;
    // Some states used when gDOMVersionRequested >= 20200824
    bool _htmlTagSeen;
    bool _headTagSeen;
    bool _bodyTagSeen;
    bool _curNodeIsSelfClosing;
    bool _curTagIsIgnored;
    ldomElementWriter* _curNodeBeforeFostering;
    ldomElementWriter* _curFosteredNode;
    ldomElementWriter* _lastP;
    virtual void AutoClose(lUInt16 tag_id, bool open);
    virtual bool AutoOpenClosePop(int step, lUInt16 tag_id);
    virtual lUInt16 popUpTo(ldomElementWriter* target, lUInt16 target_id = 0, int scope = 0);
    virtual bool CheckAndEnsureFosterParenting(lUInt16 tag_id);
    virtual void ElementCloseHandler(ldomNode* node);
    virtual void appendStyle(const lChar32* style);
    virtual void setClass(const lChar32* className, bool overrideExisting = false);
public:
    /// called on attribute
    virtual void OnAttribute(const lChar32* nsname, const lChar32* attrname, const lChar32* attrvalue);
    /// called on opening tag
    virtual ldomNode* OnTagOpen(const lChar32* nsname, const lChar32* tagname);
    /// called after > of opening tag (when entering tag body)
    virtual void OnTagBody();
    /// called on closing tag
    virtual void OnTagClose(const lChar32* nsname, const lChar32* tagname, bool self_closing_tag = false);
    /// called on text
    virtual void OnText(const lChar32* text, int len, lUInt32 flags);
    /// constructor
    ldomDocumentWriterFilter(ldomDocument* document, bool headerOnly, const char*** rules);
    /// destructor
    virtual ~ldomDocumentWriterFilter();
};

#endif // __LDOMDOCUMENTWRITERFILTER_H_INCLUDED__
