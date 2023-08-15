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
 * \file tests_book_cover.cpp
 * \brief Testing getting a book cover.
 */

#include <crlog.h>
#include <lvdocview.h>

#include "gtest/gtest.h"

#ifndef TESTS_DATADIR
#error Please define TESTS_DATADIR, which points to the directory with the data files for the tests
#endif

// units tests

TEST(BookCoverTests, GetBookCoverInFB2_slow) {
    CRLog::info("===============================");
    CRLog::info("Starting GetBookCoverInFB2_slow");

    // open document (only metadata)
    LVDocView* view = new LVDocView(32, true);
    ASSERT_TRUE(view->LoadDocument(TESTS_DATADIR "example.fb2.zip@/example.fb2", true));

    // Get image
    LVStreamRef imgStream = view->getBookCoverImageStream();
    ASSERT_FALSE(imgStream.isNull());
    LVImageSourceRef img = LVCreateStreamImageSource(imgStream);
    ASSERT_FALSE(img.isNull());

    // Test
    EXPECT_EQ(img->GetWidth(), 120);
    EXPECT_EQ(img->GetHeight(), 171);

    delete view;

    CRLog::info("Finished GetBookCoverInFB2_slow");
    CRLog::info("===============================");
}

TEST(BookCoverTests, GetBookCoverInFB2_fast) {
    CRLog::info("===============================");
    CRLog::info("Starting GetBookCoverInFB2_fast");

    LVStreamRef imgStream = LVGetBookCoverStream(TESTS_DATADIR "example.fb2.zip@/example.fb2");
    ASSERT_FALSE(imgStream.isNull());

    // Get image
    LVImageSourceRef img = LVCreateStreamImageSource(imgStream);
    ASSERT_FALSE(img.isNull());

    // Test
    EXPECT_EQ(img->GetWidth(), 120);
    EXPECT_EQ(img->GetHeight(), 171);

    CRLog::info("Finished GetBookCoverInFB2_fast");
    CRLog::info("===============================");
}

TEST(BookCoverTests, GetBookCoverInEPUB_slow) {
    CRLog::info("================================");
    CRLog::info("Starting GetBookCoverInEPUB_slow");

    // open document (only metadata)
    LVDocView* view = new LVDocView(32, true);
    ASSERT_TRUE(view->LoadDocument(TESTS_DATADIR "simple-epub2-cover.epub", true));

    // Get image
    LVStreamRef imgStream = view->getBookCoverImageStream();
    ASSERT_FALSE(imgStream.isNull());
    LVImageSourceRef img = LVCreateStreamImageSource(imgStream);
    ASSERT_FALSE(img.isNull());

    // Test
    EXPECT_EQ(img->GetWidth(), 188);
    EXPECT_EQ(img->GetHeight(), 334);

    delete view;

    CRLog::info("Finished GetBookCoverInEPUB_slow");
    CRLog::info("================================");
}

TEST(BookCoverTests, GetBookCoverInEPUB_fast) {
    CRLog::info("================================");
    CRLog::info("Starting GetBookCoverInEPUB_fast");

    LVStreamRef imgStream = LVGetBookCoverStream(TESTS_DATADIR "simple-epub2-cover.epub");
    ASSERT_FALSE(imgStream.isNull());

    LVImageSourceRef img = LVCreateStreamImageSource(imgStream);
    ASSERT_FALSE(img.isNull());

    // Test
    EXPECT_EQ(img->GetWidth(), 188);
    EXPECT_EQ(img->GetHeight(), 334);

    CRLog::info("Finished GetBookCoverInEPUB_fast");
    CRLog::info("================================");
}
