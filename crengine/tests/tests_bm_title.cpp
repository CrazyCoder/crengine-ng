/***************************************************************************
 *   crengine-ng, unit testing                                             *
 *   Copyright (C) 2023 Aleksey Chernov <valexlin@gmail.com>               *
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

/**
 * \file tests_bm_title.cpp
 * \brief Testing getting the header for an arbitrary bookmark.
 */

// uncomment to dump test document in xml
//#define DEBUG_DUMP_XML 1

#include <crlog.h>
#include <lvdocview.h>
#include <ldomdocument.h>
#include <lvrend.h>

#if DEBUG_DUMP_XML
#include <lvstreamutils.h>
#include "../src/lvtinydom/writenodeex.h"
#endif

#include "gtest/gtest.h"

#ifndef TESTS_DATADIR
#error Please define TESTS_DATADIR, which points to the directory with the data files for the tests
#endif

// Fixtures

class BookmarkGetTitleTests: public testing::Test
{
protected:
    bool m_initOK;
    LVDocView* m_view;
    CRPropRef m_props;
protected:
    BookmarkGetTitleTests()
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
            m_props->setInt(PROP_REQUESTED_DOM_VERSION, DOM_VERSION_CURRENT);
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
#if DEBUG_DUMP_XML
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
#endif
};

// units tests

TEST_F(BookmarkGetTitleTests, GetBMTitleInFB2) {
    CRLog::info("========================");
    CRLog::info("Starting GetBMTitleInFB2");
    ASSERT_TRUE(m_initOK);

    // open document
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "structured-doc.fb2")); // load document
    ASSERT_TRUE(setCSS("fb2.css"));

#if DEBUG_DUMP_XML
    ASSERT_TRUE(dumpXML("structured-doc-fb2-dump.xml"));
#endif

    // regular text (in paragraph)
    ldomXPointer xptr = m_view->getDocument()->createXPointer(U"/FictionBook/body/section[2]/section[1]/p[4]/text().0");
    EXPECT_FALSE(xptr.isNull());

    lString32 titleText;
    lString32 posText;
    // Get title & text at pointer
    EXPECT_TRUE(m_view->getBookmarkPosText(xptr, titleText, posText));

    // compare with reference
    EXPECT_STREQ(UnicodeToUtf8(titleText).c_str(), "Chapter II. Chapter II.a");
    EXPECT_STREQ(UnicodeToUtf8(posText).c_str(), "Key target text!");

    // (in title)
    xptr = m_view->getDocument()->createXPointer(U"/FictionBook/body/section[2]/section[1]/title/p/text().0");
    EXPECT_FALSE(xptr.isNull());

    // Get title & text at pointer
    EXPECT_TRUE(m_view->getBookmarkPosText(xptr, titleText, posText));

    // compare with reference
    EXPECT_STREQ(UnicodeToUtf8(titleText).c_str(), "Chapter II. Chapter II.a");
    EXPECT_STREQ(UnicodeToUtf8(posText).c_str(), "Paragraph 2.1.");

    CRLog::info("Finished GetBMTitleInFB2");
    CRLog::info("========================");
}

TEST_F(BookmarkGetTitleTests, GetBMTitleInEPUB) {
    CRLog::info("=========================");
    CRLog::info("Starting GetBMTitleInEPUB");
    ASSERT_TRUE(m_initOK);

    // open document
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "structured-doc.epub")); // load document
    ASSERT_TRUE(setCSS("epub.css"));

#if DEBUG_DUMP_XML
    ASSERT_TRUE(dumpXML("structured-doc-epub-dump.xml"));
#endif

    // 1. xptr - regular text (in paragraph)
    ldomXPointer xptr = m_view->getDocument()->createXPointer(U"/body/DocFragment[2]/body/p[3]/text().0");
    EXPECT_FALSE(xptr.isNull());

    lString32 titleText;
    lString32 posText;
    // Get title & text at pointer
    EXPECT_TRUE(m_view->getBookmarkPosText(xptr, titleText, posText));

    // compare with reference
    EXPECT_STREQ(UnicodeToUtf8(titleText).c_str(), "Chapter II. Chapter II.a");
    EXPECT_STREQ(UnicodeToUtf8(posText).c_str(), "Key target text!");

    // 2. xptr - regular text (in paragraph)
    xptr = m_view->getDocument()->createXPointer(U"/body/DocFragment[2]/body/p[9]/text().0");
    EXPECT_FALSE(xptr.isNull());

    // Get title & text at pointer
    EXPECT_TRUE(m_view->getBookmarkPosText(xptr, titleText, posText));

    // compare with reference
    EXPECT_STREQ(UnicodeToUtf8(titleText).c_str(), "Chapter II. Chapter II.b");
    EXPECT_STREQ(UnicodeToUtf8(posText).c_str(), "Key target text (2)!");

    // 3. xptr - regular text (in paragraph), complex header (contains child elements)
    xptr = m_view->getDocument()->createXPointer(U"/body/DocFragment[3]/body/p[2]/text().0");
    EXPECT_FALSE(xptr.isNull());

    // Get title & text at pointer
    EXPECT_TRUE(m_view->getBookmarkPosText(xptr, titleText, posText));

    // compare with reference
    EXPECT_STREQ(UnicodeToUtf8(titleText).c_str(), "CHAPTER III. Some Chapter Name");
    EXPECT_STREQ(UnicodeToUtf8(posText).c_str(), "Key target text!");

    // 4. xptr - regular text (in paragraph), complex header (header is inside another element)
    xptr = m_view->getDocument()->createXPointer(U"/body/DocFragment[4]/body/p[2]/text().0");
    EXPECT_FALSE(xptr.isNull());

    // Get title & text at pointer
    EXPECT_TRUE(m_view->getBookmarkPosText(xptr, titleText, posText));

    // compare with reference
    EXPECT_STREQ(UnicodeToUtf8(titleText).c_str(), "CHAPTER IV. Some Chapter Name");
    EXPECT_STREQ(UnicodeToUtf8(posText).c_str(), "Key target text!");

    // 5. xptr - in title
    xptr = m_view->getDocument()->createXPointer(U"/body/DocFragment[2]/body/h3[2]/text().0");
    EXPECT_FALSE(xptr.isNull());

    // Get title & text at pointer
    EXPECT_TRUE(m_view->getBookmarkPosText(xptr, titleText, posText));

    // compare with reference
    EXPECT_STREQ(UnicodeToUtf8(titleText).c_str(), "Chapter II. Chapter II.b");
    EXPECT_STREQ(UnicodeToUtf8(posText).c_str(), "Paragraph 2.2.");

    CRLog::info("Finished GetBMTitleInEPUB");
    CRLog::info("=========================");
}
