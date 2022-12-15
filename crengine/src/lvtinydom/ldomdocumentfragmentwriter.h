/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2008-2012 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2012 Daniel Savard <daniels@xsoli.com>                  *
 *   Copyright (C) 2013 Konstantin Potapov <pkbo@users.sourceforge.net>    *
 *   Copyright (C) 2016 Yifei(Frank) ZHU <fredyifei@gmail.com>             *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
 *   Copyright (C) 2020 Jellby <jellby@yahoo.com>                          *
 *   Copyright (C) 2019,2020 poire-z <poire-z@users.noreply.github.com>    *
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
