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

#ifndef RENDER_REFERENCE_DIR
#error Please define RENDER_REFERENCE_DIR, which points to the directory with the render reference files
#endif

// Fixtures

class DocPropsTests: public testing::Test
{
protected:
    LVDocView* m_view;
    CRPropRef m_props;
protected:
    DocPropsTests()
            : testing::Test() {
        m_view = NULL;
    }
    virtual void SetUp() override {
        m_view = new LVDocView(32, false);
    }

    virtual void TearDown() override {
        if (m_view) {
            delete m_view;
            m_view = 0;
        }
    }
};

// units tests

TEST_F(DocPropsTests, GetDocAuthorsOneInFB2) {
    CRLog::info("==============================");
    CRLog::info("Starting GetDocAuthorsOneInFB2");

    // open document
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "testprops-one-author.fb2"));

    // Get author(s)
    lString32 authors = m_view->getAuthors();

    EXPECT_STREQ(LCSTR(authors), "First Author");

    CRLog::info("Finished GetDocAuthorsOneInFB2");
    CRLog::info("==============================");
}

TEST_F(DocPropsTests, GetDocAuthorsTwoInFB2) {
    CRLog::info("==============================");
    CRLog::info("Starting GetDocAuthorsTwoInFB2");

    // open document
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "testprops-two-authors.fb2"));

    // Get author(s)
    lString32 authors = m_view->getAuthors();

    EXPECT_STREQ(LCSTR(authors), "First Author, Second G. Author");

    CRLog::info("Finished GetDocAuthorsTwoInFB2");
    CRLog::info("==============================");
}

TEST_F(DocPropsTests, GetDocGenresOneInFB2) {
    CRLog::info("=============================");
    CRLog::info("Starting GetDocGenresOneInFB2");

    // open document
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "testprops-one-genre.fb2"));

    // Get genres
    lString32 genres = m_view->getKeywords();

    EXPECT_STREQ(LCSTR(genres), "comp_programming");

    CRLog::info("Finished GetDocGenresOneInFB2");
    CRLog::info("=============================");
}

TEST_F(DocPropsTests, GetDocGenresTwoInFB2) {
    CRLog::info("=============================");
    CRLog::info("Starting GetDocGenresTwoInFB2");

    // open document
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "testprops-two-genres.fb2"));

    // Get genres
    lString32 genres = m_view->getKeywords();

    EXPECT_STREQ(LCSTR(genres), "comp_programming, comp_soft");

    CRLog::info("Finished GetDocGenresTwoInFB2");
    CRLog::info("=============================");
}

TEST_F(DocPropsTests, GetDocAuthorsOneInEPUB) {
    CRLog::info("===============================");
    CRLog::info("Starting GetDocAuthorsOneInEPUB");

    // open document
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "testprops-one-author.epub"));

    // Get author(s)
    lString32 authors = m_view->getAuthors();

    EXPECT_STREQ(LCSTR(authors), "First Author");

    CRLog::info("Finished GetDocAuthorsOneInEPUB");
    CRLog::info("===============================");
}

TEST_F(DocPropsTests, GetDocAuthorsTwoInEPUB) {
    CRLog::info("===============================");
    CRLog::info("Starting GetDocAuthorsTwoInEPUB");

    // open document
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "testprops-two-authors.epub"));

    // Get author(s)
    lString32 authors = m_view->getAuthors();

    EXPECT_STREQ(LCSTR(authors), "First Author, Second Good Author");

    CRLog::info("Finished GetDocAuthorsTwoInEPUB");
    CRLog::info("===============================");
}

TEST_F(DocPropsTests, GetDocKeywordsOneInEPUB) {
    CRLog::info("================================");
    CRLog::info("Starting GetDocKeywordsOneInEPUB");

    // open document
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "testprops-one-keyword.epub"));

    // Get keywords
    lString32 keywords = m_view->getKeywords();

    EXPECT_STREQ(LCSTR(keywords), "programming");

    CRLog::info("Finished GetDocKeywordsOneInEPUB");
    CRLog::info("================================");
}

TEST_F(DocPropsTests, GetDocKeywordsTwoInEPUB) {
    CRLog::info("================================");
    CRLog::info("Starting GetDocKeywordsTwoInEPUB");

    // open document
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "testprops-two-keywords.epub"));

    // Get keywords
    lString32 keywords = m_view->getKeywords();

    EXPECT_STREQ(LCSTR(keywords), "programming, soft");

    CRLog::info("Finished GetDocKeywordsTwoInEPUB");
    CRLog::info("================================");
}
