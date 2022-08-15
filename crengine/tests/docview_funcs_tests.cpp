/**
 *  crengine-ng
 *  Copyright (c) 2022 Aleksey Chernov
 *
 *  \file docview_fucns_tests.cpp
 *  \brief crengine-ng unit test module.
 *  Tests various functions LVDocView class.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
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

// Fixtures

class DocViewFuncsTests: public testing::Test
{
protected:
    bool m_initOK;
    LVDocView* m_view;
    CRPropRef m_props;
protected:
    DocViewFuncsTests()
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
            m_props->setString(PROP_FONT_FACE, "FreeSerif");
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
};

// units tests

TEST_F(DocViewFuncsTests, TestGetAvgTextLineHeight) {
    CRLog::info("=================================");
    CRLog::info("Starting TestGetAvgTextLineHeight");
    ASSERT_TRUE(m_initOK);

    // set properties
    ASSERT_TRUE(setProperty(PROP_FONT_SIZE, "20"));
    ASSERT_TRUE(setProperty(PROP_INTERLINE_SPACE, "100"));

    // render document
    m_view->requestRender();
    m_view->checkRender();

    int avgTextLineHeight = m_view->getAvgTextLineHeight();
    EXPECT_GE(avgTextLineHeight, 19);
    EXPECT_LT(avgTextLineHeight, 26);

    ASSERT_TRUE(setProperty(PROP_INTERLINE_SPACE, "200"));
    avgTextLineHeight = m_view->getAvgTextLineHeight();
    EXPECT_GE(avgTextLineHeight, 39);
    EXPECT_LT(avgTextLineHeight, 50);

    ASSERT_TRUE(setProperty(PROP_INTERLINE_SPACE, "50"));
    avgTextLineHeight = m_view->getAvgTextLineHeight();
    EXPECT_GE(avgTextLineHeight, 9);
    EXPECT_LT(avgTextLineHeight, 13);

    CRLog::info("Finished TestGetAvgTextLineHeight");
    CRLog::info("=================================");
}

TEST_F(DocViewFuncsTests, TestGetFileCRC32) {
    CRLog::info("=========================");
    CRLog::info("Starting TestGetFileCRC32");
    ASSERT_TRUE(m_initOK);

    // open document (fb2)
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "hello_fb2.fb2"));
    // Retrive CRC32
    lUInt32 crc32 = m_view->getFileCRC32();
    // And test it
    EXPECT_EQ(crc32, 0x5628A889); // CRC32 for file "hello_fb2.fb2"

    // open document in archive (fb2)
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "example.fb2.zip@/example.fb2"));
    // Retrive hash
    crc32 = m_view->getFileCRC32();
    // And test it
    EXPECT_EQ(crc32, 0xEB69EC66); // CRC32 for file "example.fb2"

    // open document (epub)
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "simple-epub2.epub"));
    // Retrive hash
    crc32 = m_view->getFileCRC32();
    // And test it
    EXPECT_EQ(crc32, 0x28BF3A0E); // CRC32 for file "simple-epub2.epub"

    // open document (html)
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "mathml-test.html"));
    // Retrive hash
    crc32 = m_view->getFileCRC32();
    // And test it
    EXPECT_EQ(crc32, 0x2FC73083); // CRC32 for file "mathml-test.html"

    // open document (fb3)
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "simple-fb3.fb3"));
    // Retrive hash
    crc32 = m_view->getFileCRC32();
    // And test it
    EXPECT_EQ(crc32, 0x81E72936); // CRC32 for file "simple-fb3.fb3"

#if 0
    // Disabled due to multiple asan errors
    // open document (doc)
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "simple-doc.doc"));
    // Retrive hash
    crc32 = m_view->getFileCRC32();
    // And test it
    EXPECT_EQ(crc32, E2C0435C);   // CRC32 for file "simple-doc.doc"
#endif

    // open document (docx)
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "simple-docx.docx"));
    // Retrive hash
    crc32 = m_view->getFileCRC32();
    // And test it
    EXPECT_EQ(crc32, 0xCBC76B0F); // CRC32 for file "simple-docx.docx"

    // open document (odt)
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "simple-odt.odt"));
    // Retrive hash
    crc32 = m_view->getFileCRC32();
    // And test it
    EXPECT_EQ(crc32, 0x562871D6); // CRC32 for file "simple-odt.odt"

    CRLog::info("Finished TestGetFileCRC32");
    CRLog::info("=========================");
}

#if (USE_SHASUM == 1)
TEST_F(DocViewFuncsTests, TestGetFileHash) {
    CRLog::info("========================");
    CRLog::info("Starting TestGetFileHash");
    ASSERT_TRUE(m_initOK);

    // open document (fb2)
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "hello_fb2.fb2"));
    // Retrive hash
    lString32 hash = m_view->getFileHash();
    // And test it
    EXPECT_STREQ(LCSTR(hash), "sha256:5d1c2b66dec9b53c39409a4fe1c47b06877fdeb571252d429c24256f8805f200"); // SHA256 for file "hello_fb2.fb2"

    // open document in archive (fb2)
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "example.fb2.zip@/example.fb2"));
    // Retrive hash
    hash = m_view->getFileHash();
    // And test it
    EXPECT_STREQ(LCSTR(hash), "sha256:6d4036055786606a238359926277698369e0608bccf58babdb558e0a47b78e77"); // SHA256 for file "example.fb2"

    // open document (epub)
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "simple-epub2.epub"));
    // Retrive hash
    hash = m_view->getFileHash();
    // And test it
    EXPECT_STREQ(LCSTR(hash), "sha256:c7ece7b7a2526c42c9bcaec1f251d403d176f44ee4d0075f098adf7018c0c166"); // SHA256 for file "simple-epub2.epub"

    // open document (html)
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "mathml-test.html"));
    // Retrive hash
    hash = m_view->getFileHash();
    // And test it
    EXPECT_STREQ(LCSTR(hash), "sha256:908885e9371b7cd0308360ac948363979f731cdc2b1818c2b13857d4be0932c9"); // SHA256 for file "mathml-test.html"

    // open document (fb3)
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "simple-fb3.fb3"));
    // Retrive hash
    hash = m_view->getFileHash();
    // And test it
    EXPECT_STREQ(LCSTR(hash), "sha256:e2143bfb003a1233e28ac529084441ca53f8a3c99eae0ee478c8bb6db1a7362d"); // SHA256 for file "simple-fb3.fb3"

#if 0
    // Disabled due to multiple asan errors
    // open document (doc)
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "simple-doc.doc"));
    // Retrive hash
    hash = m_view->getFileHash();
    // And test it
    EXPECT_STREQ(LCSTR(hash), "sha256:c21020fd2b3c3232444fd40fdf9a55aa9ae22d3276e280f797fc6efe19d97b3d");   // SHA256 for file "simple-doc.doc"
#endif

    // open document (docx)
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "simple-docx.docx"));
    // Retrive hash
    hash = m_view->getFileHash();
    // And test it
    EXPECT_STREQ(LCSTR(hash), "sha256:269edf17c6d81d399ebdb95916aff3c5517801d672a1c26c628eb77301f6c6d1"); // SHA256 for file "simple-docx.docx"

    // open document (odt)
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "simple-odt.odt"));
    // Retrive hash
    hash = m_view->getFileHash();
    // And test it
    EXPECT_STREQ(LCSTR(hash), "sha256:2e81cc0fbed178bda64fb68aa63d8f0254633025f1e021253dfa05442fdbefdb"); // SHA256 for file "simple-odt.odt"

    CRLog::info("Finished TestGetFileHash");
    CRLog::info("=========================");
}
#endif
