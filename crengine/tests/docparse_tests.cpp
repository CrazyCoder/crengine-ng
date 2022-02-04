/**
 *  crengine-ng
 *  Copyright (c) 2022 Aleksey Chernov
 *
 *  \file docparse_tests.cpp
 *  \brief crengine-ng unit test module.
 *  Tests document parsing.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <crlog.h>
#include <lvdocview.h>
#include <ldomdocument.h>
#include <lvrend.h>
#include <lvstreamutils.h>

#include "gtest/gtest.h"

#include "compare-two-textfiles.h"

#ifndef TESTS_DATADIR
#error Please define TESTS_DATADIR, which points to the directory with the data files for the tests
#endif

#ifndef DOCPARSE_REFERENCE_DIR
#error Please define RENDER_REFERENCE_DIR, which points to the directory with the render reference files
#endif

// Flags for writeNodeEx():
// Copied from lvtinydom.cpp
#define WRITENODEEX_TEXT_HYPHENATE              0x0001 ///< add soft-hyphens where hyphenation is allowed
#define WRITENODEEX_TEXT_MARK_NODE_BOUNDARIES   0x0002 ///< mark start and end of text nodes (useful when indented)
#define WRITENODEEX_TEXT_SHOW_UNICODE_CODEPOINT 0x0004 ///< show unicode codepoint after char
#define WRITENODEEX_TEXT_UNESCAPED              0x0008 ///< let &, < and > unescaped in text nodes (makes HTML invalid)
#define WRITENODEEX_INDENT_NEWLINE              0x0010 ///< indent newlines according to node level
#define WRITENODEEX_NEWLINE_BLOCK_NODES         0x0020 ///< start only nodes rendered as block/final on a new line, \ \
        ///  so inline elements and text nodes are stuck together
#define WRITENODEEX_NEWLINE_ALL_NODES           0x0040 ///< start all nodes on a new line
#define WRITENODEEX_UNUSED_1                    0x0080 ///<
#define WRITENODEEX_NB_SKIPPED_CHARS            0x0100 ///< show number of skipped chars in text nodes: (...43...)
#define WRITENODEEX_NB_SKIPPED_NODES            0x0200 ///< show number of skipped sibling nodes: [...17...]
#define WRITENODEEX_SHOW_REND_METHOD            0x0400 ///< show rendering method at end of tag (<div ~F> =Final, <b ~i>=Inline...)
#define WRITENODEEX_SHOW_MISC_INFO              0x0800 ///< show additional info (depend on context)
#define WRITENODEEX_ADD_UPPER_DIR_LANG_ATTR     0x1000 ///< add dir= and lang= grabbed from upper nodes
#define WRITENODEEX_GET_CSS_FILES               0x2000 ///< ensure css files that apply to initial node are returned \ \
        ///  in &cssFiles (needed when not starting from root node)
#define WRITENODEEX_INCLUDE_STYLESHEET_ELEMENT  0x4000 ///< includes crengine <stylesheet> element in HTML \ \
        ///  (not done if outside of sub-tree)
#define WRITENODEEX_COMPUTED_STYLES_AS_ATTR     0x8000 ///< set style='' from computed styles (not implemented)

// Fixtures

class DocParseTests: public testing::Test
{
protected:
    bool m_initOK;
    LVDocView* m_view;
    CRPropRef m_props;
protected:
    DocParseTests()
            : testing::Test() {
        m_view = NULL;
    }
    virtual void SetUp() override {
        lString8 css;
        m_initOK = LVLoadStylesheetFile(Utf8ToUnicode(CSS_DIR "fb2.css"), css);
        if (m_initOK) {
            m_view = new LVDocView(32, false);
            m_view->setStyleSheet(css);
            m_view->setMinFontSize(8);
            m_view->setMaxFontSize(320);
            m_props = LVCreatePropsContainer();
            m_props->setInt(PROP_REQUESTED_DOM_VERSION, 20200824);
            m_props->setInt(PROP_RENDER_BLOCK_RENDERING_FLAGS, BLOCK_RENDERING_FLAGS_WEB);
            CRPropRef unknown = m_view->propsApply(m_props);
            m_initOK = unknown->getCount() == 0;
            m_view->Resize(640, 360);
        }
    }

    virtual void TearDown() override {
        if (m_view) {
            delete m_view;
            m_view = 0;
        }
    }

    bool setCSS(const char* cssfile) {
        lString8 csscont;
        lString8 fname = cs8(CSS_DIR);
        fname += cssfile;
        bool res = LVLoadStylesheetFile(Utf8ToUnicode(fname), csscont);
        if (res)
            m_view->setStyleSheet(csscont);
        return res;
    }

    bool setProperty(const char* propName, const char* value) {
        m_props->setString(propName, value);
        CRPropRef unknown = m_view->propsApply(m_props);
        return unknown->getCount() == 0;
    }

    bool setProperty(const char* propName, int value) {
        m_props->setInt(propName, value);
        CRPropRef unknown = m_view->propsApply(m_props);
        return unknown->getCount() == 0;
    }

    bool dumpXML(const char* fname) {
        ldomDocument* doc = m_view->getDocument();
        if (NULL == doc)
            return false;
        LVStreamRef ostream = LVOpenFileStream(fname, LVOM_WRITE);
        if (ostream.isNull())
            return false;
        ldomNode* rootNode = doc->getRootNode();
        if (NULL == rootNode)
            return false;
        for (int i = 0; i < rootNode->getChildCount(); i++) {
            ldomXPointer pointer = ldomXPointer(rootNode->getChildNode(i), 0);
            lString8 dump = pointer.getHtml(WRITENODEEX_INDENT_NEWLINE |
                                            WRITENODEEX_NEWLINE_ALL_NODES |
                                            WRITENODEEX_INCLUDE_STYLESHEET_ELEMENT);
            *ostream << dump;
        }
        return true;
    }
};

// units tests

TEST_F(DocParseTests, ParseSimpleFB2) {
    CRLog::info("=======================");
    CRLog::info("Starting ParseSimpleFB2");
    ASSERT_TRUE(m_initOK);

    // set properties
    //ASSERT_TRUE(setProperty(PROP_REQUESTED_DOM_VERSION, "0"));
    //ASSERT_TRUE(setProperty(PROP_RENDER_BLOCK_RENDERING_FLAGS, "0"));

    // open document & render into drawbuf
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "hello_fb2.fb2")); // load document
    ASSERT_TRUE(setCSS("fb2.css"));

    // render document
    m_view->requestRender();
    m_view->checkRender();

    // Save dump
    ASSERT_TRUE(dumpXML("doc-dump.xml"));

    // compare with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoTextFiles("doc-dump.xml", DOCPARSE_REFERENCE_DIR "hello_fb2_dump.xml.gz"));

    CRLog::info("Finished ParseSimpleFB2");
    CRLog::info("=======================");
}

TEST_F(DocParseTests, ParseHTMLAutoboxingLegacy) {
    CRLog::info("==============================");
    CRLog::info("Starting ParseAutoboxingLegacy");
    ASSERT_TRUE(m_initOK);

    // set properties
    ASSERT_TRUE(setProperty(PROP_REQUESTED_DOM_VERSION, 0));
    ASSERT_TRUE(setProperty(PROP_RENDER_BLOCK_RENDERING_FLAGS, 0));

    // open document & render into drawbuf
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "autoboxing-1.html")); // load document
    ASSERT_TRUE(setCSS("htm.css"));

    // render document
    m_view->requestRender();
    m_view->checkRender();

    // Save dump
    ASSERT_TRUE(dumpXML("doc-dump.xml"));

    // compare with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoTextFiles("doc-dump.xml", DOCPARSE_REFERENCE_DIR "autoboxing-1_legacy_dump.xml.gz"));

    CRLog::info("Finished ParseAutoboxingLegacy");
    CRLog::info("==============================");
}

TEST_F(DocParseTests, ParseHTMLAutoboxingEnhanced) {
    CRLog::info("================================");
    CRLog::info("Starting ParseAutoboxingEnhanced");
    ASSERT_TRUE(m_initOK);

    // set properties
    ASSERT_TRUE(setProperty(PROP_REQUESTED_DOM_VERSION, 20200824));
    ASSERT_TRUE(setProperty(PROP_RENDER_BLOCK_RENDERING_FLAGS, BLOCK_RENDERING_FLAGS_WEB));

    // open document & render into drawbuf
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "autoboxing-1.html")); // load document
    ASSERT_TRUE(setCSS("htm.css"));

    // render document
    m_view->requestRender();
    m_view->checkRender();

    // Save dump
    ASSERT_TRUE(dumpXML("doc-dump.xml"));

    // compare with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoTextFiles("doc-dump.xml", DOCPARSE_REFERENCE_DIR "autoboxing-1_enhanced_dump.xml.gz"));

    CRLog::info("Finished ParseAutoboxingEnhanced");
    CRLog::info("================================");
}

TEST_F(DocParseTests, ParseHTMLRubyElementsLegacy) {
    CRLog::info("================================");
    CRLog::info("Starting ParseRubyElementsLegacy");
    ASSERT_TRUE(m_initOK);

    // set properties
    ASSERT_TRUE(setProperty(PROP_REQUESTED_DOM_VERSION, 0));
    ASSERT_TRUE(setProperty(PROP_RENDER_BLOCK_RENDERING_FLAGS, 0));

    // open document & render into drawbuf
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "ruby1.html")); // load document
    ASSERT_TRUE(setCSS("htm.css"));

    // render document
    m_view->requestRender();
    m_view->checkRender();

    // Save dump
    ASSERT_TRUE(dumpXML("doc-dump.xml"));

    // compare with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoTextFiles("doc-dump.xml", DOCPARSE_REFERENCE_DIR "ruby1_legacy_dump.xml.gz"));

    CRLog::info("Finished ParseRubyElementsLegacy");
    CRLog::info("================================");
}

TEST_F(DocParseTests, ParseHTMLRubyElementsEnhanced) {
    CRLog::info("==================================");
    CRLog::info("Starting ParseRubyElementsEnhanced");
    ASSERT_TRUE(m_initOK);

    // set properties
    ASSERT_TRUE(setProperty(PROP_REQUESTED_DOM_VERSION, 20200824));
    ASSERT_TRUE(setProperty(PROP_RENDER_BLOCK_RENDERING_FLAGS, BLOCK_RENDERING_FLAGS_WEB));

    // open document & render into drawbuf
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "ruby1.html")); // load document
    ASSERT_TRUE(setCSS("htm.css"));

    // render document
    m_view->requestRender();
    m_view->checkRender();

    // Save dump
    ASSERT_TRUE(dumpXML("doc-dump.xml"));

    // compare with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoTextFiles("doc-dump.xml", DOCPARSE_REFERENCE_DIR "ruby1_enhanced_dump.xml.gz"));

    CRLog::info("Finished ParseRubyElementsEnhanced");
    CRLog::info("==================================");
}

TEST_F(DocParseTests, ParseSimpleODT) {
    // KNOWN ISSUES:
    //   Not implemented:
    //    * text color attribure
    //    * text background color attribure
    //   To pass this test these attributes are omitted from the reference XML file.
    //   When these features are implemented, the reference file should be updated.

    CRLog::info("==========================");
    CRLog::info("Starting ParseSimpleODTDoc");
    ASSERT_TRUE(m_initOK);

    // open document & render into drawbuf
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "simple-odt.odt")); // load document
    ASSERT_TRUE(setCSS("docx.css"));

    // render document
    m_view->requestRender();
    m_view->checkRender();

    // Save dump
    ASSERT_TRUE(dumpXML("doc-dump.xml"));

    // compare with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoTextFiles("doc-dump.xml", DOCPARSE_REFERENCE_DIR "simple-odt_dump.xml.gz"));

    CRLog::info("Finished ParseSimpleODTDoc");
    CRLog::info("==========================");
}

TEST_F(DocParseTests, ParseSimpleDOCX) {
    // KNOWN ISSUES:
    //   Not implemented:
    //    * text color attribure
    //    * text background color attribure
    //   To pass this test these attributes are omitted from the reference XML file.
    //   When these features are implemented, the reference file should be updated.

    CRLog::info("===========================");
    CRLog::info("Starting ParseSimpleDOCXDoc");
    ASSERT_TRUE(m_initOK);

    // open document & render into drawbuf
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "simple-docx.docx")); // load document
    ASSERT_TRUE(setCSS("docx.css"));

    // render document
    m_view->requestRender();
    m_view->checkRender();

    // Save dump
    ASSERT_TRUE(dumpXML("doc-dump.xml"));

    // compare with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoTextFiles("doc-dump.xml", DOCPARSE_REFERENCE_DIR "simple-docx_dump.xml.gz"));

    CRLog::info("Finished ParseSimpleDOCXDoc");
    CRLog::info("===========================");
}

TEST_F(DocParseTests, DISABLED_ParseSimpleDOC) {
    // This test is disabled because significant flaws were found.
    // * markered vs decimal list items
    // * underline text property
    // * crossedout text property
    // * upper index, lower index property
    // * hyperlinks
    // * text color attribure
    // * text background color attribure
    // TOOD: fix this...
    CRLog::info("==========================");
    CRLog::info("Starting ParseSimpleDOCDoc");
    ASSERT_TRUE(m_initOK);

    // open document & render into drawbuf
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "simple-doc.doc")); // load document
    ASSERT_TRUE(setCSS("doc.css"));

    // render document
    m_view->requestRender();
    m_view->checkRender();

    // Save dump
    ASSERT_TRUE(dumpXML("doc-dump.xml"));

    // compare with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoTextFiles("doc-dump.xml", DOCPARSE_REFERENCE_DIR "simple-doc_dump.xml.gz"));

    CRLog::info("Finished ParseSimpleDOCDoc");
    CRLog::info("==========================");
}

TEST_F(DocParseTests, ParseSimpleFB3) {
    CRLog::info("=======================");
    CRLog::info("Starting ParseSimpleFB3");
    ASSERT_TRUE(m_initOK);

    // open document & render into drawbuf
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "simple-fb3.fb3")); // load document
    ASSERT_TRUE(setCSS("docx.css"));

    // render document
    m_view->requestRender();
    m_view->checkRender();

    // Save dump
    ASSERT_TRUE(dumpXML("doc-dump.xml"));

    // compare with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoTextFiles("doc-dump.xml", DOCPARSE_REFERENCE_DIR "simple-fb3_dump.xml.gz"));

    CRLog::info("Finished ParseSimpleFB3");
    CRLog::info("=======================");
}
