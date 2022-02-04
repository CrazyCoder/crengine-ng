/**
 * CoolReader Engine
 *
 * (c) Vadim Lopatin, 2000-2011
 * (c) Other CoolReader authors (See AUTHORS file)
 *
 * This source code is distributed under the terms of
 * GNU General Public License version 2
 * See LICENSE file for details
 */

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
