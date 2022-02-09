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

#include "ldomdocumentwriter.h"

#include <ldomnode.h>
#include <ldomdocument.h>
#include <fb2def.h>
#include <lvstreamutils.h>
#include <crlog.h>

#include "ldomelementwriter.h"
#include "../lvxml/lvfileformatparser.h"
#include "../lvxml/lvxmlutils.h"

#include <stdio.h>

static bool hasInvisibleParent(ldomNode* node) {
    for (; node && !node->isRoot(); node = node->getParentNode()) {
        css_style_ref_t style = node->getStyle();
        if (!style.isNull() && style->display == css_d_none)
            return true;
    }
    return false;
}

/////////////////////////////////////////////////////////////////
/// ldomDocumentWriter
// Used to parse expected XHTML (possibly made by crengine or helpers) for
// formats: FB2, RTF, WORD, plain text, PDB(txt)
// Also used for EPUB to build a single document, but driven by ldomDocumentFragmentWriter
// for each individual HTML files in the EPUB.
// For all these document formats, it is fed by HTMLParser that does
// convert to lowercase the tag names and attributes.
// ldomDocumentWriter does not do any auto-close of unbalanced tags and
// expect a fully correct and balanced XHTML.

// overrides

void ldomDocumentWriter::ElementCloseHandler(ldomNode* node) {
    node->persist();
}

void ldomDocumentWriter::OnStart(LVFileFormatParser* parser) {
    //logfile << "ldomDocumentWriter::OnStart()\n";
    // add document root node
    //CRLog::trace("ldomDocumentWriter::OnStart()");
    if (!_headerOnly)
        _stopTagId = 0xFFFE;
    else {
        _stopTagId = _document->getElementNameIndex(U"description");
        //CRLog::trace( "ldomDocumentWriter() : header only, tag id=%d", _stopTagId );
    }
    LVXMLParserCallback::OnStart(parser);
    _currNode = new ldomElementWriter(_document, 0, 0, NULL);
}

void ldomDocumentWriter::OnStop() {
    //logfile << "ldomDocumentWriter::OnStop()\n";
    while (_currNode)
        _currNode = pop(_currNode, _currNode->getElement()->getNodeId());
}

/// called after > of opening tag (when entering tag body)
// Note to avoid confusion: all tags HAVE a body (their content), so this
// is called on all tags.
// But in this, we do some specifics for tags that ARE a <BODY> tag.
void ldomDocumentWriter::OnTagBody() {
    // Specific if we meet the <BODY> tag and we have styles to apply and
    // store in the DOM
    // (This can't happen with EPUBs: the ldomDocumentFragmentWriter that
    // drives this ldomDocumentWriter has parsed the HEAD STYLEs and LINKs
    // of each individual HTML file, and we see from them only their BODY:
    // _headStyleText and _stylesheetLinks are then empty. Styles for EPUB
    // are handled in :OnTagOpen() when being a DocFragment and meeting
    // the BODY.)
    if (_currNode && _currNode->getElement() && _currNode->getElement()->isNodeName("body") &&
        (!_headStyleText.empty() || _stylesheetLinks.length() > 0)) {
        // If we're BODY, and we have meet styles in the previous HEAD
        // (links to css files or <STYLE> content), we need to save them
        // in an added <body><stylesheet> element so they are in the DOM
        // and saved in the cache, and found again when loading from cache
        // and applied again when a re-rendering is needed.

        // Make out an aggregated single stylesheet text.
        // @import's need to be first in the final stylesheet text
        lString32 imports;
        for (int i = 0; i < _stylesheetLinks.length(); i++) {
            lString32 import("@import url(\"");
            import << _stylesheetLinks.at(i);
            import << "\");\n";
            imports << import;
        }
        lString32 styleText = imports + _headStyleText.c_str();
        _stylesheetLinks.clear();
        _headStyleText.clear();

        // It's only at this point that we push() the previous stylesheet state
        // and apply the combined style text we made to the document:
        if (_document->getDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES)) {
            _document->getStyleSheet()->push();
            _popStyleOnFinish = true; // superclass ~ldomDocumentWriter() will do the ->pop()
            _document->parseStyleSheet(lString32(), styleText);
            // printf("applied: %s\n", LCSTR(styleText));
            // apply any FB2 stylesheet too, so it's removed too when pop()
            _document->applyDocumentStyleSheet();
        }
        // We needed to add that /\ to the _document->_stylesheet before this
        // onBodyEnter \/, for any body {} css declaration to be available
        // as this onBodyEnter will apply the current _stylesheet to this BODY node.
        _currNode->onBodyEnter();
        _flags = _currNode->getFlags(); // _flags may have been updated (if white-space: pre)
        // And only after this we can add the <stylesheet> as a first child
        // element of this BODY node. It will not be displayed thanks to fb2def.h:
        //   XS_TAG1D( stylesheet, true, css_d_none, css_ws_inherit )
        OnTagOpen(U"", U"stylesheet");
        OnTagBody();
        OnText(styleText.c_str(), styleText.length(), 0);
        OnTagClose(U"", U"stylesheet");
        CRLog::trace("added BODY>stylesheet child element with HEAD>STYLE&LINKS content");
    } else if (_currNode) { // for all other tags (including BODY when no style)
#if MATHML_SUPPORT == 1
        if (_currNode->_insideMathML) {
            // All attributes are set, this may add other attributes depending
            // on ancestors: to be done before onBodyEnter() if these attributes
            // may affect styling.
            _mathMLHelper.handleMathMLtag(this, MATHML_STEP_NODE_SET, el_NULL);
        }
#endif
        _currNode->onBodyEnter();
        _flags = _currNode->getFlags(); // _flags may have been updated (if white-space: pre)
    }

#if MATHML_SUPPORT == 1
    if (_currNode && _currNode->_insideMathML) {
        // At this point, the style for this node has been applied.
        // Check if the <math> element (or any of its parent) is display:none,
        // in which case we don't need to spend time handling MathML that
        // won't be shown.
        if (_currNode->getElement()->getNodeId() == el_math && hasInvisibleParent(_currNode->getElement())) {
            _currNode->_insideMathML = false;
        } else {
            // This may create a wrapping mathBox around all this element's children
            _mathMLHelper.handleMathMLtag(this, MATHML_STEP_NODE_ENTERED, el_NULL);
        }
    }
#endif
}

ldomNode* ldomDocumentWriter::OnTagOpen(const lChar32* nsname, const lChar32* tagname) {
    //logfile << "ldomDocumentWriter::OnTagOpen() [" << nsname << ":" << tagname << "]";
    //CRLog::trace("OnTagOpen(%s)", UnicodeToUtf8(lString32(tagname)).c_str());
    lUInt16 id = _document->getElementNameIndex(tagname);
    lUInt16 nsid = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;

#if MATHML_SUPPORT == 1
    if ((_currNode && _currNode->_insideMathML) || (id == el_math)) {
        // This may create a wrapping mathBox around this new element
        _mathMLHelper.handleMathMLtag(this, MATHML_STEP_BEFORE_NEW_CHILD, id);
    }
#endif

    // Set a flag for OnText to accumulate the content of any <HEAD><STYLE>
    if (id == el_style && _currNode && _currNode->getElement()->getNodeId() == el_head) {
        _inHeadStyle = true;
    }

    // For EPUB, when ldomDocumentWriter is driven by ldomDocumentFragmentWriter:
    // if we see a BODY coming and we are a DocFragment, its time to apply the
    // styles set to the DocFragment before switching to BODY (so the styles can
    // be applied to BODY)
    if (id == el_body && _currNode && _currNode->_element->getNodeId() == el_DocFragment) {
        _currNode->_stylesheetIsSet = _currNode->getElement()->applyNodeStylesheet();
        // _stylesheetIsSet will be used to pop() the stylesheet when
        // leaving/destroying this DocFragment ldomElementWriter
    }

    //if ( id==_stopTagId ) {
    //CRLog::trace("stop tag found, stopping...");
    //    _parser->Stop();
    //}
    _currNode = new ldomElementWriter(_document, nsid, id, _currNode);
    _flags = _currNode->getFlags();
    //logfile << " !o!\n";
    //return _currNode->getElement();
    return _currNode->getElement();
}

ldomDocumentWriter::~ldomDocumentWriter() {
    while (_currNode)
        _currNode = pop(_currNode, _currNode->getElement()->getNodeId());
    if (_document->isDefStyleSet()) {
        if (_popStyleOnFinish)
            // pop any added styles to the original stylesheet so we get
            // the original one back and avoid a stylesheet hash mismatch
            _document->getStyleSheet()->pop();
        // Not sure why we would do that at end of parsing, but ok: it's
        // not recursive, so not expensive:
        _document->getRootNode()->initNodeStyle();
        _document->getRootNode()->initNodeFont();
        //if ( !_document->validateDocument() )
        //    CRLog::error("*** document style validation failed!!!");
        _document->updateRenderContext();
        _document->dumpStatistics();
        if (_document->_nodeStylesInvalidIfLoading) {
            // Some pseudoclass like :last-child has been met which has set this flag
            // (or, with the HTML parser, foster parenting of invalid element in tables)
            printf("CRE: document loaded, but styles re-init needed (cause: peculiar CSS pseudoclasses met)\n");
            _document->_nodeStylesInvalidIfLoading = false; // show this message only once
            _document->forceReinitStyles();
        }
        if (_document->hasRenderData()) {
            // We have created some RenderRectAccessors, to cache some CSS check results
            // (i.e. :nth-child(), :last-of-type...): we should clean them.
            // (We do that here for after the initial loading phase - on re-renderings,
            // this is done in updateRendMethod() called by initNodeRendMethodRecursive()
            // on all nodes.)
            _document->getRootNode()->clearRenderDataRecursive();
        }
        _document->_parsing = false; // done parsing
    }
}

void ldomDocumentWriter::OnTagClose(const lChar32*, const lChar32* tagname, bool self_closing_tag) {
    //logfile << "ldomDocumentWriter::OnTagClose() [" << nsname << ":" << tagname << "]";
    if (!_currNode || !_currNode->getElement()) {
        _errFlag = true;
        //logfile << " !c-err!\n";
        return;
    }

    //lUInt16 nsid = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;
    lUInt16 curNodeId = _currNode->getElement()->getNodeId();
    lUInt16 id = _document->getElementNameIndex(tagname);
    _errFlag |= (id != curNodeId); // (we seem to not do anything with _errFlag)
    // We should expect the tagname we got to be the same as curNode's element name,
    // but it looks like we may get an upper closing tag, that pop() below might
    // handle. So, here below, we check that both id and curNodeId match the
    // element id we check for.

    // Parse <link rel="stylesheet">, put the css file link in _stylesheetLinks.
    // They will be added to <body><stylesheet> when we meet <BODY>
    // (duplicated in ldomDocumentWriterFilter::OnTagClose)
    if (id == el_link && curNodeId == el_link) { // link node
        ldomNode* n = _currNode->getElement();
        if (n->getParentNode() && n->getParentNode()->getNodeId() == el_head &&
            lString32(n->getAttributeValue("rel")).lowercase() == U"stylesheet" &&
            lString32(n->getAttributeValue("type")).lowercase() == U"text/css") {
            lString32 href = n->getAttributeValue("href");
            lString32 stylesheetFile = LVCombinePaths(_document->getCodeBase(), href);
            CRLog::debug("Internal stylesheet file: %s", LCSTR(stylesheetFile));
            // We no more apply it immediately: it will be when <BODY> is met
            // _document->setDocStylesheetFileName(stylesheetFile);
            // _document->applyDocumentStyleSheet();
            _stylesheetLinks.add(stylesheetFile);
        }
    }

#if MATHML_SUPPORT == 1
    if (_currNode->_insideMathML) {
        if (_mathMLHelper.handleMathMLtag(this, MATHML_STEP_NODE_CLOSING, id)) {
            // curnode may have changed
            curNodeId = _currNode->getElement()->getNodeId();
            id = tagname ? _document->getElementNameIndex(tagname) : curNodeId;
            _errFlag |= (id != curNodeId); // (we seem to not do anything with _errFlag)
        }
    }
#endif

    _currNode = pop(_currNode, id);
    // _currNode is now the parent

#if MATHML_SUPPORT == 1
    if (_currNode->_insideMathML) {
        // This may close wrapping mathBoxes
        _mathMLHelper.handleMathMLtag(this, MATHML_STEP_NODE_CLOSED, id);
    }
#endif

    if (_currNode)
        _flags = _currNode->getFlags();

    if (id == _stopTagId) {
        //CRLog::trace("stop tag found, stopping...");
        _parser->Stop();
    }

    // For EPUB/HTML, this is now dealt with in :OnTagBody(), just before creating this <stylesheet> tag.
    // But for FB2, where we have:
    //   <FictionBook>
    //     <stylesheet type="text/css">
    //       some css
    //     </stylesheet>
    //     <p>...
    //     other content
    //   </FictionBook>
    // we need to apply the <stylesheet> content we have just left, so it applies
    // to the coming up content.
    // We check the parent we have just pop'ed is a <FictionBook>.
    // Caveat: any style set on the <FictionBook> element itself won't be applied now
    // in this loading phase (as we have already set its style) - but it will apply
    // on re-renderings.
    if (id == el_stylesheet && _currNode && _currNode->getElement()->getNodeId() == el_FictionBook) {
        //CRLog::trace("</stylesheet> found");
        if (!_popStyleOnFinish && _document->getDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES)) {
            //CRLog::trace("saving current stylesheet before applying of document stylesheet");
            _document->getStyleSheet()->push();
            _popStyleOnFinish = true;
            _document->applyDocumentStyleSheet();
        }
    }

    //logfile << " !c!\n";
}

void ldomDocumentWriter::OnAttribute(const lChar32* nsname, const lChar32* attrname, const lChar32* attrvalue) {
    //logfile << "ldomDocumentWriter::OnAttribute() [" << nsname << ":" << attrname << "]";
    lUInt16 attr_ns = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;
    lUInt16 attr_id = (attrname && attrname[0]) ? _document->getAttrNameIndex(attrname) : 0;
    _currNode->addAttribute(attr_ns, attr_id, attrvalue);

    //logfile << " !a!\n";
}

void ldomDocumentWriter::OnText(const lChar32* text, int len, lUInt32 flags) {
    //logfile << "ldomDocumentWriter::OnText() fpos=" << fpos;

    // Accumulate <HEAD><STYLE> content
    if (_inHeadStyle) {
        _headStyleText << lString32(text, len);
        _inHeadStyle = false;
        return;
    }

    if (_currNode) {
        if ((_flags & XML_FLAG_NO_SPACE_TEXT) && IsEmptySpace(text, len) && !(flags & TXTFLG_PRE))
            return;
#if MATHML_SUPPORT == 1
        if (_currNode->_insideMathML) {
            lString32 math_text = _mathMLHelper.getMathMLAdjustedText(_currNode->getElement(), text, len);
            if (!math_text.empty()) {
                _mathMLHelper.handleMathMLtag(this, MATHML_STEP_BEFORE_NEW_CHILD, el_NULL);
                _currNode->onText(math_text.c_str(), math_text.length(), flags);
            }
            return;
        }
#endif
        if (_currNode->_allowText)
            _currNode->onText(text, len, flags);
    }
    //logfile << " !t!\n";
}

bool ldomDocumentWriter::OnBlob(lString32 name, const lUInt8* data, int size) {
    return _document->addBlob(name, data, size);
}

void ldomDocumentWriter::OnDocProperty(const char* name, lString8 value) {
    _document->getProps()->setString(name, value);
}

void ldomDocumentWriter::OnEncoding(const lChar32*, const lChar32*) {
}

bool IS_FIRST_BODY = false;

ldomDocumentWriter::ldomDocumentWriter(ldomDocument* document, bool headerOnly)
        : _document(document)
        , _currNode(NULL)
        , _errFlag(false)
        , _headerOnly(headerOnly)
        , _popStyleOnFinish(false)
        , _flags(0)
        , _inHeadStyle(false) {
    _headStyleText.clear();
    _stylesheetLinks.clear();
    _stopTagId = 0xFFFE;
    IS_FIRST_BODY = true;
    _document->_parsing = true;

    if (_document->isDefStyleSet()) {
        _document->getRootNode()->initNodeStyle();
        _document->getRootNode()->setRendMethod(erm_block);
    }

    //CRLog::trace("ldomDocumentWriter() headerOnly=%s", _headerOnly?"true":"false");
}

ldomElementWriter* ldomDocumentWriter::pop(ldomElementWriter* obj, lUInt16 id) {
    // First check if there's an element with provided id in the stack
    //logfile << "{p";
    ldomElementWriter* tmp = obj;
    for (; tmp; tmp = tmp->_parent) {
        //logfile << "-";
        if (tmp->getElement()->getNodeId() == id)
            break;
    }
    //logfile << "1";
    if (!tmp) {
        // No element in the stack with provided id: nothing to close, stay at current element
        //logfile << "-err}";
        return obj; // error!!!
    }
    ldomElementWriter* tmp2 = NULL;
    //logfile << "2";
    for (tmp = obj; tmp; tmp = tmp2) {
        //logfile << "-";
        tmp2 = tmp->_parent;
        bool stop = (tmp->getElement()->getNodeId() == id);
        ElementCloseHandler(tmp->getElement());
        delete tmp;
        if (stop)
            return tmp2;
    }
    /*
    logfile << "3 * ";
    logfile << (int)tmp << " - " << (int)tmp2 << " | cnt=";
    logfile << (int)tmp->getElement()->childCount << " - "
            << (int)tmp2->getElement()->childCount;
    */
    //logfile << "}";
    return tmp2;
}
