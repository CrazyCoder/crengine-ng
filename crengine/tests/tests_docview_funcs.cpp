/***************************************************************************
 *   crengine-ng, unit testing                                             *
 *   Copyright (C) 2022,2023 Aleksey Chernov <valexlin@gmail.com>          *
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
 * \file tests_docview_funcs.cpp
 * \brief Tests various functions of the LVDocView class.
 */

#include <crlog.h>
#include <lvdocview.h>
#include <ldomdocument.h>
#include <lvrend.h>
#include <lvstreamutils.h>
#include <ldomdoccache.h>

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

    // open document in archive (fb2)
    // We specify only the archive file name, LVDocView should find and open the inner file
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "example.fb2.zip"));
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

    // open document in archive (fb2)
    // We specify only the archive file name, LVDocView should find and open the inner file
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "example.fb2.zip"));
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

TEST_F(DocViewFuncsTests, GetFB2FileProps1) {
    CRLog::info("=========================");
    CRLog::info("Starting GetFB2FileProps1");
    ASSERT_TRUE(m_initOK);

    // open document
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "hello_fb2.fb2"));

    // full file name
    lString32 filename = m_view->getFileName();
    EXPECT_STREQ(LCSTR(filename), TESTS_DATADIR "hello_fb2.fb2");

    // file size
    lvsize_t file_size = m_view->getFileSize();
    EXPECT_EQ(file_size, 876L);

    CRLog::info("Finished GetFB2FileProps1");
    CRLog::info("=========================");
}

TEST_F(DocViewFuncsTests, GetFB2FilePropsInArc1) {
    CRLog::info("==============================");
    CRLog::info("Starting GetFB2FilePropsInArc1");
    ASSERT_TRUE(m_initOK);

    // open document
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "example.fb2.zip"));

    // full file name
    lString32 filename = m_view->getFileName();
    EXPECT_STREQ(LCSTR(filename), TESTS_DATADIR "example.fb2.zip@/example.fb2");

    // file size
    lvsize_t file_size = m_view->getFileSize();
    EXPECT_EQ(file_size, 100407L);

    CRLog::info("Finished GetFB2FilePropsInArc1");
    CRLog::info("==============================");
}

TEST_F(DocViewFuncsTests, GetFB2FilePropsInArc2) {
    CRLog::info("==============================");
    CRLog::info("Starting GetFB2FilePropsInArc2");
    ASSERT_TRUE(m_initOK);

    // open document
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "example.fb2.zip@/example.fb2"));

    // full file name
    lString32 filename = m_view->getFileName();
    EXPECT_STREQ(LCSTR(filename), TESTS_DATADIR "example.fb2.zip@/example.fb2");

    // file size
    lvsize_t file_size = m_view->getFileSize();
    EXPECT_EQ(file_size, 100407L);

    CRLog::info("Finished GetFB2FilePropsInArc2");
    CRLog::info("==============================");
}

TEST_F(DocViewFuncsTests, GetEPUBFileProps1) {
    CRLog::info("==========================");
    CRLog::info("Starting GetEPUBFileProps1");
    ASSERT_TRUE(m_initOK);

    // open document
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "simple-epub2.epub"));

    // full file name
    lString32 filename = m_view->getFileName();
    EXPECT_STREQ(LCSTR(filename), TESTS_DATADIR "simple-epub2.epub");

    // file size
    lvsize_t file_size = m_view->getFileSize();
    EXPECT_EQ(file_size, 10775L);

    CRLog::info("Finished GetEPUBFileProps1");
    CRLog::info("==========================");
}

TEST_F(DocViewFuncsTests, GetEPUBFilePropsInArc1) {
    CRLog::info("===============================");
    CRLog::info("Starting GetEPUBFilePropsInArc1");
    ASSERT_TRUE(m_initOK);

    // open document
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "simple-epub2.epub.zip"));

    // full file name
    lString32 filename = m_view->getFileName();
    EXPECT_STREQ(LCSTR(filename), TESTS_DATADIR "simple-epub2.epub.zip@/simple-epub2.epub");

    // file size
    lvsize_t file_size = m_view->getFileSize();
    EXPECT_EQ(file_size, 10775L);

    CRLog::info("Finished GetEPUBFilePropsInArc1");
    CRLog::info("===============================");
}

TEST_F(DocViewFuncsTests, GetEPUBFilePropsInArc2) {
    CRLog::info("===============================");
    CRLog::info("Starting GetEPUBFilePropsInArc2");
    ASSERT_TRUE(m_initOK);

    // open document
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "simple-epub2.epub.zip@/simple-epub2.epub"));

    // full file name
    lString32 filename = m_view->getFileName();
    EXPECT_STREQ(LCSTR(filename), TESTS_DATADIR "simple-epub2.epub.zip@/simple-epub2.epub");

    // file size
    lvsize_t file_size = m_view->getFileSize();
    EXPECT_EQ(file_size, 10775L);

    CRLog::info("Finished GetEPUBFilePropsInArc2");
    CRLog::info("===============================");
}

TEST_F(DocViewFuncsTests, GetFB2StreamProps1) {
    CRLog::info("===========================");
    CRLog::info("Starting GetFB2StreamProps1");
    ASSERT_TRUE(m_initOK);

    // create memory stream
    LVStreamRef stream = LVCreateMemoryStream(lString32(TESTS_DATADIR "hello_fb2.fb2"));
    ASSERT_FALSE(stream.isNull());

    // open document
    ASSERT_TRUE(m_view->LoadDocument(stream, U"/path/to/file/hello_fb2.fb2"));

    // full file name
    lString32 filename = m_view->getFileName();
    EXPECT_STREQ(LCSTR(filename), "/path/to/file/hello_fb2.fb2");

    // file size
    lvsize_t file_size = m_view->getFileSize();
    EXPECT_EQ(file_size, 876L);

    CRLog::info("Finished GetFB2StreamProps1");
    CRLog::info("===========================");
}

TEST_F(DocViewFuncsTests, GetFB2StreamPropsInArc1) {
    CRLog::info("================================");
    CRLog::info("Starting GetFB2StreamPropsInArc1");
    ASSERT_TRUE(m_initOK);

    // create memory stream
    LVStreamRef stream = LVCreateMemoryStream(lString32(TESTS_DATADIR "example.fb2.zip"));
    ASSERT_FALSE(stream.isNull());

    // open document
    ASSERT_TRUE(m_view->LoadDocument(stream, U"/path/to/file/example.fb2.zip"));

    // full file name
    lString32 filename = m_view->getFileName();
    EXPECT_STREQ(LCSTR(filename), "/path/to/file/example.fb2.zip@/example.fb2");

    // file size
    lvsize_t file_size = m_view->getFileSize();
    EXPECT_EQ(file_size, 100407L);

    CRLog::info("Finished GetFB2StreamPropsInArc1");
    CRLog::info("================================");
}

TEST_F(DocViewFuncsTests, GetFB2FilePropsInArc1FromCache) {
    CRLog::info("==============================");
    CRLog::info("Starting GetFB2FilePropsInArc1FromCache");
    ASSERT_TRUE(m_initOK);

    // set up cache
    ASSERT_TRUE(ldomDocCache::init(cs32("./cache"), 10000000UL));

    // open document
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "example.fb2.zip@/example.fb2"));
    // build render data
    m_view->Render(1000, 1000);
    // Save to cache
    ASSERT_EQ(m_view->updateCache(), CR_DONE);
    // Close
    m_view->close();

    // Make a file copy (in current directory)
    LVStreamRef streamIn = LVOpenFileStream(TESTS_DATADIR "example.fb2.zip", LVOM_READ);
    ASSERT_FALSE(streamIn.isNull());
    LVStreamRef streamOut = LVOpenFileStream("tmp.fb2.zip", LVOM_WRITE);
    ASSERT_FALSE(streamOut.isNull());
    ASSERT_EQ(LVPumpStream(streamOut, streamIn), 60828);
    streamIn.Clear();
    streamOut.Clear();

    // open the copied document using the cache of the original document
    ASSERT_TRUE(m_view->LoadDocument("tmp.fb2.zip"));
    ASSERT_TRUE(m_view->isOpenFromCache());

    // full file name
    lString32 filename = m_view->getFileName();
    EXPECT_STREQ(LCSTR(filename), "tmp.fb2.zip@/example.fb2");

    // file size
    lvsize_t file_size = m_view->getFileSize();
    EXPECT_EQ(file_size, 100407L);

    ldomDocCache::clear();
    ldomDocCache::close();

    // Delete a copy of a file
    LVDeleteFile(cs32("tmp.fb2.zip"));

    CRLog::info("Finished GetFB2FilePropsInArc1FromCache");
    CRLog::info("=======================================");
}

TEST_F(DocViewFuncsTests, GetFB2FilePropsInArc2FromCache) {
    CRLog::info("==============================");
    CRLog::info("Starting GetFB2FilePropsInArc2FromCache");
    ASSERT_TRUE(m_initOK);

    // set up cache
    ASSERT_TRUE(ldomDocCache::init(cs32("./cache"), 10000000UL));

    // open document
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "example.fb2.zip@/example.fb2"));
    // build render data
    m_view->Render(1000, 1000);
    // Save to cache
    ASSERT_EQ(m_view->updateCache(), CR_DONE);
    // Close
    m_view->close();

    // Make a file copy (in current directory)
    LVStreamRef streamIn = LVOpenFileStream(TESTS_DATADIR "example.fb2.zip", LVOM_READ);
    ASSERT_FALSE(streamIn.isNull());
    LVStreamRef streamOut = LVOpenFileStream("tmp.fb2.zip", LVOM_WRITE);
    ASSERT_FALSE(streamOut.isNull());
    ASSERT_EQ(LVPumpStream(streamOut, streamIn), 60828);
    streamIn.Clear();
    streamOut.Clear();

    // open the copied document using the cache of the original document
    ASSERT_TRUE(m_view->LoadDocument("tmp.fb2.zip@/example.fb2"));
    ASSERT_TRUE(m_view->isOpenFromCache());

    // full file name
    lString32 filename = m_view->getFileName();
    EXPECT_STREQ(LCSTR(filename), "tmp.fb2.zip@/example.fb2");

    // file size
    lvsize_t file_size = m_view->getFileSize();
    EXPECT_EQ(file_size, 100407L);

    ldomDocCache::clear();
    ldomDocCache::close();

    // Delete a copy of a file
    LVDeleteFile(cs32("tmp.fb2.zip"));

    CRLog::info("Finished GetFB2FilePropsInArc2FromCache");
    CRLog::info("=======================================");
}
