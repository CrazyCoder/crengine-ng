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

#ifndef __LDOMDOCUMENTFRAGMENTWRITER_H_INCLUDED__
#define __LDOMDOCUMENTFRAGMENTWRITER_H_INCLUDED__

#include <lvxmlparsercallback.h>
#include <lvstring32collection.h>
#include <lvhashtable.h>

class ldomDocumentFragmentWriter: public LVXMLParserCallback
{
private:
    //============================
    LVXMLParserCallback* parent;
    lString32 baseTag;
    lString32 baseTagReplacement;
    lString32 codeBase;
    lString32 filePathName;
    lString32 codeBasePrefix;
    lString32 stylesheetFile;
    lString32 tmpStylesheetFile;
    lString32Collection stylesheetLinks;
    bool insideTag;
    int styleDetectionState;
    LVHashTable<lString32, lString32> pathSubstitutions;

    ldomNode* baseElement;
    ldomNode* lastBaseElement;

    lString8 headStyleText;
    int headStyleState;

    lString32 htmlDir;
    lString32 htmlLang;
    bool insideHtmlTag;

    bool m_nonlinear = false;
public:
    /// return content of html/head/style element
    lString8 getHeadStyleText() {
        return headStyleText;
    }

    ldomNode* getBaseElement() {
        return lastBaseElement;
    }

    lString32 convertId(lString32 id);
    lString32 convertHref(lString32 href);

    void addPathSubstitution(lString32 key, lString32 value) {
        pathSubstitutions.set(key, value);
    }

    virtual void setCodeBase(lString32 filePath);
    /// returns flags
    virtual lUInt32 getFlags() {
        return parent->getFlags();
    }
    /// sets flags
    virtual void setFlags(lUInt32 flags) {
        parent->setFlags(flags);
    }
    // overrides
    /// called when encoding directive found in document
    virtual void OnEncoding(const lChar32* name, const lChar32* table) {
        parent->OnEncoding(name, table);
    }
    /// called on parsing start
    virtual void OnStart(LVFileFormatParser*) {
        insideTag = false;
        headStyleText.clear();
        headStyleState = 0;
        insideHtmlTag = false;
        htmlDir.clear();
        htmlLang.clear();
    }
    /// called on parsing end
    virtual void OnStop() {
        if (insideTag) {
            insideTag = false;
            if (!baseTagReplacement.empty()) {
                parent->OnTagClose(U"", baseTagReplacement.c_str());
            }
            baseElement = NULL;
            return;
        }
        insideTag = false;
    }
    /// called on opening tag
    virtual ldomNode* OnTagOpen(const lChar32* nsname, const lChar32* tagname);
    /// called after > of opening tag (when entering tag body)
    virtual void OnTagBody();
    /// called on closing tag
    virtual void OnTagClose(const lChar32* nsname, const lChar32* tagname, bool self_closing_tag = false);
    /// called on attribute
    virtual void OnAttribute(const lChar32* nsname, const lChar32* attrname, const lChar32* attrvalue);
    /// called on text
    virtual void OnText(const lChar32* text, int len, lUInt32 flags) {
        if (headStyleState == 1) {
            headStyleText << UnicodeToUtf8(lString32(text, len));
            return;
        }
        if (insideTag)
            parent->OnText(text, len, flags);
    }
    /// add named BLOB data to document
    virtual bool OnBlob(lString32 name, const lUInt8* data, int size) {
        return parent->OnBlob(name, data, size);
    }
    /// set document property
    virtual void OnDocProperty(const char* name, lString8 value) {
        parent->OnDocProperty(name, value);
    }
    // set non-linear flag
    virtual void setNonLinearFlag(bool nonlinear) {
        m_nonlinear = nonlinear;
    }
    /// constructor
    ldomDocumentFragmentWriter(LVXMLParserCallback* parentWriter, lString32 baseTagName, lString32 baseTagReplacementName, lString32 fragmentFilePath)
            : parent(parentWriter)
            , baseTag(baseTagName)
            , baseTagReplacement(baseTagReplacementName)
            , insideTag(false)
            , styleDetectionState(0)
            , pathSubstitutions(100)
            , baseElement(NULL)
            , lastBaseElement(NULL)
            , headStyleState(0)
            , insideHtmlTag(false) {
        setCodeBase(fragmentFilePath);
    }
    /// destructor
    virtual ~ldomDocumentFragmentWriter() { }
};

#endif // __LDOMDOCUMENTFRAGMENTWRITER_H_INCLUDED__
