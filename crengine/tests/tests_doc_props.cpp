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
