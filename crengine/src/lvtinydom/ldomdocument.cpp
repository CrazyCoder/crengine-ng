/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007-2013,2015 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2014 Huang Xin <chrox.huang@gmail.com>                  *
 *   Copyright (C) 2015,2016 Yifei(Frank) ZHU <fredyifei@gmail.com>        *
 *   Copyright (C) 2013,2020 Konstantin Potapov <pkbo@users.sourceforge.net>
 *   Copyright (C) 2020 NiLuJe <ninuje@gmail.com>                          *
 *   Copyright (C) 2021 ourairquality <info@ourairquality.org>             *
 *   Copyright (C) 2017-2021 poire-z <poire-z@users.noreply.github.com>    *
 *   Copyright (C) 2018,2020-2023 Aleksey Chernov <valexlin@gmail.com>     *
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

#include <ldomdocument.h>

#include <lvrend.h>
#include <lvdocprops.h>
#include <lvimg.h>
#include <fb2def.h>
#include <lvstreamutils.h>
#include <lvcacheloadingcallback.h>
#include <lvdocviewcallback.h>
#include <lvdocviewprops.h>
#include <crlog.h>

#include "lxmlattribute.h"
#include "lvimportstylesheetparser.h"
#include "renderrectaccessor.h"
#include "ldomnodeidpredicate.h"
#include "cachefile.h"
#include "ldomdatastoragemanager.h"
#include "lvtinydom_private.h"
#include "ldomblobcache.h"
#include "../lvstream/lvbase64stream.h"
#include "../textlang.h"

#ifndef STREAM_AUTO_SYNC_SIZE
#define STREAM_AUTO_SYNC_SIZE 300000
#endif //STREAM_AUTO_SYNC_SIZE

/// XPath step kind
typedef enum
{
    xpath_step_error = 0, // error
    xpath_step_element,   // element of type 'name' with 'index'        /elemname[N]/
    xpath_step_text,      // text node with 'index'                     /text()[N]/
    xpath_step_nodeindex, // node index                                 /N/
    xpath_step_point      // point index                                .N
} xpath_step_t;

static xpath_step_t ParseXPathStep(const lChar32*& path, lString32& name, int& index) {
    int pos = 0;
    const lChar32* s = path;
    //int len = path.GetLength();
    name.clear();
    index = -1;
    int flgPrefix = 0;
    if (s && s[pos]) {
        lChar32 ch = s[pos];
        // prefix: none, '/' or '.'
        if (ch == '/') {
            flgPrefix = 1;
            ch = s[++pos];
        } else if (ch == '.') {
            flgPrefix = 2;
            ch = s[++pos];
        }
        int nstart = pos;
        if (ch >= '0' && ch <= '9') {
            // node or point index
            pos++;
            while (s[pos] >= '0' && s[pos] <= '9')
                pos++;
            if (s[pos] && s[pos] != '/' && s[pos] != '.')
                return xpath_step_error;
            lString32 sindex(path + nstart, pos - nstart);
            index = sindex.atoi();
            if (index < ((flgPrefix == 2) ? 0 : 1))
                return xpath_step_error;
            path += pos;
            return (flgPrefix == 2) ? xpath_step_point : xpath_step_nodeindex;
        }
        while (s[pos] && s[pos] != '[' && s[pos] != '/' && s[pos] != '.')
            pos++;
        if (pos == nstart)
            return xpath_step_error;
        name = lString32(path + nstart, pos - nstart);
        if (s[pos] == '[') {
            // index
            pos++;
            int istart = pos;
            while (s[pos] && s[pos] != ']' && s[pos] != '/' && s[pos] != '.')
                pos++;
            if (!s[pos] || pos == istart)
                return xpath_step_error;

            lString32 sindex(path + istart, pos - istart);
            index = sindex.atoi();
            pos++;
        }
        if (!s[pos] || s[pos] == '/' || s[pos] == '.') {
            path += pos;
            return (name == "text()") ? xpath_step_text : xpath_step_element; // OK!
        }
        return xpath_step_error; // error
    }
    return xpath_step_error;
}

static bool isBoxingNode(ldomNode* node) {
    // In the context this is used (xpointers), handle pseudoElems (that don't
    // box anything) just as boxing nodes: ignoring them in XPointers.
    return node->isBoxingNode(true);
}

static bool isTextNode(ldomNode* node) {
    return (node && node->isText());
}

static bool notNull(ldomNode* node) {
    return (NULL != node);
}

template <typename T>
static ldomNode* getNodeByIndex(ldomNode* parent, int index, T predicat, int& count) {
    ldomNode* foundNode = NULL;

    for (int i = 0; i < (int)parent->getChildCount(); i++) {
        ldomNode* p = parent->getChildNode(i);
        if (isBoxingNode(p)) {
            foundNode = getNodeByIndex(p, index, predicat, count);
            if (foundNode)
                return foundNode;
        } else if (predicat(p)) {
            count++;
            if (index == -1 || count == index) {
                if (!foundNode)
                    foundNode = p;
                return foundNode;
            }
        }
    }
    return NULL;
}

static void dumpRendMethods(ldomNode* node, lString32 prefix) {
    lString32 name = prefix;
    if (node->isText())
        name << node->getText();
    else
        name << "<" << node->getNodeName() << ">   " << fmt::decimal(node->getRendMethod());
    CRLog::trace("%s ", LCSTR(name));
    for (int i = 0; i < node->getChildCount(); i++) {
        dumpRendMethods(node->getChildNode(i), prefix + "   ");
    }
}

ldomDocument::ldomDocument()
        : lxmlDocBase(DEF_DOC_DATA_BUFFER_SIZE)
        , m_toc(this)
        , m_pagemap(this)
        , _last_docflags(0)
        , _page_height(0)
        , _page_width(0)
        , _parsing(false)
        , _rendered(false)
        , _just_rendered_from_cache(false)
        , _toc_from_cache_valid(false)
        , _warnings_seen_bitmap(0)
        , _doc_rendering_hash(0)
        , _open_from_cache(false)
        , lists(100) {
    _docIndex = ldomNode::registerDocument(this);
    ldomNode* node = allocTinyElement(NULL, 0, 0);
    node->persist();

    //new ldomElement( this, NULL, 0, 0, 0 );
    //assert( _instanceMapCount==2 );
}

/// creates empty document which is ready to be copy target of doc partial contents
ldomDocument::ldomDocument(ldomDocument& doc)
        : lxmlDocBase(doc)
        , m_toc(this)
        , m_pagemap(this)
        , _def_font(doc._def_font) // default font
        , _def_style(doc._def_style)
        , _last_docflags(doc._last_docflags)
        , _page_height(doc._page_height)
        , _page_width(doc._page_width)
        , _container(doc._container)
        , lists(100) {
    _docIndex = ldomNode::registerDocument(this);
}

static void writeNode(LVStream* stream, ldomNode* node, bool treeLayout) {
    int level = 0;
    if (treeLayout) {
        level = node->getNodeLevel();
        for (int i = 0; i < level; i++)
            *stream << "  ";
    }
    if (node->isText()) {
        lString8 txt = node->getText8();
        *stream << txt;
        if (treeLayout)
            *stream << "\n";
    } else if (node->isElement()) {
        lString8 elemName = UnicodeToUtf8(node->getNodeName());
        lString8 elemNsName = UnicodeToUtf8(node->getNodeNsName());
        if (!elemNsName.empty())
            elemName = elemNsName + ":" + elemName;
        if (!elemName.empty())
            *stream << "<" << elemName;
        int i;
        for (i = 0; i < (int)node->getAttrCount(); i++) {
            const lxmlAttribute* attr = node->getAttribute(i);
            if (attr) {
                lString8 attrName(UnicodeToUtf8(node->getDocument()->getAttrName(attr->id)));
                lString8 nsName(UnicodeToUtf8(node->getDocument()->getNsName(attr->nsid)));
                lString8 attrValue(UnicodeToUtf8(node->getDocument()->getAttrValue(attr->index)));
                *stream << " ";
                if (nsName.length() > 0)
                    *stream << nsName << ":";
                *stream << attrName << "=\"" << attrValue << "\"";
            }
        }

#if 0
            if (!elemName.empty())
            {
                ldomNode * elem = node;
                lvdomElementFormatRec * fmt = elem->getRenderData();
                css_style_ref_t style = elem->getStyle();
                if ( fmt ) {
                    lvRect rect;
                    elem->getAbsRect( rect );
                    *stream << U" fmt=\"";
                    *stream << U"rm:" << lString32::itoa( (int)elem->getRendMethod() ) << U" ";
                    if ( style.isNull() )
                        *stream << U"style: NULL ";
                    else {
                        *stream << U"disp:" << lString32::itoa( (int)style->display ) << U" ";
                    }
                    *stream << U"y:" << lString32::itoa( (int)fmt->getY() ) << U" ";
                    *stream << U"h:" << lString32::itoa( (int)fmt->getHeight() ) << U" ";
                    *stream << U"ay:" << lString32::itoa( (int)rect.top ) << U" ";
                    *stream << U"ah:" << lString32::itoa( (int)rect.height() ) << U" ";
                    *stream << U"\"";
                }
            }
#endif

        if (node->getChildCount() == 0) {
            if (!elemName.empty()) {
                if (elemName[0] == '?')
                    *stream << "?>";
                else
                    *stream << "/>";
            }
            if (treeLayout)
                *stream << "\n";
        } else {
            if (!elemName.empty())
                *stream << ">";
            if (treeLayout)
                *stream << "\n";
            for (i = 0; i < (int)node->getChildCount(); i++) {
                writeNode(stream, node->getChildNode(i), treeLayout);
            }
            if (treeLayout) {
                for (int i = 0; i < level; i++)
                    *stream << "  ";
            }
            if (!elemName.empty())
                *stream << "</" << elemName << ">";
            if (treeLayout)
                *stream << "\n";
        }
    }
}

bool ldomDocument::saveToStream(LVStreamRef stream, const char*, bool treeLayout) {
    //CRLog::trace("ldomDocument::saveToStream()");
    if (!stream || !getRootNode()->getChildCount())
        return false;

    *stream.get() << UnicodeToLocal(cs32(U"\xFEFF"));
    writeNode(stream.get(), getRootNode(), treeLayout);
    return true;
}

void ldomDocument::printWarning(const char* msg, int warning_id) {
    // Provide a warning_id from 1 to 32 to have this warning emited only once
    // Provide 0 to have it printed it every time
    lUInt32 warning_bit = 0;
    if (warning_id > 0 && warning_id <= 32) {
        warning_bit = 1 << (warning_id - 1);
    }
    if (!(warning_bit & _warnings_seen_bitmap)) {
        CRLog::warn("CRE WARNING: %s\n", msg);
        _warnings_seen_bitmap |= warning_bit;
    }
}

ldomDocument::~ldomDocument() {
    updateMap(); // NOLINT: Call to virtual function during destruction
    fontMan->UnregisterDocumentFonts(_docIndex);
    ldomNode::unregisterDocument(this);
}

/// renders (formats) document in memory
bool ldomDocument::setRenderProps(int width, int dy, bool /*showCover*/, int /*y0*/, font_ref_t def_font, int def_interline_space, CRPropRef props) {
    // Note: def_interline_space is no more used here
    bool changed = false;
    // Don't clear this cache of LFormattedText if
    // render props don't change.
    //   _renderedBlockCache.clear();
    changed = _imgScalingOptions.update(props, def_font->getSize()) || changed;
    css_style_ref_t s(new css_style_rec_t);
    s->display = css_d_block;
    s->white_space = css_ws_normal;
    s->text_align = css_ta_start;
    s->text_align_last = css_ta_auto;
    s->text_decoration = css_td_none;
    s->text_transform = css_tt_none;
    s->hyphenate = css_hyph_auto;
    s->color.type = css_val_unspecified;
    s->color.value = props->getColorDef(PROP_FONT_COLOR, 0);
    s->background_color.type = css_val_unspecified;
    s->background_color.value = props->getColorDef(PROP_BACKGROUND_COLOR, 0);
    //_def_style->background_color.type = color;
    //_def_style->background_color.value = 0xFFFFFF;
    s->page_break_before = css_pb_auto;
    s->page_break_after = css_pb_auto;
    s->page_break_inside = css_pb_auto;
    s->list_style_type = css_lst_disc;
    s->list_style_position = css_lsp_outside;
    s->vertical_align.type = css_val_unspecified;
    s->vertical_align.value = css_va_baseline;
    s->font_family = def_font->getFontFamily();
    s->font_size.type = css_val_screen_px; // we use this type, as we got the real font size from FontManager
    s->font_size.value = def_font->getSize();
    s->font_name = def_font->getTypeFace();
    s->font_weight = css_fw_400;
    s->font_style = css_fs_normal;
    s->font_features.type = css_val_unspecified;
    s->font_features.value = 0;
    s->text_indent.type = css_val_px;
    s->text_indent.value = 0;
    // s->line_height.type = css_val_percent;
    // s->line_height.value = def_interline_space << 8;
    s->line_height.type = css_val_unspecified;
    s->line_height.value = css_generic_normal; // line-height: normal
    s->orphans = css_orphans_widows_1;         // default to allow orphans and widows
    s->widows = css_orphans_widows_1;
    s->float_ = css_f_none;
    s->clear = css_c_none;
    s->direction = css_dir_inherit;
    s->visibility = css_v_visible;
    s->line_break = css_lb_auto;
    s->word_break = css_wb_normal;
    s->cr_hint.type = css_val_unspecified;
    s->cr_hint.value = CSS_CR_HINT_NONE;
    //lUInt32 defStyleHash = (((_stylesheet.getHash() * 31) + calcHash(_def_style))*31 + calcHash(_def_font));
    //defStyleHash = defStyleHash * 31 + getDocFlags();
    if (_last_docflags != getDocFlags()) {
        CRLog::trace("ldomDocument::setRenderProps() - doc flags changed");
        _last_docflags = getDocFlags();
        changed = true;
    }
    if (calcHash(_def_style) != calcHash(s)) {
        CRLog::trace("ldomDocument::setRenderProps() - style is changed");
        _def_style = s;
        changed = true;
    }
    if (calcHash(_def_font) != calcHash(def_font)) {
        CRLog::trace("ldomDocument::setRenderProps() - font is changed");
        _def_font = def_font;
        changed = true;
    }
    if (_page_height != dy && dy > 0) {
        CRLog::trace("ldomDocument::setRenderProps() - page height is changed: %d != %d", _page_height, dy);
        _page_height = dy;
        changed = true;
    }
    if (_page_width != width && width > 0) {
        CRLog::trace("ldomDocument::setRenderProps() - page width is changed");
        _page_width = width;
        changed = true;
    }
    //    {
    //        lUInt32 styleHash = calcStyleHash();
    //        styleHash = styleHash * 31 + calcGlobalSettingsHash();
    //        CRLog::debug("Style hash before set root style: %x", styleHash);
    //    }
    //    getRootNode()->setFont( _def_font );
    //    getRootNode()->setStyle( _def_style );
    //    {
    //        lUInt32 styleHash = calcStyleHash();
    //        styleHash = styleHash * 31 + calcGlobalSettingsHash();
    //        CRLog::debug("Style hash after set root style: %x", styleHash);
    //    }
    return changed;
}

// This is mostly only useful for FB2 stylesheet, as we no more set
// anything in _docStylesheetFileName
void ldomDocument::applyDocumentStyleSheet() {
    if (!getDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES)) {
        CRLog::trace("applyDocumentStyleSheet() : DOC_FLAG_ENABLE_INTERNAL_STYLES is disabled");
        return;
    }
    if (!_docStylesheetFileName.empty()) {
        if (getContainer().isNull())
            return;
        if (parseStyleSheet(_docStylesheetFileName)) {
            CRLog::debug("applyDocumentStyleSheet() : Using document stylesheet from link/stylesheet from %s",
                         LCSTR(_docStylesheetFileName));
        }
    } else {
        ldomXPointer ss = createXPointer(cs32("/FictionBook/stylesheet"));
        if (!ss.isNull()) {
            lString32 css = ss.getText('\n');
            if (!css.empty()) {
                CRLog::debug("applyDocumentStyleSheet() : Using internal FB2 document stylesheet:\n%s", LCSTR(css));
                _stylesheet.parse(LCSTR(css));
            } else {
                CRLog::trace("applyDocumentStyleSheet() : stylesheet under /FictionBook/stylesheet is empty");
            }
        } else {
            CRLog::trace("applyDocumentStyleSheet() : No internal FB2 stylesheet found under /FictionBook/stylesheet");
        }
    }
}

bool ldomDocument::parseStyleSheet(lString32 codeBase, lString32 css) {
    LVImportStylesheetParser parser(this);
    return parser.Parse(codeBase, css);
}

bool ldomDocument::parseStyleSheet(lString32 cssFile) {
    LVImportStylesheetParser parser(this);
    return parser.Parse(cssFile);
}

bool ldomDocument::render(LVRendPageList* pages, LVDocViewCallback* callback, int width, int dy,
                          bool showCover, int y0, font_ref_t def_font, int def_interline_space,
                          CRPropRef props, int usable_left_overflow, int usable_right_overflow) {
    CRLog::info("Render is called for width %d, pageHeight=%d, fontFace=%s, docFlags=%d", width, dy, def_font->getTypeFace().c_str(), getDocFlags());
    CRLog::trace("initializing default style...");
    //persist();
    //    {
    //        lUInt32 styleHash = calcStyleHash();
    //        styleHash = styleHash * 31 + calcGlobalSettingsHash();
    //        CRLog::debug("Style hash before setRenderProps: %x", styleHash);
    //    } //bool propsChanged =
    setRenderProps(width, dy, showCover, y0, def_font, def_interline_space, props);

    // update styles
    //    if ( getRootNode()->getStyle().isNull() || getRootNode()->getFont().isNull()
    //        || _docFlags != _hdr.render_docflags
    //        || width!=_hdr.render_dx || dy!=_hdr.render_dy || defStyleHash!=_hdr.stylesheet_hash ) {
    //        CRLog::trace("init format data...");
    //        getRootNode()->recurseElements( initFormatData );
    //    } else {
    //        CRLog::trace("reusing existing format data...");
    //    }

    bool was_just_rendered_from_cache = _just_rendered_from_cache; // cleared by checkRenderContext()
    if (!checkRenderContext()) {
        if (_nodeDisplayStyleHashInitial == NODE_DISPLAY_STYLE_HASH_UNINITIALIZED) { // happen when just loaded
            // For knowing/debugging cases when node styles set up during loading
            // is invalid (should happen now only when EPUB has embedded fonts
            // or some pseudoclass like :last-child has been met).
            CRLog::warn("CRE: styles re-init needed after load, re-rendering\n");
        }
        CRLog::info("rendering context is changed - full render required...");
        // Clear LFormattedTextRef cache
        _renderedBlockCache.clear();
        CRLog::trace("init format data...");
        //CRLog::trace("validate 1...");
        //validateDocument();
        CRLog::trace("Dropping existing styles...");
        //CRLog::debug( "root style before drop style %d", getNodeStyleIndex(getRootNode()->getDataIndex()));
        dropStyles();
        //CRLog::debug( "root style after drop style %d", getNodeStyleIndex(getRootNode()->getDataIndex()));

        // After having dropped styles, which should have dropped most references
        // to fonts instances, we want to drop these fonts instances.
        // Mostly because some fallback fonts, possibly synthetized (fake bold and
        // italic) may have been instantiated in the late phase of text rendering.
        // We don't want such instances to be used for styles as it could cause some
        // cache check issues (perpetual "style hash mismatch", as these synthetised
        // fonts would not yet be there when loading from cache).
        // We need 2 gc() for a complete cleanup. The performance impact of
        // reinstantiating the fonts is minimal.
        gc(); // drop font instances that were only referenced by dropped styles
        gc(); // drop fallback font instances that were only referenced by dropped fonts

        //ldomNode * root = getRootNode();
        //css_style_ref_t roots = root->getStyle();
        //CRLog::trace("validate 2...");
        //validateDocument();

        // Reset counters (quotes nesting levels...)
        TextLangMan::resetCounters();

        CRLog::trace("Save stylesheet...");
        _stylesheet.push();
        CRLog::trace("Init node styles...");
        applyDocumentStyleSheet();
        getRootNode()->initNodeStyleRecursive(callback);
        CRLog::trace("Restoring stylesheet...");
        _stylesheet.pop();

        CRLog::trace("init render method...");
        getRootNode()->initNodeRendMethodRecursive();

        //        getRootNode()->setFont( _def_font );
        //        getRootNode()->setStyle( _def_style );
        updateRenderContext();

        // DEBUG dump of render methods
        //dumpRendMethods( getRootNode(), cs32(" - ") );
        //        lUInt32 styleHash = calcStyleHash();
        //        styleHash = styleHash * 31 + calcGlobalSettingsHash();
        //        CRLog::debug("Style hash: %x", styleHash);

        _rendered = false;
    }
    if (!_rendered) {
        if (callback) {
            callback->OnFormatStart();
        }
        _renderedBlockCache.reduceSize(1); // Reduce size to save some checking and trashing time
        setCacheFileStale(true);           // new rendering: cache file will be updated
        _toc_from_cache_valid = false;
        // force recalculation of page numbers (even if not computed in this
        // session, they will be when loaded from cache next session)
        m_toc.invalidatePageNumbers();
        m_pagemap.invalidatePageInfo();
        pages->clear();
        if (showCover)
            pages->add(new LVRendPageInfo(_page_height));
        LVRendPageContext context(pages, _page_height, _def_font->getSize());
        int numFinalBlocks = calcFinalBlocks();
        CRLog::info("Final block count: %d", numFinalBlocks);
        context.setCallback(callback, numFinalBlocks);
        //updateStyles();
        CRLog::trace("rendering...");
        renderBlockElement(context, getRootNode(), 0, y0, width, usable_left_overflow, usable_right_overflow);
        _rendered = true;
#if 0 //def _DEBUG
        LVStreamRef ostream = LVOpenFileStream( "test_save_after_init_rend_method.xml", LVOM_WRITE );
        saveToStream( ostream, "utf-16" );
#endif
        gc();
        CRLog::trace("finalizing... fonts.length=%d", _fonts.length());
        context.Finalize();
        updateRenderContext();
        _pagesData.reset();
        pages->serialize(_pagesData);
        _renderedBlockCache.restoreSize(); // Restore original cache size

        if (_nodeDisplayStyleHashInitial == NODE_DISPLAY_STYLE_HASH_UNINITIALIZED) {
            // If _nodeDisplayStyleHashInitial has not been initialized from its
            // former value from the cache file, we use the one computed (just
            // above in updateRenderContext()) after the first full rendering
            // (which has applied styles and created the needed autoBoxing nodes
            // in the DOM). It is coherent with the DOM built up to now.
            _nodeDisplayStyleHashInitial = _nodeDisplayStyleHash;
            CRLog::info("Initializing _nodeDisplayStyleHashInitial after first rendering: %x", _nodeDisplayStyleHashInitial);
            // We also save it directly into DocFileHeader _hdr (normally,
            // updateRenderContext() does this, but doing it here avoids
            // a call and an expensive CalcStyleHash)
            _hdr.node_displaystyle_hash = _nodeDisplayStyleHashInitial;
        }

        if (callback) {
            callback->OnFormatEnd();
            callback->OnDocumentReady();
        }

        //saveChanges();

        //persist();
        dumpStatistics();

        return true; // full (re-)rendering done

    } else {
        CRLog::info("rendering context is not changed - no render!");
        if (_pagesData.pos()) {
            _pagesData.setPos(0);
            pages->deserialize(_pagesData);
        }
        CRLog::info("%d rendered pages found", pages->length());

        if (was_just_rendered_from_cache && callback)
            callback->OnDocumentReady();

        return false; // no (re-)rendering needed
    }
}

/// create xpointer from pointer string
ldomXPointer ldomDocument::createXPointer(const lString32& xPointerStr) {
    if (xPointerStr[0] == '#') {
        lString32 id = xPointerStr.substr(1);
        lUInt32 idid = getAttrValueIndex(id.c_str());
        lInt32 nodeIndex;
        if (_idNodeMap.get(idid, nodeIndex)) {
            ldomNode* node = getTinyNode(nodeIndex);
            if (node && node->isElement()) {
                return ldomXPointer(node, -1);
            }
        }
        return ldomXPointer();
    }
    return createXPointer(getRootNode(), xPointerStr);
}

ldomXPointer ldomDocument::createXPointer(ldomNode* baseNode, const lString32& xPointerStr) {
    if (_DOMVersionRequested >= DOM_VERSION_WITH_NORMALIZED_XPOINTERS)
        return createXPointerV2(baseNode, xPointerStr);
    return createXPointerV1(baseNode, xPointerStr);
}

lString32 ldomDocument::textFromXPath(const lString32& xPointerStr) {
    ldomNode* node = nodeFromXPath(xPointerStr);
    if (!node)
        return lString32::empty_str;
    return node->getText();
}

/// create xpointer from doc point
ldomXPointer ldomDocument::createXPointer(lvPoint pt, int direction, bool strictBounds, ldomNode* fromNode) {
    //
    lvPoint orig_pt = lvPoint(pt);
    ldomXPointer ptr;
    if (!getRootNode())
        return ptr;
    ldomNode* startNode;
    if (fromNode) {
        // Start looking from the fromNode provided - only used when we are
        // looking inside a floatBox or an inlineBox below and we have this
        // recursive call to createXPointer().
        // Even with a provided fromNode, pt must be provided in full absolute
        // coordinates. But we need to give to startNode->elementFromPoint()
        // a pt with coordinates relative to fromNode.
        // And because elementFromPoint() uses the fmt x/y offsets of the
        // start node (relative to the containing final block), we would
        // need to have pt relative to that containing final block - and so,
        // we'd need to lookup the final node from here (or have it provided
        // as an additional parameter if it's known by caller).
        // But because we're called only for floatBox and inlineBox, which
        // have only a single child, we can use the trick of calling
        // ->elementFromPoint() on that first child, while still getting
        // pt relative to fromNode itself:
        startNode = fromNode->getChildNode(0);
        lvRect rc;
        fromNode->getAbsRect(rc, true);
        pt.x -= rc.left;
        pt.y -= rc.top;
    } else {
        startNode = getRootNode();
    }
    ldomNode* finalNode = startNode->elementFromPoint(pt, direction);
    if (fromNode)
        pt = orig_pt; // restore orig pt
    if (!finalNode) {
        // printf("no finalNode found from %s\n", UnicodeToLocal(ldomXPointer(fromNode, 0).toString()).c_str());
        // No node found, return start or end of document if pt overflows it, otherwise NULL
        if (pt.y >= getFullHeight()) {
            ldomNode* node = getRootNode()->getLastTextChild();
            return ldomXPointer(node, node ? node->getText().length() : 0);
        } else if (pt.y <= 0) {
            ldomNode* node = getRootNode()->getFirstTextChild();
            return ldomXPointer(node, 0);
        }
        CRLog::trace("not final node");
        return ptr;
    }
    // printf("finalNode %s\n", UnicodeToLocal(ldomXPointer(finalNode, 0).toString()).c_str());

    lvdom_element_render_method rm = finalNode->getRendMethod();
    if (rm != erm_final) {
        // Not final, return XPointer to first or last child
        lvRect rc;
        finalNode->getAbsRect(rc);
        if (pt.y < (rc.bottom + rc.top) / 2)
            return ldomXPointer(finalNode, 0);
        else
            return ldomXPointer(finalNode, finalNode->getChildCount());
    }

    // Final node found
    // Adjust pt in coordinates of the FormattedText
    RenderRectAccessor fmt(finalNode);
    lvRect rc;
    // When in enhanced rendering mode, we can get the FormattedText coordinates
    // and its width (inner_width) directly
    finalNode->getAbsRect(rc, true); // inner=true
    pt.x -= rc.left;
    pt.y -= rc.top;
    int inner_width;
    if (RENDER_RECT_HAS_FLAG(fmt, INNER_FIELDS_SET)) {
        inner_width = fmt.getInnerWidth();
    } else {
        // In legacy mode, we just got the erm_final coordinates, and we must
        // compute and remove left/top border and padding (using rc.width() as
        // the base for % is wrong here, and so is rc.height() for padding top)
        int padding_left = measureBorder(finalNode, 3) + lengthToPx(finalNode, finalNode->getStyle()->padding[0], rc.width());
        int padding_right = measureBorder(finalNode, 1) + lengthToPx(finalNode, finalNode->getStyle()->padding[1], rc.width());
        int padding_top = measureBorder(finalNode, 0) + lengthToPx(finalNode, finalNode->getStyle()->padding[2], rc.height());
        pt.x -= padding_left;
        pt.y -= padding_top;
        // As well as the inner width
        inner_width = fmt.getWidth() - padding_left - padding_right;
    }

    // Get the formatted text, so we can look for 'pt' line by line, word by word,
    // (and embedded float by embedded float if there are some).
    LFormattedTextRef txtform;
    {
        // This will possibly return it from CVRendBlockCache
        finalNode->renderFinalBlock(txtform, &fmt, inner_width);
    }

    // First, look if pt happens to be in some float
    // (this may not work with floats with negative margins)
    int fcount = txtform->GetFloatCount();
    for (int f = 0; f < fcount; f++) {
        const embedded_float_t* flt = txtform->GetFloatInfo(f);
        // Ignore fake floats (no srctext) made from outer floats footprint
        if (flt->srctext == NULL)
            continue;
        if (pt.x >= flt->x && pt.x < flt->x + flt->width && pt.y >= flt->y && pt.y < flt->y + (lInt32)flt->height) {
            // pt is inside this float.
            ldomNode* node = (ldomNode*)flt->srctext->object; // floatBox node
            ldomXPointer inside_ptr = createXPointer(orig_pt, direction, strictBounds, node);
            if (!inside_ptr.isNull()) {
                return inside_ptr;
            }
            // Otherwise, return xpointer to the floatNode itself
            return ldomXPointer(node, 0);
            // (Or should we let just go on looking only at the text in the original final node?)
        }
        // If no containing float, go on looking at the text of the original final node
    }

    // Look at words in the rendered final node (whether it's the original
    // main final node, or the one found in a float)
    int lcount = txtform->GetLineCount();
    for (int l = 0; l < lcount; l++) {
        const formatted_line_t* frmline = txtform->GetLineInfo(l);
        if (pt.y >= (int)(frmline->y + frmline->height) && l < lcount - 1)
            continue;
        // CRLog::debug("  point (%d, %d) line found [%d]: (%d..%d)",
        //      pt.x, pt.y, l, frmline->y, frmline->y+frmline->height);
        bool line_is_bidi = frmline->flags & LTEXT_LINE_IS_BIDI;
        int wc = (int)frmline->word_count;

        if (direction >= PT_DIR_SCAN_FORWARD_LOGICAL_FIRST || direction <= PT_DIR_SCAN_BACKWARD_LOGICAL_FIRST) {
            // Only used by LVDocView::getBookmark(), LVDocView::getPageDocumentRange()
            // and ldomDocument::findText(), to not miss any content or text from
            // the page.
            // The SCAN_ part has been done done: a line has been found, and we want
            // to find node/chars from it in the logical (HTML) order, and not in the
            // visual order (that PT_DIR_SCAN_FORWARD/PT_DIR_SCAN_BACKWARD do), which
            // might not be the same in bidi lines:
            bool find_first = direction == PT_DIR_SCAN_FORWARD_LOGICAL_FIRST ||
                              direction == PT_DIR_SCAN_BACKWARD_LOGICAL_FIRST;
            // so, false when PT_DIR_SCAN_FORWARD_LOGICAL_LAST
            //             or PT_DIR_SCAN_BACKWARD_LOGICAL_LAST

            const formatted_word_t* word = NULL;
            for (int w = 0; w < wc; w++) {
                const formatted_word_t* tmpword = &frmline->words[w];
                const src_text_fragment_t* src = txtform->GetSrcInfo(tmpword->src_text_index);
                ldomNode* node = (ldomNode*)src->object;
                if (!node) // ignore crengine added text (spacing, list item bullets...)
                    continue;
                if (!line_is_bidi) {
                    word = tmpword;
                    if (find_first)
                        break; // found logical first real word
                    // otherwise, go to the end, word will be logical last real word
                } else {
                    if (!word) { // first word seen: first candidate
                        word = tmpword;
                    } else { // compare current word to the current candidate
                        if (find_first && tmpword->src_text_index < word->src_text_index) {
                            word = tmpword;
                        } else if (!find_first && tmpword->src_text_index > word->src_text_index) {
                            word = tmpword;
                        } else if (tmpword->src_text_index == word->src_text_index) {
                            // (Same src_text_fragment_t, same src->u.t.offset, skip in when comparing)
                            if (find_first && tmpword->u.t.start < word->u.t.start) {
                                word = tmpword;
                            } else if (!find_first && tmpword->u.t.start > word->u.t.start) {
                                word = tmpword;
                            }
                        }
                    }
                }
            }
            if (!word) // no word: no xpointer (should not happen?)
                return ptr;
            // Found right word/image
            const src_text_fragment_t* src = txtform->GetSrcInfo(word->src_text_index);
            ldomNode* node = (ldomNode*)src->object;
            if (word->flags & LTEXT_WORD_IS_INLINE_BOX) {
                // pt is inside this inline-block inlineBox node
                ldomXPointer inside_ptr = createXPointer(orig_pt, direction, strictBounds, node);
                if (!inside_ptr.isNull()) {
                    return inside_ptr;
                }
                // Otherwise, return xpointer to the inlineBox itself
                return ldomXPointer(node, 0);
            }
            if (word->flags & LTEXT_WORD_IS_OBJECT) {
                return ldomXPointer(node, 0);
            }
            // It is a word
            if (find_first) // return xpointer to logical start of word
                return ldomXPointer(node, src->u.t.offset + word->u.t.start);
            else // return xpointer to logical end of word
                return ldomXPointer(node, src->u.t.offset + word->u.t.start + word->u.t.len);
        }

        // Found line, searching for word (words are in visual order)
        int x = pt.x - frmline->x;
        // frmline->x is text indentation (+ possibly leading space if text
        // centered or right aligned)
        if (strictBounds) {
            if (x < 0 || x > frmline->width) { // pt is before or after formatted text: nothing there
                return ptr;
            }
        }

        for (int w = 0; w < wc; w++) {
            const formatted_word_t* word = &frmline->words[w];
            if ((!line_is_bidi && x < word->x + word->width) ||
                (line_is_bidi && x >= word->x && x < word->x + word->width) ||
                (w == wc - 1)) {
                const src_text_fragment_t* src = txtform->GetSrcInfo(word->src_text_index);
                // CRLog::debug(" word found [%d]: x=%d..%d, start=%d, len=%d  %08X",
                //      w, word->x, word->x + word->width, word->u.t.start, word->u.t.len, src->object);

                ldomNode* node = (ldomNode*)src->object;
                if (!node) // Ignore crengine added text (spacing, list item bullets...)
                    continue;

                if (word->flags & LTEXT_WORD_IS_INLINE_BOX) {
                    // pt is inside this inline-block inlineBox node
                    ldomXPointer inside_ptr = createXPointer(orig_pt, direction, strictBounds, node);
                    if (!inside_ptr.isNull()) {
                        return inside_ptr;
                    }
                    // Otherwise, return xpointer to the inlineBox itself
                    return ldomXPointer(node, 0);
                }
                if (word->flags & LTEXT_WORD_IS_OBJECT) {
// Object (image)
#if 1
                    // return image object itself
                    return ldomXPointer(node, 0);
#else
                    return ldomXPointer(node->getParentNode(),
                                        node->getNodeIndex() + ((x < word->x + word->width / 2) ? 0 : 1));
#endif
                }

                // Found word, searching for letters
                LVFont* font = (LVFont*)src->u.t.font;
                lUInt16 width[512];
                lUInt8 flg[512];

                lString32 str = node->getText();
                // We need to transform the node text as it had been when
                // rendered (the transform may change chars widths) for the
                // XPointer offset to be correct
                switch (node->getParentNode()->getStyle()->text_transform) {
                    case css_tt_uppercase:
                        str.uppercase();
                        break;
                    case css_tt_lowercase:
                        str.lowercase();
                        break;
                    case css_tt_capitalize:
                        str.capitalize();
                        break;
                    case css_tt_full_width:
                        // str.fullWidthChars(); // disabled for now in lvrend.cpp
                        break;
                    default:
                        break;
                }

                lUInt32 hints = WORD_FLAGS_TO_FNT_FLAGS(word->flags);
                if (str.empty() && word->u.t.len > 0) {
                    // Don't know that the fuck up, but it happens
                    for (int i = 0; i < word->u.t.len; i++)
                        width[i] = 0;
                } else {
                    font->measureText(str.c_str() + word->u.t.start, word->u.t.len, width, flg, word->width + 50, '?',
                                      src->lang_cfg, src->letter_spacing + word->added_letter_spacing, false, hints);
                }

                bool word_is_rtl = word->flags & LTEXT_WORD_DIRECTION_IS_RTL;
                if (word_is_rtl) {
                    for (int i = word->u.t.len - 1; i >= 0; i--) {
                        int xx = (i > 0) ? (width[i - 1] + width[i]) / 2 : width[i] / 2;
                        xx = word->width - xx;
                        if (x < word->x + xx) {
                            return ldomXPointer(node, src->u.t.offset + word->u.t.start + i);
                        }
                    }
                    return ldomXPointer(node, src->u.t.offset + word->u.t.start);
                } else {
                    for (int i = 0; i < word->u.t.len; i++) {
                        int xx = (i > 0) ? (width[i - 1] + width[i]) / 2 : width[i] / 2;
                        if (x < word->x + xx) {
                            return ldomXPointer(node, src->u.t.offset + word->u.t.start + i);
                        }
                    }
                    return ldomXPointer(node, src->u.t.offset + word->u.t.start + word->u.t.len);
                }
            }
        }
    }
    return ptr;
}

/// create XPointer from relative pointer non-normalized string made by toStringV1()
ldomXPointer ldomDocument::createXPointerV1(ldomNode* baseNode, const lString32& xPointerStr) {
    //CRLog::trace( "ldomDocument::createXPointer(%s)", UnicodeToUtf8(xPointerStr).c_str() );
    if (xPointerStr.empty() || !baseNode)
        return ldomXPointer();
    const lChar32* str = xPointerStr.c_str();
    int index = -1;
    ldomNode* currNode = baseNode;
    lString32 name;
    lString8 ptr8 = UnicodeToUtf8(xPointerStr);
    //const char * ptr = ptr8.c_str();
    xpath_step_t step_type;

    while (*str) {
        //CRLog::trace( "    %s", UnicodeToUtf8(lString32(str)).c_str() );
        step_type = ParseXPathStep(str, name, index);
        //CRLog::trace( "        name=%s index=%d", UnicodeToUtf8(lString32(name)).c_str(), index );
        switch (step_type) {
            case xpath_step_error:
                // error
                //CRLog::trace("    xpath_step_error");
                return ldomXPointer();
            case xpath_step_element:
                // element of type 'name' with 'index'        /elemname[N]/
                {
                    lUInt16 id = getElementNameIndex(name.c_str());
                    ldomNode* foundItem = currNode->findChildElement(LXML_NS_ANY, id, index > 0 ? index - 1 : -1);
                    if (foundItem == NULL && currNode->getChildCount() == 1) {
                        // make saved pointers work properly even after moving of some part of path one element deeper
                        foundItem = currNode->getChildNode(0)->findChildElement(LXML_NS_ANY, id, index > 0 ? index - 1 : -1);
                    }
                    //                int foundCount = 0;
                    //                for (unsigned i=0; i<currNode->getChildCount(); i++) {
                    //                    ldomNode * p = currNode->getChildNode(i);
                    //                    //CRLog::trace( "        node[%d] = %d %s", i, p->getNodeId(), LCSTR(p->getNodeName()) );
                    //                    if ( p && p->isElement() && p->getNodeId()==id ) {
                    //                        foundCount++;
                    //                        if ( foundCount==index || index==-1 ) {
                    //                            foundItem = p;
                    //                            break; // DON'T CHECK WHETHER OTHER ELEMENTS EXIST
                    //                        }
                    //                    }
                    //                }
                    //                if ( foundItem==NULL || (index==-1 && foundCount>1) ) {
                    //                    //CRLog::trace("    Element %d is not found. foundCount=%d", id, foundCount);
                    //                    return ldomXPointer(); // node not found
                    //                }
                    if (foundItem == NULL) {
                        //CRLog::trace("    Element %d is not found. foundCount=%d", id, foundCount);
                        return ldomXPointer(); // node not found
                    }
                    // found element node
                    currNode = foundItem;
                }
                break;
            case xpath_step_text:
                // text node with 'index'                     /text()[N]/
                {
                    ldomNode* foundItem = NULL;
                    int foundCount = 0;
                    for (int i = 0; i < currNode->getChildCount(); i++) {
                        ldomNode* p = currNode->getChildNode(i);
                        if (p->isText()) {
                            foundCount++;
                            if (foundCount == index || index == -1) {
                                foundItem = p;
                            }
                        }
                    }
                    if (foundItem == NULL || (index == -1 && foundCount > 1))
                        return ldomXPointer(); // node not found
                    // found text node
                    currNode = foundItem;
                }
                break;
            case xpath_step_nodeindex:
                // node index                                 /N/
                if (index <= 0 || index > (int)currNode->getChildCount())
                    return ldomXPointer(); // node not found: invalid index
                currNode = currNode->getChildNode(index - 1);
                break;
            case xpath_step_point:
                // point index                                .N
                if (*str)
                    return ldomXPointer(); // not at end of string
                if (currNode->isElement()) {
                    // element point
                    if (index < 0 || index > (int)currNode->getChildCount())
                        return ldomXPointer();
                    return ldomXPointer(currNode, index);
                } else {
                    // text point
                    if (index < 0 || index > (int)currNode->getText().length())
                        return ldomXPointer();
                    return ldomXPointer(currNode, index);
                }
                break;
        }
    }
    return ldomXPointer(currNode, -1); // XPath: index==-1
}

/// create XPointer from relative pointer normalized string made by toStringV2()
ldomXPointer ldomDocument::createXPointerV2(ldomNode* baseNode, const lString32& xPointerStr) {
    //CRLog::trace( "ldomDocument::createXPointer(%s)", UnicodeToUtf8(xPointerStr).c_str() );
    if (xPointerStr.empty() || !baseNode)
        return ldomXPointer();
    const lChar32* str = xPointerStr.c_str();
    int index = -1;
    int count;
    ldomNode* currNode = baseNode;
    ldomNode* foundNode;
    lString32 name;
    xpath_step_t step_type;

    while (*str) {
        //CRLog::trace( "    %s", UnicodeToUtf8(lString32(str)).c_str() );
        step_type = ParseXPathStep(str, name, index);
        //CRLog::trace( "        name=%s index=%d", UnicodeToUtf8(lString32(name)).c_str(), index );
        switch (step_type) {
            case xpath_step_error:
                // error
                //CRLog::trace("    xpath_step_error");
                return ldomXPointer();
            case xpath_step_element:
                // element of type 'name' with 'index'        /elemname[N]/
                {
                    ldomNodeIdPredicate predicat(getElementNameIndex(name.c_str()));
                    count = 0;
                    foundNode = getNodeByIndex(currNode, index, predicat, count);
                    if (foundNode == NULL) {
                        //CRLog::trace("    Element %d is not found. foundCount=%d", id, foundCount);
                        return ldomXPointer(); // node not found
                    }
                    // found element node
                    currNode = foundNode;
                    lString32 nm = currNode->getNodeName();
                    //CRLog::trace("%d -> %s", index, LCSTR(nm));
                }
                break;
            case xpath_step_text:
                //
                count = 0;
                foundNode = getNodeByIndex(currNode, index, isTextNode, count);

                if (foundNode == NULL)
                    return ldomXPointer(); // node not found
                // found text node
                currNode = foundNode;
                break;
            case xpath_step_nodeindex:
                // node index                                 /N/
                count = 0;
                foundNode = getNodeByIndex(currNode, index, notNull, count);
                if (foundNode == NULL)
                    return ldomXPointer(); // node not found: invalid index
                currNode = foundNode;
                break;
            case xpath_step_point:
                // point index                                .N
                if (*str)
                    return ldomXPointer(); // not at end of string
                if (currNode->isElement()) {
                    // element point
                    if (index < 0 || index > (int)currNode->getChildCount())
                        return ldomXPointer();
                    return ldomXPointer(currNode, index);
                } else {
                    // text point
                    if (index < 0 || index > (int)currNode->getText().length())
                        return ldomXPointer();
                    return ldomXPointer(currNode, index);
                }
                break;
        }
    }
    return ldomXPointer(currNode, -1); // XPath: index==-1
}

int ldomDocument::getFullHeight() {
    RenderRectAccessor rd(this->getRootNode());
    return rd.getHeight() + rd.getY();
}

bool ldomDocument::findText(lString32 pattern, bool caseInsensitive, bool reverse, int minY, int maxY, LVArray<ldomWord>& words, int maxCount, int maxHeight, int maxHeightCheckStartY) {
    if (minY < 0)
        minY = 0;
    int fh = getFullHeight();
    if (maxY <= 0 || maxY > fh)
        maxY = fh;
    // ldomXPointer start = createXPointer( lvPoint(0, minY), reverse?-1:1 );
    // ldomXPointer end = createXPointer( lvPoint(10000, maxY), reverse?-1:1 );
    // If we're provided with minY or maxY in some empty space (margins, empty
    // elements...), they may not resolve to a XPointer.
    // Find a valid y near each of them that does resolve to a XPointer:
    // We also want to get start/end point to logical-order HTML nodes,
    // which might be different from visual-order in bidi text.
    ldomXPointer start;
    ldomXPointer end;
    for (int y = minY; y >= 0; y--) {
        start = createXPointer(lvPoint(0, y), reverse ? PT_DIR_SCAN_BACKWARD_LOGICAL_FIRST
                                                      : PT_DIR_SCAN_FORWARD_LOGICAL_FIRST);
        if (!start.isNull())
            break;
    }
    if (start.isNull()) {
        // If none found (can happen when minY=0 and blank content at start
        // of document like a <br/>), scan forward from document start
        for (int y = 0; y <= fh; y++) {
            start = createXPointer(lvPoint(0, y), reverse ? PT_DIR_SCAN_BACKWARD_LOGICAL_FIRST
                                                          : PT_DIR_SCAN_FORWARD_LOGICAL_FIRST);
            if (!start.isNull())
                break;
        }
    }
    for (int y = maxY; y <= fh; y++) {
        end = createXPointer(lvPoint(10000, y), reverse ? PT_DIR_SCAN_BACKWARD_LOGICAL_LAST
                                                        : PT_DIR_SCAN_FORWARD_LOGICAL_LAST);
        if (!end.isNull())
            break;
    }
    if (end.isNull()) {
        // If none found (can happen when maxY=fh and blank content at end
        // of book like a <br/>), scan backward from document end
        for (int y = fh; y >= 0; y--) {
            end = createXPointer(lvPoint(10000, y), reverse ? PT_DIR_SCAN_BACKWARD_LOGICAL_LAST
                                                            : PT_DIR_SCAN_FORWARD_LOGICAL_LAST);
            if (!end.isNull())
                break;
        }
    }

    if (start.isNull() || end.isNull())
        return false;
    ldomXRange range(start, end);
    CRLog::debug("ldomDocument::findText() for Y %d..%d, range %d..%d",
                 minY, maxY, start.toPoint().y, end.toPoint().y);
    if (range.getStart().toPoint().y == -1) {
        range.getStart().nextVisibleText();
        CRLog::debug("ldomDocument::findText() updated range %d..%d",
                     range.getStart().toPoint().y, range.getEnd().toPoint().y);
    }
    if (range.getEnd().toPoint().y == -1) {
        range.getEnd().prevVisibleText();
        CRLog::debug("ldomDocument::findText() updated range %d..%d",
                     range.getStart().toPoint().y, range.getEnd().toPoint().y);
    }
    if (range.isNull()) {
        CRLog::debug("No text found: Range is empty");
        return false;
    }
    return range.findText(pattern, caseInsensitive, reverse, words, maxCount, maxHeight, maxHeightCheckStartY);
}

void ldomDocument::clear() {
    clearRendBlockCache();
    _open_from_cache = false;
    _rendered = false;
    _urlImageMap.clear();
    _fontList.clear();
    fontMan->UnregisterDocumentFonts(_docIndex);
    //TODO: implement clear
    //_elemStorage->
}

bool ldomDocument::openFromCache(CacheLoadingCallback* formatCallback, LVDocViewCallback* progressCallback) {
    setCacheFileStale(true);
    if (!openCacheFile()) {
        CRLog::info("Cannot open document from cache. Need to read fully");
        clear();
        return false;
    }
    if (!loadCacheFileContent(formatCallback, progressCallback)) {
        CRLog::info("Error while loading document content from cache file.");
        clear();
        return false;
    }
#if 0
    LVStreamRef s = LVOpenFileStream("/tmp/test.xml", LVOM_WRITE);
    if ( !s.isNull() )
        saveToStream(s, "UTF8");
#endif
    _mapped = true;
    _rendered = true;
    _just_rendered_from_cache = true;
    _toc_from_cache_valid = true;
    _open_from_cache = true;
    // Use cached node_displaystyle_hash as _nodeDisplayStyleHashInitial, as it
    // should be in sync with the DOM stored in the cache
    _nodeDisplayStyleHashInitial = _hdr.node_displaystyle_hash;
    CRLog::info("Initializing _nodeDisplayStyleHashInitial from cache file: %x", _nodeDisplayStyleHashInitial);

    setCacheFileStale(false);
    return true;
}

/// load document cache file content, @see saveChanges()
bool ldomDocument::loadCacheFileContent(CacheLoadingCallback* formatCallback, LVDocViewCallback* progressCallback) {
    CRLog::trace("ldomDocument::loadCacheFileContent()");
    {
        if (progressCallback)
            progressCallback->OnLoadFileProgress(5);
        SerialBuf propsbuf(0, true);
        if (!_cacheFile->read(CBT_PROP_DATA, propsbuf)) {
            CRLog::error("Error while reading props data");
            return false;
        }
        CRPropRef docProps = LVCreatePropsContainer();
        docProps->deserialize(propsbuf);
        if (propsbuf.error()) {
            CRLog::error("Can not decode property table for document");
            return false;
        }
        // Check properties in cache
        lString32 arcName = getProps()->getStringDef(DOC_PROP_ARC_NAME, "");
        lString32 arcPath = getProps()->getStringDef(DOC_PROP_ARC_PATH, "");
        lString32 arcSize = getProps()->getStringDef(DOC_PROP_ARC_SIZE, "");
        lString32 fileName = getProps()->getStringDef(DOC_PROP_FILE_NAME, "");
        lString32 filePath = getProps()->getStringDef(DOC_PROP_FILE_PATH, "");
        lString32 CRC32 = getProps()->getStringDef(DOC_PROP_FILE_CRC32, "");
        lString32 hash = getProps()->getStringDef(DOC_PROP_FILE_HASH, "");
        lString32 size = getProps()->getStringDef(DOC_PROP_FILE_SIZE, "");

        lString32 inCacheArcName = docProps->getStringDef(DOC_PROP_ARC_NAME, "");
        lString32 inCacheArcPath = docProps->getStringDef(DOC_PROP_ARC_PATH, "");
        lString32 inCacheArcSize = docProps->getStringDef(DOC_PROP_ARC_SIZE, "");
        lString32 inCacheFileName = docProps->getStringDef(DOC_PROP_FILE_NAME, "");
        lString32 inCacheFilePath = docProps->getStringDef(DOC_PROP_FILE_PATH, "");
        lString32 inCacheCRC32 = docProps->getStringDef(DOC_PROP_FILE_CRC32, "");
        lString32 inCacheHash = docProps->getStringDef(DOC_PROP_FILE_HASH, "");
        lString32 inCacheSize = docProps->getStringDef(DOC_PROP_FILE_SIZE, "");

        if (CRC32 != inCacheCRC32 || hash != inCacheHash || size != inCacheSize) {
            CRLog::error("Invalid cache file, cannot be used");
            CRLog::error("CRC32: current %s; in cache %s", LCSTR(CRC32), LCSTR(inCacheCRC32));
            CRLog::error("hash: current %s; in cache %s", LCSTR(hash), LCSTR(inCacheHash));
            CRLog::error("size: current %s; in cache %s", LCSTR(size), LCSTR(inCacheSize));
            return false;
        }
        // The file may have been moved so we can't use the old cached properties related to the filepath.
        if (inCacheArcName != arcName)
            docProps->setString(DOC_PROP_ARC_NAME, arcName);
        if (inCacheArcPath != arcPath)
            docProps->setString(DOC_PROP_ARC_PATH, arcPath);
        if (inCacheArcSize != arcSize)
            docProps->setString(DOC_PROP_ARC_SIZE, arcSize);
        if (inCacheFileName != fileName)
            docProps->setString(DOC_PROP_FILE_NAME, fileName);
        if (inCacheFilePath != filePath)
            docProps->setString(DOC_PROP_FILE_PATH, filePath);
        getProps()->set(docProps);
        if (formatCallback) {
            int fmt = getProps()->getIntDef(DOC_PROP_FILE_FORMAT_ID,
                                            doc_format_fb2);
            if (fmt < doc_format_fb2 || fmt > doc_format_max)
                fmt = doc_format_fb2;
            // notify about format detection, to allow setting format-specific CSS
            formatCallback->OnCacheFileFormatDetected((doc_format_t)fmt);
        }

        if (progressCallback)
            progressCallback->OnLoadFileProgress(10);
        CRLog::trace("ldomDocument::loadCacheFileContent() - ID data");
        SerialBuf idbuf(0, true);
        if (!_cacheFile->read(CBT_MAPS_DATA, idbuf)) {
            CRLog::error("Error while reading Id data");
            return false;
        }
        deserializeMaps(idbuf);
        if (idbuf.error()) {
            CRLog::error("Cannot decode ID table for document");
            return false;
        }

        if (progressCallback)
            progressCallback->OnLoadFileProgress(15);
        CRLog::trace("ldomDocument::loadCacheFileContent() - page data");
        SerialBuf pagebuf(0, true);
        if (!_cacheFile->read(CBT_PAGE_DATA, pagebuf)) {
            CRLog::error("Error while reading pages data");
            return false;
        }
        pagebuf.swap(_pagesData);
        _pagesData.setPos(0);
        LVRendPageList pages;
        pages.deserialize(_pagesData);
        if (_pagesData.error()) {
            CRLog::error("Page data deserialization is failed");
            return false;
        }
        CRLog::info("%d pages read from cache file", pages.length());
        //_pagesData.setPos( 0 );

        if (progressCallback)
            progressCallback->OnLoadFileProgress(20);
        CRLog::trace("ldomDocument::loadCacheFileContent() - embedded font data");
        {
            SerialBuf buf(0, true);
            if (!_cacheFile->read(CBT_FONT_DATA, buf)) {
                CRLog::error("Error while reading font data");
                return false;
            }
            if (!_fontList.deserialize(buf)) {
                CRLog::error("Error while parsing font data");
                return false;
            }
            registerEmbeddedFonts();
        }

        if (progressCallback)
            progressCallback->OnLoadFileProgress(25);
        DocFileHeader h = {};
        SerialBuf hdrbuf(0, true);
        if (!_cacheFile->read(CBT_REND_PARAMS, hdrbuf)) {
            CRLog::error("Error while reading header data");
            return false;
        } else if (!h.deserialize(hdrbuf)) {
            CRLog::error("Header data deserialization is failed");
            return false;
        }
        _hdr = h;
        CRLog::info("Loaded render properties: styleHash=%x, stylesheetHash=%x, docflags=%x, width=%x, height=%x, nodeDisplayStyleHash=%x",
                    _hdr.render_style_hash, _hdr.stylesheet_hash, _hdr.render_docflags, _hdr.render_dx, _hdr.render_dy, _hdr.node_displaystyle_hash);
    }

    if (progressCallback)
        progressCallback->OnLoadFileProgress(30);
    CRLog::trace("ldomDocument::loadCacheFileContent() - node data");
    if (!loadNodeData()) {
        CRLog::error("Error while reading node instance data");
        return false;
    }

    if (progressCallback)
        progressCallback->OnLoadFileProgress(40);
    CRLog::trace("ldomDocument::loadCacheFileContent() - element storage");
    if (!_elemStorage->load()) {
        CRLog::error("Error while loading element data");
        return false;
    }
    if (progressCallback)
        progressCallback->OnLoadFileProgress(50);
    CRLog::trace("ldomDocument::loadCacheFileContent() - text storage");
    if (!_textStorage->load()) {
        CRLog::error("Error while loading text data");
        return false;
    }
    if (progressCallback)
        progressCallback->OnLoadFileProgress(60);
    CRLog::trace("ldomDocument::loadCacheFileContent() - rect storage");
    if (!_rectStorage->load()) {
        CRLog::error("Error while loading rect data");
        return false;
    }
    if (progressCallback)
        progressCallback->OnLoadFileProgress(70);
    CRLog::trace("ldomDocument::loadCacheFileContent() - node style storage");
    if (!_styleStorage->load()) {
        CRLog::error("Error while loading node style data");
        return false;
    }

    if (progressCallback)
        progressCallback->OnLoadFileProgress(80);
    CRLog::trace("ldomDocument::loadCacheFileContent() - TOC");
    {
        SerialBuf tocbuf(0, true);
        if (!_cacheFile->read(CBT_TOC_DATA, tocbuf)) {
            CRLog::error("Error while reading TOC data");
            return false;
        } else if (!m_toc.deserialize(this, tocbuf)) {
            CRLog::error("TOC data deserialization is failed");
            return false;
        }
    }
    if (progressCallback)
        progressCallback->OnLoadFileProgress(85);
    CRLog::trace("ldomDocument::loadCacheFileContent() - PageMap");
    {
        SerialBuf pagemapbuf(0, true);
        if (!_cacheFile->read(CBT_PAGEMAP_DATA, pagemapbuf)) {
            CRLog::error("Error while reading PageMap data");
            return false;
        } else if (!m_pagemap.deserialize(this, pagemapbuf)) {
            CRLog::error("PageMap data deserialization is failed");
            return false;
        }
    }

    if (progressCallback)
        progressCallback->OnLoadFileProgress(90);
    if (loadStylesData()) {
        CRLog::trace("ldomDocument::loadCacheFileContent() - using loaded styles");
        updateLoadedStyles(true);
        //        lUInt32 styleHash = calcStyleHash();
        //        styleHash = styleHash * 31 + calcGlobalSettingsHash();
        //        CRLog::debug("Loaded style hash: %x", styleHash);
        //        lUInt32 styleHash = calcStyleHash();
        //        CRLog::info("Loaded style hash = %08x", styleHash);
    } else {
        CRLog::trace("ldomDocument::loadCacheFileContent() - style loading failed: will reinit ");
        updateLoadedStyles(false);
    }

    CRLog::trace("ldomDocument::loadCacheFileContent() - completed successfully");
    if (progressCallback)
        progressCallback->OnLoadFileProgress(95);

    // NOTE: And now might have been a good place to release uncompression resources...
    //       Except not quite...
    //       While the ldomDocument instance exists, the getElem() and getText() сalls access the cache
    //_cacheFile->cleanupUncompressor();

    return true;
}

#define CHECK_EXPIRATION(s)                    \
    if (maxTime.expired()) {                   \
        CRLog::info("timer expired while " s); \
        return CR_TIMEOUT;                     \
    }

/// saves changes to cache file, limited by time interval (can be called again to continue after TIMEOUT)
ContinuousOperationResult ldomDocument::saveChanges(CRTimerUtil& maxTime, LVDocViewCallback* progressCallback) {
    if (!_cacheFile)
        return CR_DONE;

    if (progressCallback)
        progressCallback->OnSaveCacheFileStart();

    if (maxTime.infinite()) {
        _mapSavingStage = 0; // all stages from the beginning
        _cacheFile->setAutoSyncSize(0);
    } else {
        //CRLog::trace("setting autosync");
        _cacheFile->setAutoSyncSize(STREAM_AUTO_SYNC_SIZE);
        //CRLog::trace("setting autosync - done");
    }

    CRLog::trace("ldomDocument::saveChanges(timeout=%d stage=%d)", maxTime.interval(), _mapSavingStage);
    setCacheFileStale(true);

    switch (_mapSavingStage) {
        default:
        case 0:

            if (!maxTime.infinite())
                _cacheFile->flush(false, maxTime);
            CHECK_EXPIRATION("flushing of stream")

            persist(maxTime);
            CHECK_EXPIRATION("persisting of node data")
            if (progressCallback)
                progressCallback->OnSaveCacheFileProgress(0);

            // fall through
        case 1:
            _mapSavingStage = 1;
            CRLog::trace("ldomDocument::saveChanges() - element storage");

            if (!_elemStorage->save(maxTime)) {
                CRLog::error("Error while saving element data");
                return CR_ERROR;
            }
            CHECK_EXPIRATION("saving element storate")
            if (progressCallback)
                progressCallback->OnSaveCacheFileProgress(10);
            // fall through
        case 2:
            _mapSavingStage = 2;
            CRLog::trace("ldomDocument::saveChanges() - text storage");
            if (!_textStorage->save(maxTime)) {
                CRLog::error("Error while saving text data");
                return CR_ERROR;
            }
            CHECK_EXPIRATION("saving text storate")
            if (progressCallback)
                progressCallback->OnSaveCacheFileProgress(20);
            // fall through
        case 3:
            _mapSavingStage = 3;
            CRLog::trace("ldomDocument::saveChanges() - rect storage");

            if (!_rectStorage->save(maxTime)) {
                CRLog::error("Error while saving rect data");
                return CR_ERROR;
            }
            CHECK_EXPIRATION("saving rect storate")
            if (progressCallback)
                progressCallback->OnSaveCacheFileProgress(30);
            // fall through
        case 41:
            _mapSavingStage = 41;
            CRLog::trace("ldomDocument::saveChanges() - blob storage data");

            if (_blobCache->saveToCache(maxTime) == CR_ERROR) {
                CRLog::error("Error while saving blob storage data");
                return CR_ERROR;
            }
            if (!maxTime.infinite())
                _cacheFile->flush(false, maxTime); // intermediate flush
            CHECK_EXPIRATION("saving blob storage data")
            if (progressCallback)
                progressCallback->OnSaveCacheFileProgress(35);
            // fall through
        case 4:
            _mapSavingStage = 4;
            CRLog::trace("ldomDocument::saveChanges() - node style storage");

            if (!_styleStorage->save(maxTime)) {
                CRLog::error("Error while saving node style data");
                return CR_ERROR;
            }
            if (!maxTime.infinite())
                _cacheFile->flush(false, maxTime); // intermediate flush
            CHECK_EXPIRATION("saving node style storage")
            if (progressCallback)
                progressCallback->OnSaveCacheFileProgress(40);
            // fall through
        case 5:
            _mapSavingStage = 5;
            CRLog::trace("ldomDocument::saveChanges() - misc data");
            {
                SerialBuf propsbuf(4096);
                getProps()->serialize(propsbuf);
                if (!_cacheFile->write(CBT_PROP_DATA, propsbuf, COMPRESS_MISC_DATA)) {
                    CRLog::error("Error while saving props data");
                    return CR_ERROR;
                }
            }
            if (!maxTime.infinite())
                _cacheFile->flush(false, maxTime); // intermediate flush
            CHECK_EXPIRATION("saving props data")
            if (progressCallback)
                progressCallback->OnSaveCacheFileProgress(45);
            // fall through
        case 6:
            _mapSavingStage = 6;
            CRLog::trace("ldomDocument::saveChanges() - ID data");
            {
                SerialBuf idbuf(4096);
                serializeMaps(idbuf);
                if (!_cacheFile->write(CBT_MAPS_DATA, idbuf, COMPRESS_MISC_DATA)) {
                    CRLog::error("Error while saving Id data");
                    return CR_ERROR;
                }
            }
            if (!maxTime.infinite())
                _cacheFile->flush(false, maxTime); // intermediate flush
            CHECK_EXPIRATION("saving ID data")
            if (progressCallback)
                progressCallback->OnSaveCacheFileProgress(50);
            // fall through
        case 7:
            _mapSavingStage = 7;
            if (_pagesData.pos()) {
                CRLog::trace("ldomDocument::saveChanges() - page data (%d bytes)", _pagesData.pos());
                if (!_cacheFile->write(CBT_PAGE_DATA, _pagesData, COMPRESS_PAGES_DATA)) {
                    CRLog::error("Error while saving pages data");
                    return CR_ERROR;
                }
            } else {
                CRLog::trace("ldomDocument::saveChanges() - no page data");
            }
            if (!maxTime.infinite())
                _cacheFile->flush(false, maxTime); // intermediate flush
            CHECK_EXPIRATION("saving page data")
            if (progressCallback)
                progressCallback->OnSaveCacheFileProgress(60);
            // fall through
        case 8:
            _mapSavingStage = 8;

            CRLog::trace("ldomDocument::saveChanges() - node data");
            if (!saveNodeData()) {
                CRLog::error("Error while node instance data");
                return CR_ERROR;
            }
            if (!maxTime.infinite())
                _cacheFile->flush(false, maxTime); // intermediate flush
            CHECK_EXPIRATION("saving node data")
            if (progressCallback)
                progressCallback->OnSaveCacheFileProgress(70);
            // fall through
        case 9:
            _mapSavingStage = 9;
            CRLog::trace("ldomDocument::saveChanges() - render info");
            {
                SerialBuf hdrbuf(0, true);
                if (!_hdr.serialize(hdrbuf)) {
                    CRLog::error("Header data serialization is failed");
                    return CR_ERROR;
                } else if (!_cacheFile->write(CBT_REND_PARAMS, hdrbuf, false)) {
                    CRLog::error("Error while writing header data");
                    return CR_ERROR;
                }
            }
            CRLog::info("Saving render properties: styleHash=%x, stylesheetHash=%x, docflags=%x, width=%x, height=%x, nodeDisplayStyleHash=%x",
                        _hdr.render_style_hash, _hdr.stylesheet_hash, _hdr.render_docflags, _hdr.render_dx, _hdr.render_dy, _hdr.node_displaystyle_hash);
            if (progressCallback)
                progressCallback->OnSaveCacheFileProgress(73);

            CRLog::trace("ldomDocument::saveChanges() - TOC");
            {
                SerialBuf tocbuf(0, true);
                if (!m_toc.serialize(tocbuf)) {
                    CRLog::error("TOC data serialization is failed");
                    return CR_ERROR;
                } else if (!_cacheFile->write(CBT_TOC_DATA, tocbuf, COMPRESS_TOC_DATA)) {
                    CRLog::error("Error while writing TOC data");
                    return CR_ERROR;
                }
            }
            if (progressCallback)
                progressCallback->OnSaveCacheFileProgress(76);

            CRLog::trace("ldomDocument::saveChanges() - PageMap");
            {
                SerialBuf pagemapbuf(0, true);
                if (!m_pagemap.serialize(pagemapbuf)) {
                    CRLog::error("PageMap data serialization is failed");
                    return CR_ERROR;
                } else if (!_cacheFile->write(CBT_PAGEMAP_DATA, pagemapbuf, COMPRESS_PAGEMAP_DATA)) {
                    CRLog::error("Error while writing PageMap data");
                    return CR_ERROR;
                }
            }
            if (!maxTime.infinite())
                _cacheFile->flush(false, maxTime); // intermediate flush
            CHECK_EXPIRATION("saving TOC data")
            if (progressCallback)
                progressCallback->OnSaveCacheFileProgress(80);
            // fall through
        case 10:
            _mapSavingStage = 10;

            if (!saveStylesData()) {
                CRLog::error("Error while writing style data");
                return CR_ERROR;
            }
            if (progressCallback)
                progressCallback->OnSaveCacheFileProgress(90);
            // fall through
        case 11:
            _mapSavingStage = 11;
            CRLog::trace("ldomDocument::saveChanges() - embedded fonts");
            {
                SerialBuf buf(4096);
                _fontList.serialize(buf);
                if (!_cacheFile->write(CBT_FONT_DATA, buf, COMPRESS_MISC_DATA)) {
                    CRLog::error("Error while saving embedded font data");
                    return CR_ERROR;
                }
                CHECK_EXPIRATION("saving embedded fonts")
            }
            if (progressCallback)
                progressCallback->OnSaveCacheFileProgress(95);
            // fall through
        case 12:
            _mapSavingStage = 12;
            CRLog::trace("ldomDocument::saveChanges() - flush");
            {
                CRTimerUtil infinite;
                if (!_cacheFile->flush(true, infinite)) {
                    CRLog::error("Error while updating index of cache file");
                    return CR_ERROR;
                }
                CHECK_EXPIRATION("flushing")
            }
            if (progressCallback)
                progressCallback->OnSaveCacheFileProgress(100);
            // fall through
        case 13:
            _mapSavingStage = 13;
            setCacheFileStale(false);
    }
    CRLog::trace("ldomDocument::saveChanges() - done");
    if (progressCallback)
        progressCallback->OnSaveCacheFileEnd();

    // And now should be a good place to release compression resources...
    _cacheFile->cleanupCompressor();

    return CR_DONE;
}

/// save changes to cache file, @see loadCacheFileContent()
bool ldomDocument::saveChanges() {
    if (!_cacheFile)
        return true;
    CRLog::debug("ldomDocument::saveChanges() - infinite");
    CRTimerUtil timerNoLimit;
    ContinuousOperationResult res = saveChanges(timerNoLimit);
    return res != CR_ERROR;
}

/// swaps to cache file or saves changes, limited by time interval
ContinuousOperationResult ldomDocument::swapToCache(CRTimerUtil& maxTime) {
    CRLog::trace("ldomDocument::swapToCache entered");
    if (_maperror)
        return CR_ERROR;
    if (!_mapped) {
        CRLog::trace("ldomDocument::swapToCache creating cache file");
        if (!createCacheFile()) {
            CRLog::error("ldomDocument::swapToCache: failed: cannot create cache file");
            _maperror = true;
            return CR_ERROR;
        }
    }
    _mapped = true;
    if (!maxTime.infinite()) {
        CRLog::info("Cache file is created, but document saving is postponed");
        return CR_TIMEOUT;
    }
    ContinuousOperationResult res = saveChanges(maxTime);
    if (res == CR_ERROR) {
        CRLog::error("Error while saving changes to cache file");
        _maperror = true;
        return CR_ERROR;
    }
    CRLog::info("Successfully saved document to cache file: %dK", _cacheFile->getSize() / 1024);
    return res;
}

/// saves recent changes to mapped file
ContinuousOperationResult ldomDocument::updateMap(CRTimerUtil& maxTime, LVDocViewCallback* progressCallback) {
    if (!_cacheFile || !_mapped) {
        CRLog::info("No cache file or not mapped");
        return CR_DONE;
    }

    if (_cacheFileLeaveAsDirty) {
        CRLog::info("Requested to set cache file as dirty without any update");
        _cacheFile->setDirtyFlag(true);
        return CR_DONE;
    }

    if (!_cacheFileStale) {
        CRLog::info("No change, cache file update not needed");
        return CR_DONE;
    }
    CRLog::info("Updating cache file");

    ContinuousOperationResult res = saveChanges(maxTime, progressCallback); // NOLINT: Call to virtual function during destruction
    if (res == CR_ERROR) {
        CRLog::error("Error while saving changes to cache file");
        return CR_ERROR;
    }

    if (res == CR_DONE) {
        CRLog::info("Cache file updated successfully");
        dumpStatistics();
    }
    return res;
}

/// save document formatting parameters after render
void ldomDocument::updateRenderContext() {
    int dx = _page_width;
    int dy = _page_height;
    _nodeStyleHash = 0; // force recalculation by calcStyleHash()
    lUInt32 styleHash = calcStyleHash(_rendered);
    lUInt32 stylesheetHash = (((_stylesheet.getHash() * 31) + calcHash(_def_style)) * 31 + calcHash(_def_font));
    _hdr.render_style_hash = styleHash;
    _hdr.stylesheet_hash = stylesheetHash;
    _hdr.render_dx = dx;
    _hdr.render_dy = dy;
    _hdr.render_docflags = _docFlags;
    _hdr.node_displaystyle_hash = _nodeDisplayStyleHashInitial; // we keep using the initial one
    CRLog::info("Updating render properties: styleHash=%x, stylesheetHash=%x, docflags=%x, width=%x, height=%x, nodeDisplayStyleHash=%x",
                _hdr.render_style_hash, _hdr.stylesheet_hash, _hdr.render_docflags, _hdr.render_dx, _hdr.render_dy, _hdr.node_displaystyle_hash);
    _doc_rendering_hash = ((((((
                                       +(lUInt32)dx) *
                                       31 +
                               (lUInt32)dy) *
                                      31 +
                              (lUInt32)_docFlags) *
                                     31 +
                             (lUInt32)_nodeDisplayStyleHashInitial) *
                                    31 +
                            (lUInt32)stylesheetHash) *
                                   31 +
                           (lUInt32)styleHash);
}

/// check document formatting parameters before render - whether we need to reformat; returns false if render is necessary
bool ldomDocument::checkRenderContext() {
    bool res = true;
    ldomNode* node = getRootNode();
    if (node != NULL && node->getFont().isNull()) {
        // This may happen when epubfmt.cpp has called forceReinitStyles()
        // because the EPUB contains embedded fonts: a full nodes styles
        // re-init is needed to use the new fonts (only available at end
        // of loading)
        CRLog::info("checkRenderContext: style is not set for root node");
        res = false;
    }
    int dx = _page_width;
    int dy = _page_height;
    lUInt32 styleHash = calcStyleHash(_rendered);
    lUInt32 stylesheetHash = (((_stylesheet.getHash() * 31) + calcHash(_def_style)) * 31 + calcHash(_def_font));
    if (styleHash != _hdr.render_style_hash) {
        CRLog::info("checkRenderContext: Style hash doesn't match %x!=%x", styleHash, _hdr.render_style_hash);
        res = false;
        if (_just_rendered_from_cache)
            CRLog::warn("CRE WARNING: cached rendering is invalid (style hash mismatch): doing full rendering\n");
    } else if (stylesheetHash != _hdr.stylesheet_hash) {
        CRLog::info("checkRenderContext: Stylesheet hash doesn't match %x!=%x", stylesheetHash, _hdr.stylesheet_hash);
        res = false;
        if (_just_rendered_from_cache)
            CRLog::warn("CRE WARNING: cached rendering is invalid (stylesheet hash mismatch): doing full rendering\n");
    } else if (_docFlags != _hdr.render_docflags) {
        CRLog::info("checkRenderContext: Doc flags don't match %x!=%x", _docFlags, _hdr.render_docflags);
        res = false;
        if (_just_rendered_from_cache)
            CRLog::warn("CRE WARNING: cached rendering is invalid (doc flags mismatch): doing full rendering\n");
    } else if (dx != (int)_hdr.render_dx) {
        CRLog::info("checkRenderContext: Width doesn't match %x!=%x", dx, (int)_hdr.render_dx);
        res = false;
        if (_just_rendered_from_cache)
            CRLog::warn("CRE WARNING: cached rendering is invalid (page width mismatch): doing full rendering\n");
    } else if (dy != (int)_hdr.render_dy) {
        CRLog::info("checkRenderContext: Page height doesn't match %x!=%x", dy, (int)_hdr.render_dy);
        res = false;
        if (_just_rendered_from_cache)
            CRLog::warn("CRE WARNING: cached rendering is invalid (page height mismatch): doing full rendering\n");
    }
    // no need to check for _nodeDisplayStyleHash != _hdr.node_displaystyle_hash:
    // this is implicitely done by styleHash != _hdr.render_style_hash (whose _nodeDisplayStyleHash is a subset)
    _just_rendered_from_cache = false;
    if (res) {
        //if ( pages->length()==0 ) {
        //            _pagesData.reset();
        //            pages->deserialize( _pagesData );
        //}

        return true;
    }
    //    _hdr.render_style_hash = styleHash;
    //    _hdr.stylesheet_hash = stylesheetHash;
    //    _hdr.render_dx = dx;
    //    _hdr.render_dy = dy;
    //    _hdr.render_docflags = _docFlags;
    //    CRLog::info("New render properties: styleHash=%x, stylesheetHash=%x, docflags=%04x, width=%d, height=%d",
    //                _hdr.render_style_hash, _hdr.stylesheet_hash, _hdr.render_docflags, _hdr.render_dx, _hdr.render_dy);
    return false;
}

/// register embedded document fonts in font manager, if any exist in document
void ldomDocument::registerEmbeddedFonts() {
    if (_fontList.empty())
        return;
    int list = _fontList.length();
    lString8 x = lString8("");
    lString32Collection flist;
    fontMan->getFaceList(flist);
    int cnt = flist.length();
    for (int i = 0; i < list; i++) {
        LVEmbeddedFontDef* item = _fontList.get(i);
        lString32 url = item->getUrl();
        lString8 face = item->getFace();
        if (face.empty()) {
            for (int a = i + 1; a < list; a++) {
                lString8 tmp = _fontList.get(a)->getFace();
                if (!tmp.empty()) {
                    face = tmp;
                    break;
                }
            }
        }
        if ((!x.empty() && x.pos(face) != -1) || url.empty()) {
            continue;
        }
        if (url.startsWithNoCase(lString32("res://")) || url.startsWithNoCase(lString32("file://"))) {
            if (!fontMan->RegisterExternalFont(getDocIndex(), item->getUrl(), item->getFace(), item->getBold(), item->getItalic())) {
                //CRLog::error("Failed to register external font face: %s file: %s", item->getFace().c_str(), LCSTR(item->getUrl()));
            }
            continue;
        } else {
            if (!fontMan->RegisterDocumentFont(getDocIndex(), _container, item->getUrl(), item->getFace(), item->getBold(), item->getItalic())) {
                //CRLog::error("Failed to register document font face: %s file: %s", item->getFace().c_str(), LCSTR(item->getUrl()));
                lString32 fontface = lString32("");
                for (int j = 0; j < cnt; j = j + 1) {
                    fontface = flist[j];
                    do {
                        (fontface.replace(lString32(" "), lString32("\0")));
                    } while (fontface.pos(lString32(" ")) != -1);
                    do {
                        (url.replace(lString32(" "), lString32("\0")));
                    } while (url.pos(lString32(" ")) != -1);
                    if (fontface.lowercase().pos(url.lowercase()) != -1) {
                        if (fontMan->SetAlias(face, UnicodeToLocal(flist[j]), getDocIndex(), item->getBold(), item->getItalic())) {
                            x.append(face).append(lString8(","));
                            CRLog::debug("font-face %s matches local font %s", face.c_str(), LCSTR(flist[j]));
                            break;
                        }
                    }
                }
            }
        }
    }
}
/// unregister embedded document fonts in font manager, if any exist in document
void ldomDocument::unregisterEmbeddedFonts() {
    fontMan->UnregisterDocumentFonts(_docIndex);
}

/// returns object image stream
LVStreamRef ldomDocument::getObjectImageStream(lString32 refName) {
    LVStreamRef ref;
    if (refName.startsWith(lString32(BLOB_NAME_PREFIX))) {
        return _blobCache->getBlob(refName);
    }
    if (refName.length() > 10 && refName[4] == ':' && refName.startsWith(lString32("data:image/"))) {
        // <img src="data:image/png;base64,iVBORw0KG...>
        lString32 data = refName.substr(0, 50);
        int pos = data.pos(U";base64,");
        if (pos > 0) {
            lString8 b64data = UnicodeToUtf8(refName.substr(pos + 8));
            ref = LVStreamRef(new LVBase64Stream(b64data));
            return ref;
        }
        // Non-standard plain string data (can be used to provide SVG)
        pos = data.pos(U";-cr-plain,");
        if (pos > 0) {
            lString8 plaindata = UnicodeToUtf8(refName.substr(pos + 11));
            ref = LVCreateStringStream(plaindata);
            return ref;
        }
    }
    if (refName[0] != '#') {
        if (!getContainer().isNull()) {
            lString32 name = refName;
            if (!getCodeBase().empty())
                name = getCodeBase() + refName;
            ref = getContainer()->OpenStream(name.c_str(), LVOM_READ);
            if (ref.isNull()) {
                lString32 fname = getProps()->getStringDef(DOC_PROP_FILE_NAME, "");
                fname = LVExtractFilenameWithoutExtension(fname);
                if (!fname.empty()) {
                    lString32 fn = fname + "_img";
                    //                    if ( getContainer()->GetObjectInfo(fn) ) {

                    //                    }
                    lString32 name = fn + "/" + refName;
                    if (!getCodeBase().empty())
                        name = getCodeBase() + name;
                    ref = getContainer()->OpenStream(name.c_str(), LVOM_READ);
                }
            }
            if (ref.isNull())
                CRLog::error("Cannot open stream by name %s", LCSTR(name));
        }
        return ref;
    }
    lUInt32 refValueId = findAttrValueIndex(refName.c_str() + 1);
    if (refValueId == (lUInt32)-1) {
        return ref;
    }
    ldomNode* objnode = getNodeById(refValueId);
    if (!objnode || !objnode->isElement())
        return ref;
    ref = objnode->createBase64Stream();
    return ref;
}

/// returns object image source
LVImageSourceRef ldomDocument::getObjectImageSource(lString32 refName) {
    LVStreamRef stream = getObjectImageStream(refName);
    if (stream.isNull())
        return LVImageSourceRef();
    return LVCreateStreamImageSource(stream);
}

void ldomDocument::resetNodeNumberingProps() {
    lists.clear();
}

ListNumberingPropsRef ldomDocument::getNodeNumberingProps(lUInt32 nodeDataIndex) {
    return lists.get(nodeDataIndex);
}

void ldomDocument::setNodeNumberingProps(lUInt32 nodeDataIndex, ListNumberingPropsRef v) {
    lists.set(nodeDataIndex, v);
}

static inline void makeTocFromCrHintsOrHeadings(ldomNode* node, bool ensure_cr_hints) {
    int level;
    if (ensure_cr_hints) {
        css_style_ref_t style = node->getStyle();
        if (STYLE_HAS_CR_HINT(style, TOC_IGNORE))
            return; // requested to be ignored via style tweaks
        if (STYLE_HAS_CR_HINT(style, TOC_LEVELS_MASK)) {
            if (STYLE_HAS_CR_HINT(style, TOC_LEVEL1))
                level = 1;
            else if (STYLE_HAS_CR_HINT(style, TOC_LEVEL2))
                level = 2;
            else if (STYLE_HAS_CR_HINT(style, TOC_LEVEL3))
                level = 3;
            else if (STYLE_HAS_CR_HINT(style, TOC_LEVEL4))
                level = 4;
            else if (STYLE_HAS_CR_HINT(style, TOC_LEVEL5))
                level = 5;
            else if (STYLE_HAS_CR_HINT(style, TOC_LEVEL6))
                level = 6;
            else
                level = 7; // should not be reached
        } else if (node->getNodeId() >= el_h1 && node->getNodeId() <= el_h6)
            // el_h1 .. el_h6 are consecutive and ordered in include/fb2def.h
            level = node->getNodeId() - el_h1 + 1;
        else
            return;
    } else {
        if (node->getNodeId() >= el_h1 && node->getNodeId() <= el_h6)
            // el_h1 .. el_h6 are consecutive and ordered in include/fb2def.h
            level = node->getNodeId() - el_h1 + 1;
        else
            return;
    }
    lString32 title = removeSoftHyphens(node->getText(' '));
    ldomXPointer xp = ldomXPointer(node, 0);
    LVTocItem* root = node->getDocument()->getToc();
    LVTocItem* parent = root;
    // Find adequate parent, or create intermediates
    int plevel = 1;
    while (plevel < level) {
        int nbc = parent->getChildCount();
        if (nbc) { // use the latest child
            parent = parent->getChild(nbc - 1);
        } else {
            // If we'd like to stick it to the last parent found, even if
            // of wrong level, just do: break;
            // But it is cleaner to create intermediate(s)
            parent = parent->addChild(U"", xp, lString32::empty_str);
        }
        plevel++;
    }
    parent->addChild(title, xp, lString32::empty_str);
}

static void makeTocFromHeadings(ldomNode* node) {
    makeTocFromCrHintsOrHeadings(node, false);
}

static void makeTocFromCrHintsOrHeadings(ldomNode* node) {
    makeTocFromCrHintsOrHeadings(node, true);
}

static void makeTocFromDocFragments(ldomNode* node) {
    if (node->getNodeId() != el_DocFragment)
        return;
    // No title, and only level 1 with DocFragments
    ldomXPointer xp = ldomXPointer(node, 0);
    LVTocItem* root = node->getDocument()->getToc();
    root->addChild(U"", xp, lString32::empty_str);
}

void ldomDocument::buildTocFromHeadings() {
    m_toc.clear();
    getRootNode()->recurseElements(makeTocFromHeadings);
}

void ldomDocument::buildAlternativeToc() {
    m_toc.clear();
    // Look first for style tweaks specified -cr-hint: toc-level1 ... toc-level6
    // and/or headings (H1...H6)
    getRootNode()->recurseElements(makeTocFromCrHintsOrHeadings);
    // If no heading or hints found, fall back to gathering DocFraments
    if (!m_toc.getChildCount())
        getRootNode()->recurseElements(makeTocFromDocFragments);
    // m_toc.setAlternativeTocFlag() uses the root toc item _page property
    // (never used for the root node) to store the fact this is an
    // alternatve TOC. This info can then be serialized to cache and
    // retrieved without any additional work or space overhead.
    m_toc.setAlternativeTocFlag();
    // cache file will have to be updated with the alt TOC
    setCacheFileStale(true);
    _toc_from_cache_valid = false; // to force update of page numbers
}
