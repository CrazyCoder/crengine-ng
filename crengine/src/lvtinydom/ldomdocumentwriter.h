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

#ifndef __LDOMDOCUMENTWRITER_H_INCLUDED__
#define __LDOMDOCUMENTWRITER_H_INCLUDED__

#include <lvxmlparsercallback.h>
#include <lvstring32collection.h>
#include <crmathml.h> // TODO: Move this header to source (private) dir

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
