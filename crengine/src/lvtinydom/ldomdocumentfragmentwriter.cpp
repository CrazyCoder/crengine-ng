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

#include "ldomdocumentfragmentwriter.h"

#include <lvstreamutils.h>
#include <crlog.h>

/////////////////////////////////////////////////////////////////
/// ldomDocumentFragmentWriter
// Used for EPUB with each individual HTML files in the EPUB,
// drives ldomDocumentWriter to build one single document from them.

lString32 ldomDocumentFragmentWriter::convertId(lString32 id) {
    if (!codeBasePrefix.empty()) {
        return codeBasePrefix + "_" + " " + id; //add a space for later
    }
    return id;
}

lString32 ldomDocumentFragmentWriter::convertHref(lString32 href) {
    if (href.pos("://") >= 0)
        return href; // fully qualified href: no conversion
    if (href.length() > 10 && href[4] == ':' && href.startsWith(lString32("data:image/")))
        return href; // base64 encoded image (<img src="data:image/png;base64,iVBORw0KG...>): no conversion

    //CRLog::trace("convertHref(%s, codeBase=%s, filePathName=%s)", LCSTR(href), LCSTR(codeBase), LCSTR(filePathName));

    if (href[0] == '#') {
        // Link to anchor in the same docFragment
        lString32 replacement = pathSubstitutions.get(filePathName);
        if (replacement.empty())
            return href;
        lString32 p = cs32("#") + replacement + "_" + " " + href.substr(1);
        //CRLog::trace("href %s -> %s", LCSTR(href), LCSTR(p));
        return p;
    }

    // href = LVCombinePaths(codeBase, href);

    // Depending on what's calling us, href may or may not have
    // gone thru DecodeHTMLUrlString() to decode %-encoded bits.
    // We'll need to try again with DecodeHTMLUrlString() if not
    // initially found in "pathSubstitutions" (whose filenames went
    // thru DecodeHTMLUrlString(), and so did 'codeBase').

    // resolve relative links
    lString32 p, id; // path, id
    if (!href.split2(cs32("#"), p, id))
        p = href;
    if (p.empty()) {
        //CRLog::trace("codebase = %s -> href = %s", LCSTR(codeBase), LCSTR(href));
        if (codeBasePrefix.empty())
            return LVCombinePaths(codeBase, href);
        p = codeBasePrefix;
    } else {
        lString32 replacement = pathSubstitutions.get(LVCombinePaths(codeBase, p));
        //CRLog::trace("href %s -> %s", LCSTR(p), LCSTR(replacement));
        if (!replacement.empty())
            p = replacement;
        else {
            // Try again after DecodeHTMLUrlString()
            p = DecodeHTMLUrlString(p);
            replacement = pathSubstitutions.get(LVCombinePaths(codeBase, p));
            if (!replacement.empty())
                p = replacement;
            else
                return LVCombinePaths(codeBase, href);
        }
        //else
        //    p = codeBasePrefix;
        //p = LVCombinePaths( codeBase, p ); // relative to absolute path
    }
    if (!id.empty())
        p = p + "_" + " " + id;

    p = cs32("#") + p;

    //CRLog::debug("converted href=%s to %s", LCSTR(href), LCSTR(p) );

    return p;
}

void ldomDocumentFragmentWriter::setCodeBase(lString32 fileName) {
    filePathName = fileName;
    codeBasePrefix = pathSubstitutions.get(fileName);
    codeBase = LVExtractPath(filePathName);
    if (codeBasePrefix.empty()) {
        CRLog::trace("codeBasePrefix is empty for path %s", LCSTR(fileName));
        codeBasePrefix = pathSubstitutions.get(fileName);
    }
    stylesheetFile.clear();
}

/// called on attribute
void ldomDocumentFragmentWriter::OnAttribute(const lChar32* nsname, const lChar32* attrname, const lChar32* attrvalue) {
    if (insideTag) {
        if (!lStr_cmp(attrname, "href") || !lStr_cmp(attrname, "src")) {
            parent->OnAttribute(nsname, attrname, convertHref(lString32(attrvalue)).c_str());
        } else if (!lStr_cmp(attrname, "id")) {
            parent->OnAttribute(nsname, attrname, convertId(lString32(attrvalue)).c_str());
        } else if (!lStr_cmp(attrname, "name")) {
            //CRLog::trace("name attribute = %s", LCSTR(lString32(attrvalue)));
            parent->OnAttribute(nsname, attrname, convertId(lString32(attrvalue)).c_str());
        } else {
            parent->OnAttribute(nsname, attrname, attrvalue);
        }
    } else {
        if (insideHtmlTag) {
            // Grab attributes from <html dir="rtl" lang="he"> (not included in the DOM)
            // to reinject them in <DocFragment>
            if (!lStr_cmp(attrname, "dir"))
                htmlDir = attrvalue;
            else if (!lStr_cmp(attrname, "lang"))
                htmlLang = attrvalue;
        } else if (styleDetectionState) {
            if (!lStr_cmp(attrname, "rel") && lString32(attrvalue).lowercase() == U"stylesheet")
                styleDetectionState |= 2;
            else if (!lStr_cmp(attrname, "type")) {
                if (lString32(attrvalue).lowercase() == U"text/css")
                    styleDetectionState |= 4;
                else
                    styleDetectionState = 0; // text/css type supported only
            } else if (!lStr_cmp(attrname, "href")) {
                styleDetectionState |= 8;
                lString32 href = attrvalue;
                if (stylesheetFile.empty())
                    tmpStylesheetFile = LVCombinePaths(codeBase, href);
                else
                    tmpStylesheetFile = href;
            }
            if (styleDetectionState == 15) {
                if (!stylesheetFile.empty())
                    stylesheetLinks.add(tmpStylesheetFile);
                else
                    stylesheetFile = tmpStylesheetFile;
                styleDetectionState = 0;
                CRLog::trace("CSS file href: %s", LCSTR(stylesheetFile));
            }
        }
    }
}

/// called on opening tag
ldomNode* ldomDocumentFragmentWriter::OnTagOpen(const lChar32* nsname, const lChar32* tagname) {
    if (insideTag) {
        return parent->OnTagOpen(nsname, tagname);
    } else {
        if (!lStr_cmp(tagname, "link"))
            styleDetectionState = 1;
        else if (!lStr_cmp(tagname, "style"))
            headStyleState = 1;
        else if (!lStr_cmp(tagname, "html")) {
            insideHtmlTag = true;
            htmlDir.clear();
            htmlLang.clear();
        }
    }

    // When meeting the <body> of each of an EPUB's embedded HTML files,
    // we will insert into parent (the ldomDocumentWriter that makes out a single
    // document) a <DocFragment> wrapping that <body>. It may end up as:
    //
    //   <DocFragment StyleSheet="OEBPS/Styles/main.css" id="_doc_fragment_2">
    //     <stylesheet href="OEBPS/Text/">
    //       @import url("../Styles/other.css");
    //       @import url(path_to_3rd_css_file)
    //       here is <HEAD><STYLE> content
    //     </stylesheet>
    //     <body>
    //       here is original <BODY> content
    //     </body>
    //   </DocFragment>
    //
    // (Why one css file link in an attribute and others in the tag?
    // I suppose it's because attribute values are hashed and stored only
    // once, so it saves space in the DOM/cache for documents with many
    // fragments and a single CSS link, which is the most usual case.)

    if (!insideTag && baseTag == tagname) { // with EPUBs: baseTag="body"
        insideTag = true;
        if (!baseTagReplacement.empty()) {                                    // with EPUBs: baseTagReplacement="DocFragment"
            baseElement = parent->OnTagOpen(U"", baseTagReplacement.c_str()); // start <DocFragment
            lastBaseElement = baseElement;
            if (!stylesheetFile.empty()) {
                // add attribute <DocFragment StyleSheet="path_to_css_1st_file"
                parent->OnAttribute(U"", U"StyleSheet", stylesheetFile.c_str());
                CRLog::debug("Setting StyleSheet attribute to %s for document fragment", LCSTR(stylesheetFile));
            }
            if (!codeBasePrefix.empty()) // add attribute <DocFragment id="..html_file_name"
                parent->OnAttribute(U"", U"id", codeBasePrefix.c_str());
            if (!htmlDir.empty()) // add attribute <DocFragment dir="rtl" from <html dir="rtl"> tag
                parent->OnAttribute(U"", U"dir", htmlDir.c_str());
            if (!htmlLang.empty()) // add attribute <DocFragment lang="ar" from <html lang="ar"> tag
                parent->OnAttribute(U"", U"lang", htmlLang.c_str());
            if (this->m_nonlinear)
                parent->OnAttribute(U"", U"NonLinear", U"");

            parent->OnTagBody(); // inside <DocFragment>
            if (!headStyleText.empty() || stylesheetLinks.length() > 0) {
                // add stylesheet element as child of <DocFragment>: <stylesheet href="...">
                parent->OnTagOpen(U"", U"stylesheet");
                parent->OnAttribute(U"", U"href", codeBase.c_str());
                lString32 imports;
                for (int i = 0; i < stylesheetLinks.length(); i++) {
                    lString32 import("@import url(\"");
                    import << stylesheetLinks.at(i);
                    import << "\");\n";
                    imports << import;
                }
                stylesheetLinks.clear();
                lString32 styleText = imports + headStyleText.c_str();
                // Add it to <DocFragment><stylesheet>, so it becomes:
                //   <stylesheet href="...">
                //     @import url(path_to_css_2nd_file)
                //     @import url(path_to_css_3rd_file)
                //     here is <HEAD><STYLE> content
                //   </stylesheet>
                parent->OnTagBody();
                parent->OnText(styleText.c_str(), styleText.length(), 0);
                parent->OnTagClose(U"", U"stylesheet");
                // done with <DocFragment><stylesheet>...</stylesheet>
            }
            // Finally, create <body> and go on.
            // The styles we have just set via <stylesheet> element and
            // StyleSheet= attribute will be applied by this OnTagOpen("body")
            // (including those that may apply to body itself), push()'ing
            // the previous stylesheet state, that will be pop()'ed when the
            // ldomElementWriter for DocFragment is left/destroyed (by onBodyExit(),
            // because this OnTagOpen has set to it _stylesheetIsSet).
            parent->OnTagOpen(U"", baseTag.c_str());
            parent->OnTagBody();
            return baseElement;
        }
    }
    return NULL;
}

/// called on closing tag
void ldomDocumentFragmentWriter::OnTagClose(const lChar32* nsname, const lChar32* tagname, bool self_closing_tag) {
    styleDetectionState = headStyleState = 0;
    if (insideTag && baseTag == tagname) {
        insideTag = false;
        if (!baseTagReplacement.empty()) {
            parent->OnTagClose(U"", baseTag.c_str());
            parent->OnTagClose(U"", baseTagReplacement.c_str());
        }
        baseElement = NULL;
        return;
    }
    if (insideTag)
        parent->OnTagClose(nsname, tagname, self_closing_tag);
}

/// called after > of opening tag (when entering tag body) or just before /> closing tag for empty tags
void ldomDocumentFragmentWriter::OnTagBody() {
    if (insideTag) {
        parent->OnTagBody();
    } else if (insideHtmlTag) {
        insideHtmlTag = false;
    }
    if (styleDetectionState == 11) {
        // incomplete <link rel="stylesheet", href="..." />; assuming type="text/css"
        if (!stylesheetFile.empty())
            stylesheetLinks.add(tmpStylesheetFile);
        else
            stylesheetFile = tmpStylesheetFile;
        styleDetectionState = 0;
    } else
        styleDetectionState = 0;
}
