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
 * \file tests_doc_props.cpp
 * \brief Testing the retrieving of various document properties.
 */

#include <crlog.h>
#include <lvdocview.h>
#include <lvstreamutils.h>
#include <ldomdoccache.h>

#include "gtest/gtest.h"

#ifndef TESTS_DATADIR
#error Please define TESTS_DATADIR, which points to the directory with the data files for the tests
#endif

// units tests

TEST(DocPropsTests, GetDocAuthorsOneInFB2) {
    CRLog::info("==============================");
    CRLog::info("Starting GetDocAuthorsOneInFB2");

    // open document
    LVDocView* view = new LVDocView(32, false);
    ASSERT_TRUE(view->LoadDocument(TESTS_DATADIR "testprops-one-author.fb2"));

    // Get author(s)
    lString32 authors = view->getAuthors();

    EXPECT_STREQ(LCSTR(authors), "First Author");

    delete view;

    CRLog::info("Finished GetDocAuthorsOneInFB2");
    CRLog::info("==============================");
}

TEST(DocPropsTests, GetDocAuthorsTwoInFB2) {
    CRLog::info("==============================");
    CRLog::info("Starting GetDocAuthorsTwoInFB2");

    // open document
    LVDocView* view = new LVDocView(32, false);
    ASSERT_TRUE(view->LoadDocument(TESTS_DATADIR "testprops-two-authors.fb2"));

    // Get author(s)
    lString32 authors = view->getAuthors();

    EXPECT_STREQ(LCSTR(authors), "First Author, Second G. Author");

    delete view;

    CRLog::info("Finished GetDocAuthorsTwoInFB2");
    CRLog::info("==============================");
}

TEST(DocPropsTests, GetDocGenresOneInFB2) {
    CRLog::info("=============================");
    CRLog::info("Starting GetDocGenresOneInFB2");

    // open document
    LVDocView* view = new LVDocView(32, false);
    ASSERT_TRUE(view->LoadDocument(TESTS_DATADIR "testprops-one-genre.fb2"));

    // Get genres
    lString32 genres = view->getKeywords();

    EXPECT_STREQ(LCSTR(genres), "comp_programming");

    delete view;

    CRLog::info("Finished GetDocGenresOneInFB2");
    CRLog::info("=============================");
}

TEST(DocPropsTests, GetDocGenresTwoInFB2) {
    CRLog::info("=============================");
    CRLog::info("Starting GetDocGenresTwoInFB2");

    // open document
    LVDocView* view = new LVDocView(32, false);
    ASSERT_TRUE(view->LoadDocument(TESTS_DATADIR "testprops-two-genres.fb2"));

    // Get genres
    lString32 genres = view->getKeywords();

    EXPECT_STREQ(LCSTR(genres), "comp_programming, comp_soft");

    delete view;

    CRLog::info("Finished GetDocGenresTwoInFB2");
    CRLog::info("=============================");
}

TEST(DocPropsTests, GetDocAuthorsOneInEPUB) {
    CRLog::info("===============================");
    CRLog::info("Starting GetDocAuthorsOneInEPUB");

    // open document
    LVDocView* view = new LVDocView(32, false);
    ASSERT_TRUE(view->LoadDocument(TESTS_DATADIR "testprops-one-author.epub"));

    // Get author(s)
    lString32 authors = view->getAuthors();

    EXPECT_STREQ(LCSTR(authors), "First Author");

    delete view;

    CRLog::info("Finished GetDocAuthorsOneInEPUB");
    CRLog::info("===============================");
}

TEST(DocPropsTests, GetDocAuthorsTwoInEPUB) {
    CRLog::info("===============================");
    CRLog::info("Starting GetDocAuthorsTwoInEPUB");

    // open document
    LVDocView* view = new LVDocView(32, false);
    ASSERT_TRUE(view->LoadDocument(TESTS_DATADIR "testprops-two-authors.epub"));

    // Get author(s)
    lString32 authors = view->getAuthors();

    EXPECT_STREQ(LCSTR(authors), "First Author, Second Good Author");

    delete view;

    CRLog::info("Finished GetDocAuthorsTwoInEPUB");
    CRLog::info("===============================");
}

TEST(DocPropsTests, GetDocKeywordsOneInEPUB) {
    CRLog::info("================================");
    CRLog::info("Starting GetDocKeywordsOneInEPUB");

    // open document
    LVDocView* view = new LVDocView(32, false);
    ASSERT_TRUE(view->LoadDocument(TESTS_DATADIR "testprops-one-keyword.epub"));

    // Get keywords
    lString32 keywords = view->getKeywords();

    EXPECT_STREQ(LCSTR(keywords), "programming");

    delete view;

    CRLog::info("Finished GetDocKeywordsOneInEPUB");
    CRLog::info("================================");
}

TEST(DocPropsTests, GetDocKeywordsTwoInEPUB) {
    CRLog::info("================================");
    CRLog::info("Starting GetDocKeywordsTwoInEPUB");

    // open document
    LVDocView* view = new LVDocView(32, false);
    ASSERT_TRUE(view->LoadDocument(TESTS_DATADIR "testprops-two-keywords.epub"));

    // Get keywords
    lString32 keywords = view->getKeywords();

    EXPECT_STREQ(LCSTR(keywords), "programming, soft");

    delete view;

    CRLog::info("Finished GetDocKeywordsTwoInEPUB");
    CRLog::info("================================");
}

TEST(DocPropsTests, GetFB2FileProps1) {
    CRLog::info("=========================");
    CRLog::info("Starting GetFB2FileProps1");

    // open document
    LVDocView* view = new LVDocView(32, false);
    ASSERT_TRUE(view->LoadDocument(TESTS_DATADIR "hello_fb2.fb2"));

    CRPropRef doc_props = view->getDocProps();
    ASSERT_FALSE(doc_props.isNull());

    // archive name
    lString32 arcname = doc_props->getStringDef(DOC_PROP_ARC_NAME, "");
    EXPECT_STREQ(LCSTR(arcname), "");

    // archive path
    lString32 arcpath = doc_props->getStringDef(DOC_PROP_ARC_PATH, "");
    EXPECT_STREQ(LCSTR(arcpath), "");

    // archive file size
    lString32 s_arc_size = doc_props->getStringDef(DOC_PROP_ARC_SIZE, "");
    EXPECT_STREQ(LCSTR(s_arc_size), "");

    // path to file
    lString32 path = doc_props->getStringDef(DOC_PROP_FILE_PATH, "");
    EXPECT_STREQ(LCSTR(path), TESTS_DATADIR);

    // file name
    lString32 filename = doc_props->getStringDef(DOC_PROP_FILE_NAME, "");
    EXPECT_STREQ(LCSTR(filename), "hello_fb2.fb2");

    // file size
    lString32 s_file_size = doc_props->getStringDef(DOC_PROP_FILE_SIZE, "");
    lInt64 file_size = -1;
    ASSERT_TRUE(s_file_size.atoi(file_size));
    EXPECT_EQ(file_size, 876L);

    delete view;

    CRLog::info("Finished GetFB2FileProps1");
    CRLog::info("=========================");
}

TEST(DocPropsTests, GetFB2FilePropsInArc1) {
    CRLog::info("==============================");
    CRLog::info("Starting GetFB2FilePropsInArc1");

    // open document
    LVDocView* view = new LVDocView(32, false);
    ASSERT_TRUE(view->LoadDocument(TESTS_DATADIR "example.fb2.zip"));

    CRPropRef doc_props = view->getDocProps();
    ASSERT_FALSE(doc_props.isNull());

    // archive name
    lString32 arcname = doc_props->getStringDef(DOC_PROP_ARC_NAME, "");
    EXPECT_STREQ(LCSTR(arcname), "example.fb2.zip");

    // archive path
    lString32 arcpath = doc_props->getStringDef(DOC_PROP_ARC_PATH, "");
    EXPECT_STREQ(LCSTR(arcpath), TESTS_DATADIR);

    // archive file size
    lString32 s_arc_size = doc_props->getStringDef(DOC_PROP_ARC_SIZE, "");
    lInt64 arc_size = -1;
    ASSERT_TRUE(s_arc_size.atoi(arc_size));
    EXPECT_EQ(arc_size, 60828L);

    // path to file
    lString32 path = doc_props->getStringDef(DOC_PROP_FILE_PATH, "");
    EXPECT_STREQ(LCSTR(path), "");

    // file name
    lString32 filename = doc_props->getStringDef(DOC_PROP_FILE_NAME, "");
    EXPECT_STREQ(LCSTR(filename), "example.fb2");

    // file size
    lString32 s_file_size = doc_props->getStringDef(DOC_PROP_FILE_SIZE, "");
    lInt64 file_size = -1;
    ASSERT_TRUE(s_file_size.atoi(file_size));
    EXPECT_EQ(file_size, 100407L);

    // compressed file size
    lString32 s_file_packsize = doc_props->getStringDef(DOC_PROP_FILE_PACK_SIZE, "");
    lInt64 file_packsize = -1;
    ASSERT_TRUE(s_file_packsize.atoi(file_packsize));
    EXPECT_EQ(file_packsize, 60656L);

    delete view;

    CRLog::info("Finished GetFB2FilePropsInArc1");
    CRLog::info("==============================");
}

TEST(DocPropsTests, GetFB2FilePropsInArc2) {
    CRLog::info("==============================");
    CRLog::info("Starting GetFB2FilePropsInArc2");

    // open document
    LVDocView* view = new LVDocView(32, false);
    ASSERT_TRUE(view->LoadDocument(TESTS_DATADIR "example.fb2.zip@/example.fb2"));

    CRPropRef doc_props = view->getDocProps();
    ASSERT_FALSE(doc_props.isNull());

    // archive name
    lString32 arcname = doc_props->getStringDef(DOC_PROP_ARC_NAME, "");
    EXPECT_STREQ(LCSTR(arcname), "example.fb2.zip");

    // archive path
    lString32 arcpath = doc_props->getStringDef(DOC_PROP_ARC_PATH, "");
    EXPECT_STREQ(LCSTR(arcpath), TESTS_DATADIR);

    // archive file size
    lString32 s_arc_size = doc_props->getStringDef(DOC_PROP_ARC_SIZE, "");
    lInt64 arc_size = -1;
    ASSERT_TRUE(s_arc_size.atoi(arc_size));
    EXPECT_EQ(arc_size, 60828L);

    // path to file
    lString32 path = doc_props->getStringDef(DOC_PROP_FILE_PATH, "");
    EXPECT_STREQ(LCSTR(path), "");

    // file name
    lString32 filename = doc_props->getStringDef(DOC_PROP_FILE_NAME, "");
    EXPECT_STREQ(LCSTR(filename), "example.fb2");

    // file size
    lString32 s_file_size = doc_props->getStringDef(DOC_PROP_FILE_SIZE, "");
    lInt64 file_size = -1;
    ASSERT_TRUE(s_file_size.atoi(file_size));
    EXPECT_EQ(file_size, 100407L);

    // compressed file size
    lString32 s_file_packsize = doc_props->getStringDef(DOC_PROP_FILE_PACK_SIZE, "");
    lInt64 file_packsize = -1;
    ASSERT_TRUE(s_file_packsize.atoi(file_packsize));
    EXPECT_EQ(file_packsize, 60656L);

    delete view;

    CRLog::info("Finished GetFB2FilePropsInArc2");
    CRLog::info("==============================");
}

TEST(DocPropsTests, GetFB2FilePropsInArc1FromCache) {
    CRLog::info("=======================================");
    CRLog::info("Starting GetFB2FilePropsInArc1FromCache");

    // set up cache
    ASSERT_TRUE(ldomDocCache::init(cs32("./cache"), 10000000UL));

    LVDocView* view = new LVDocView(32, false);
    // open document
    ASSERT_TRUE(view->LoadDocument(TESTS_DATADIR "example.fb2.zip@/example.fb2"));
    // build render data
    view->Render(1000, 1000);
    // Save to cache
    ASSERT_EQ(view->updateCache(), CR_DONE);
    // Close
    view->close();
    delete view;

    // Make a file copy (in current directory)
    LVStreamRef streamIn = LVOpenFileStream(TESTS_DATADIR "example.fb2.zip", LVOM_READ);
    ASSERT_FALSE(streamIn.isNull());
    LVStreamRef streamOut = LVOpenFileStream("tmp.fb2.zip", LVOM_WRITE);
    ASSERT_FALSE(streamOut.isNull());
    ASSERT_EQ(LVPumpStream(streamOut, streamIn), 60828);
    streamIn.Clear();
    streamOut.Clear();

    // open the copied document using the cache of the original document
    view = new LVDocView(32, false);
    ASSERT_TRUE(view->LoadDocument("tmp.fb2.zip"));
    ASSERT_TRUE(view->isOpenFromCache());

    CRPropRef doc_props = view->getDocProps();
    ASSERT_FALSE(doc_props.isNull());

    // archive name
    lString32 arcname = doc_props->getStringDef(DOC_PROP_ARC_NAME, "");
    EXPECT_STREQ(LCSTR(arcname), "tmp.fb2.zip");

    // archive path
    lString32 arcpath = doc_props->getStringDef(DOC_PROP_ARC_PATH, "");
    lString32 currDir = LVGetCurrentDirectory();
    LVReplacePathSeparator(currDir, U'/');
    LVReplacePathSeparator(arcpath, U'/');
    EXPECT_STREQ(LCSTR(arcpath), LCSTR(currDir));

    // archive file size
    lString32 s_arc_size = doc_props->getStringDef(DOC_PROP_ARC_SIZE, "");
    lInt64 arc_size = -1;
    ASSERT_TRUE(s_arc_size.atoi(arc_size));
    EXPECT_EQ(arc_size, 60828L);

    // path to file
    lString32 path = doc_props->getStringDef(DOC_PROP_FILE_PATH, "");
    EXPECT_STREQ(LCSTR(path), "");

    // file name
    lString32 filename = doc_props->getStringDef(DOC_PROP_FILE_NAME, "");
    EXPECT_STREQ(LCSTR(filename), "example.fb2");

    // file size
    lString32 s_file_size = doc_props->getStringDef(DOC_PROP_FILE_SIZE, "");
    lInt64 file_size = -1;
    ASSERT_TRUE(s_file_size.atoi(file_size));
    EXPECT_EQ(file_size, 100407L);

    // compressed file size
    lString32 s_file_packsize = doc_props->getStringDef(DOC_PROP_FILE_PACK_SIZE, "");
    lInt64 file_packsize = -1;
    ASSERT_TRUE(s_file_packsize.atoi(file_packsize));
    EXPECT_EQ(file_packsize, 60656L);

    delete view;

    ldomDocCache::clear();
    ldomDocCache::close();

    // Delete a copy of a file
    LVDeleteFile(cs32("tmp.fb2.zip"));

    CRLog::info("Finished GetFB2FilePropsInArc1FromCache");
    CRLog::info("=======================================");
}

TEST(DocPropsTests, GetFB2FilePropsInArc2FromCache) {
    CRLog::info("==============================");
    CRLog::info("Starting GetFB2FilePropsInArc2FromCache");

    // set up cache
    ASSERT_TRUE(ldomDocCache::init(cs32("./cache"), 10000000UL));

    LVDocView* view = new LVDocView(32, false);
    // open document
    ASSERT_TRUE(view->LoadDocument(TESTS_DATADIR "example.fb2.zip@/example.fb2"));
    // build render data
    view->Render(1000, 1000);
    // Save to cache
    ASSERT_EQ(view->updateCache(), CR_DONE);
    // Close
    view->close();
    delete view;

    // Make a file copy (in current directory)
    LVStreamRef streamIn = LVOpenFileStream(TESTS_DATADIR "example.fb2.zip", LVOM_READ);
    ASSERT_FALSE(streamIn.isNull());
    LVStreamRef streamOut = LVOpenFileStream("tmp.fb2.zip", LVOM_WRITE);
    ASSERT_FALSE(streamOut.isNull());
    ASSERT_EQ(LVPumpStream(streamOut, streamIn), 60828);
    streamIn.Clear();
    streamOut.Clear();

    // open the copied document using the cache of the original document
    view = new LVDocView(32, false);
    ASSERT_TRUE(view->LoadDocument("tmp.fb2.zip@/example.fb2"));
    ASSERT_TRUE(view->isOpenFromCache());

    CRPropRef doc_props = view->getDocProps();
    ASSERT_FALSE(doc_props.isNull());

    // archive name
    lString32 arcname = doc_props->getStringDef(DOC_PROP_ARC_NAME, "");
    EXPECT_STREQ(LCSTR(arcname), "tmp.fb2.zip");

    // archive path
    lString32 arcpath = doc_props->getStringDef(DOC_PROP_ARC_PATH, "");
    lString32 currDir = LVGetCurrentDirectory();
    LVReplacePathSeparator(currDir, U'/');
    LVReplacePathSeparator(arcpath, U'/');
    EXPECT_STREQ(LCSTR(arcpath), LCSTR(currDir));

    // archive file size
    lString32 s_arc_size = doc_props->getStringDef(DOC_PROP_ARC_SIZE, "");
    lInt64 arc_size = -1;
    ASSERT_TRUE(s_arc_size.atoi(arc_size));
    EXPECT_EQ(arc_size, 60828L);

    // path to file
    lString32 path = doc_props->getStringDef(DOC_PROP_FILE_PATH, "");
    EXPECT_STREQ(LCSTR(path), "");

    // file name
    lString32 filename = doc_props->getStringDef(DOC_PROP_FILE_NAME, "");
    EXPECT_STREQ(LCSTR(filename), "example.fb2");

    // file size
    lString32 s_file_size = doc_props->getStringDef(DOC_PROP_FILE_SIZE, "");
    lInt64 file_size = -1;
    ASSERT_TRUE(s_file_size.atoi(file_size));
    EXPECT_EQ(file_size, 100407L);

    // compressed file size
    lString32 s_file_packsize = doc_props->getStringDef(DOC_PROP_FILE_PACK_SIZE, "");
    lInt64 file_packsize = -1;
    ASSERT_TRUE(s_file_packsize.atoi(file_packsize));
    EXPECT_EQ(file_packsize, 60656L);

    delete view;

    ldomDocCache::clear();
    ldomDocCache::close();

    // Delete a copy of a file
    LVDeleteFile(cs32("tmp.fb2.zip"));

    CRLog::info("Finished GetFB2FilePropsInArc2FromCache");
    CRLog::info("=======================================");
}

TEST(DocPropsTests, GetEPUBFileProps1) {
    CRLog::info("==========================");
    CRLog::info("Starting GetEPUBFileProps1");

    // open document
    LVDocView* view = new LVDocView(32, false);
    ASSERT_TRUE(view->LoadDocument(TESTS_DATADIR "simple-epub2.epub"));

    CRPropRef doc_props = view->getDocProps();
    ASSERT_FALSE(doc_props.isNull());

    // archive name
    lString32 arcname = doc_props->getStringDef(DOC_PROP_ARC_NAME, "");
    EXPECT_STREQ(LCSTR(arcname), "");

    // archive path
    lString32 arcpath = doc_props->getStringDef(DOC_PROP_ARC_PATH, "");
    EXPECT_STREQ(LCSTR(arcpath), "");

    // archive file size
    lString32 s_arc_size = doc_props->getStringDef(DOC_PROP_ARC_SIZE, "");
    EXPECT_STREQ(LCSTR(s_arc_size), "");

    // path to file
    lString32 path = doc_props->getStringDef(DOC_PROP_FILE_PATH, "");
    EXPECT_STREQ(LCSTR(path), TESTS_DATADIR);

    // file name
    lString32 filename = doc_props->getStringDef(DOC_PROP_FILE_NAME, "");
    EXPECT_STREQ(LCSTR(filename), "simple-epub2.epub");

    // file size
    lString32 s_file_size = doc_props->getStringDef(DOC_PROP_FILE_SIZE, "");
    lInt64 file_size = -1;
    ASSERT_TRUE(s_file_size.atoi(file_size));
    EXPECT_EQ(file_size, 10775L);

    delete view;

    CRLog::info("Finished GetEPUBFileProps1");
    CRLog::info("==========================");
}

TEST(DocPropsTests, GetEPUBFilePropsInArc1) {
    CRLog::info("===============================");
    CRLog::info("Starting GetEPUBFilePropsInArc1");

    // open document
    LVDocView* view = new LVDocView(32, false);
    ASSERT_TRUE(view->LoadDocument(TESTS_DATADIR "simple-epub2.epub.zip"));

    CRPropRef doc_props = view->getDocProps();
    ASSERT_FALSE(doc_props.isNull());

    // archive name
    lString32 arcname = doc_props->getStringDef(DOC_PROP_ARC_NAME, "");
    EXPECT_STREQ(LCSTR(arcname), "simple-epub2.epub.zip");

    // archive path
    lString32 arcpath = doc_props->getStringDef(DOC_PROP_ARC_PATH, "");
    EXPECT_STREQ(LCSTR(arcpath), TESTS_DATADIR);

    // archive file size
    lString32 s_arc_size = doc_props->getStringDef(DOC_PROP_ARC_SIZE, "");
    lInt64 arc_size = -1;
    ASSERT_TRUE(s_arc_size.atoi(arc_size));
    EXPECT_EQ(arc_size, 10633L);

    // path to file
    lString32 path = doc_props->getStringDef(DOC_PROP_FILE_PATH, "");
    EXPECT_STREQ(LCSTR(path), "");

    // file name
    lString32 filename = doc_props->getStringDef(DOC_PROP_FILE_NAME, "");
    EXPECT_STREQ(LCSTR(filename), "simple-epub2.epub");

    // file size
    lString32 s_file_size = doc_props->getStringDef(DOC_PROP_FILE_SIZE, "");
    lInt64 file_size = -1;
    ASSERT_TRUE(s_file_size.atoi(file_size));
    EXPECT_EQ(file_size, 10775L);

    // compressed file size
    lString32 s_file_packsize = doc_props->getStringDef(DOC_PROP_FILE_PACK_SIZE, "");
    lInt64 file_packsize = -1;
    ASSERT_TRUE(s_file_packsize.atoi(file_packsize));
    EXPECT_EQ(file_packsize, 10449L);

    delete view;

    CRLog::info("Finished GetEPUBFilePropsInArc1");
    CRLog::info("===============================");
}

TEST(DocPropsTests, GetEPUBFilePropsInArc2) {
    CRLog::info("===============================");
    CRLog::info("Starting GetEPUBFilePropsInArc2");

    // open document
    LVDocView* view = new LVDocView(32, false);
    ASSERT_TRUE(view->LoadDocument(TESTS_DATADIR "simple-epub2.epub.zip@/simple-epub2.epub"));

    CRPropRef doc_props = view->getDocProps();
    ASSERT_FALSE(doc_props.isNull());

    // archive name
    lString32 arcname = doc_props->getStringDef(DOC_PROP_ARC_NAME, "");
    EXPECT_STREQ(LCSTR(arcname), "simple-epub2.epub.zip");

    // archive path
    lString32 arcpath = doc_props->getStringDef(DOC_PROP_ARC_PATH, "");
    EXPECT_STREQ(LCSTR(arcpath), TESTS_DATADIR);

    // archive file size
    lString32 s_arc_size = doc_props->getStringDef(DOC_PROP_ARC_SIZE, "");
    lInt64 arc_size = -1;
    ASSERT_TRUE(s_arc_size.atoi(arc_size));
    EXPECT_EQ(arc_size, 10633L);

    // path to file
    lString32 path = doc_props->getStringDef(DOC_PROP_FILE_PATH, "");
    EXPECT_STREQ(LCSTR(path), "");

    // file name
    lString32 filename = doc_props->getStringDef(DOC_PROP_FILE_NAME, "");
    EXPECT_STREQ(LCSTR(filename), "simple-epub2.epub");

    // file size
    lString32 s_file_size = doc_props->getStringDef(DOC_PROP_FILE_SIZE, "");
    lInt64 file_size = -1;
    ASSERT_TRUE(s_file_size.atoi(file_size));
    EXPECT_EQ(file_size, 10775L);

    // compressed file size
    lString32 s_file_packsize = doc_props->getStringDef(DOC_PROP_FILE_PACK_SIZE, "");
    lInt64 file_packsize = -1;
    ASSERT_TRUE(s_file_packsize.atoi(file_packsize));
    EXPECT_EQ(file_packsize, 10449L);

    delete view;

    CRLog::info("Finished GetEPUBFilePropsInArc2");
    CRLog::info("===============================");
}

TEST(DocPropsTests, GetFB2StreamProps1) {
    CRLog::info("===========================");
    CRLog::info("Starting GetFB2StreamProps1");

    // create memory stream
    LVStreamRef stream = LVCreateMemoryStream(lString32(TESTS_DATADIR "hello_fb2.fb2"));
    ASSERT_FALSE(stream.isNull());

    // open document
    LVDocView* view = new LVDocView(32, false);
    ASSERT_TRUE(view->LoadDocument(stream, U"/path/to/file/hello_fb2.fb2"));

    CRPropRef doc_props = view->getDocProps();
    ASSERT_FALSE(doc_props.isNull());

    // archive name
    lString32 arcname = doc_props->getStringDef(DOC_PROP_ARC_NAME, "");
    EXPECT_STREQ(LCSTR(arcname), "");

    // archive path
    lString32 arcpath = doc_props->getStringDef(DOC_PROP_ARC_PATH, "");
    EXPECT_STREQ(LCSTR(arcpath), "");

    // archive file size
    lString32 s_arc_size = doc_props->getStringDef(DOC_PROP_ARC_SIZE, "");
    EXPECT_STREQ(LCSTR(s_arc_size), "");

    // path to file
    lString32 path = doc_props->getStringDef(DOC_PROP_FILE_PATH, "");
    EXPECT_STREQ(LCSTR(path), "/path/to/file/");

    // file name
    lString32 filename = doc_props->getStringDef(DOC_PROP_FILE_NAME, "");
    EXPECT_STREQ(LCSTR(filename), "hello_fb2.fb2");

    // file size
    lString32 s_file_size = doc_props->getStringDef(DOC_PROP_FILE_SIZE, "");
    lInt64 file_size = -1;
    ASSERT_TRUE(s_file_size.atoi(file_size));
    EXPECT_EQ(file_size, 876L);

    delete view;

    CRLog::info("Finished GetFB2StreamProps1");
    CRLog::info("===========================");
}

TEST(DocPropsTests, GetFB2StreamPropsInArc1) {
    CRLog::info("================================");
    CRLog::info("Starting GetFB2StreamPropsInArc1");

    // create memory stream
    LVStreamRef stream = LVCreateMemoryStream(lString32(TESTS_DATADIR "example.fb2.zip"));
    ASSERT_FALSE(stream.isNull());

    // open document
    LVDocView* view = new LVDocView(32, false);
    ASSERT_TRUE(view->LoadDocument(stream, U"/path/to/file/example.fb2.zip"));

    CRPropRef doc_props = view->getDocProps();
    ASSERT_FALSE(doc_props.isNull());

    // archive name
    lString32 arcname = doc_props->getStringDef(DOC_PROP_ARC_NAME, "");
    EXPECT_STREQ(LCSTR(arcname), "example.fb2.zip");

    // archive path
    lString32 arcpath = doc_props->getStringDef(DOC_PROP_ARC_PATH, "");
    EXPECT_STREQ(LCSTR(arcpath), "/path/to/file/");

    // archive file size
    lString32 s_arc_size = doc_props->getStringDef(DOC_PROP_ARC_SIZE, "");
    lInt64 arc_size = -1;
    ASSERT_TRUE(s_arc_size.atoi(arc_size));
    EXPECT_EQ(arc_size, 60828L);

    // path to file
    lString32 path = doc_props->getStringDef(DOC_PROP_FILE_PATH, "");
    EXPECT_STREQ(LCSTR(path), "");

    // file name
    lString32 filename = doc_props->getStringDef(DOC_PROP_FILE_NAME, "");
    EXPECT_STREQ(LCSTR(filename), "example.fb2");

    // file size
    lString32 s_file_size = doc_props->getStringDef(DOC_PROP_FILE_SIZE, "");
    lInt64 file_size = -1;
    ASSERT_TRUE(s_file_size.atoi(file_size));
    EXPECT_EQ(file_size, 100407L);

    // compressed file size
    lString32 s_file_packsize = doc_props->getStringDef(DOC_PROP_FILE_PACK_SIZE, "");
    lInt64 file_packsize = -1;
    ASSERT_TRUE(s_file_packsize.atoi(file_packsize));
    EXPECT_EQ(file_packsize, 60656L);

    delete view;

    CRLog::info("Finished GetFB2StreamPropsInArc1");
    CRLog::info("================================");
}
