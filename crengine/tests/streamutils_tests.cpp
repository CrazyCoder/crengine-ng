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
 * \file streamutils_tests.cpp
 * \brief Various tests for streamutils module.
 */

#include <lvstreamutils.h>
#include <lvdocview.h>
#include <crlog.h>

#include "gtest/gtest.h"

// units tests

TEST(StreamUtilsTests, LVGetCurrentDirectoryTest) {
    lString32 cwd = LVGetCurrentDirectory();
    CRLog::trace("cwd is %s", LCSTR(cwd));
    EXPECT_GT(cwd.length(), 0);
    EXPECT_TRUE(LVIsAbsolutePath(cwd));
    EXPECT_TRUE(cwd.endsWith("/crengine/tests/"));
}

TEST(StreamUtilsTests, LVGetAbsolutePathTest) {
    lString32 relPath1 = cs32("Makefile");
    lString32 absPath1 = LVGetAbsolutePath(relPath1);
    CRLog::trace("absPath1 is %s", LCSTR(absPath1));
    EXPECT_GT(absPath1.length(), 0);
    EXPECT_TRUE(LVIsAbsolutePath(absPath1));
    EXPECT_TRUE(absPath1.endsWith("/crengine/tests/Makefile"));
    lString32 relPath2 = cs32("../Makefile");
    lString32 absPath2 = LVGetAbsolutePath(relPath2);
    CRLog::trace("absPath2 is %s", LCSTR(absPath2));
    EXPECT_GT(absPath2.length(), 0);
    EXPECT_TRUE(LVIsAbsolutePath(absPath2));
    EXPECT_TRUE(absPath2.endsWith("/crengine/Makefile"));
}

TEST(StreamUtilsTests, OpenDocByRelativePathTest) {
    // Create doc view instance
    LVDocView* docView = new LVDocView(32, false);
    docView->setMinFontSize(8);
    docView->setMaxFontSize(320);
    docView->Resize(640, 360);
    // Open document by relative path
    ASSERT_TRUE(docView->LoadDocument("./hello_fb2_rel.fb2"));
    // Checking if the file path is absolute
    lString32 path = docView->getDocProps()->getStringDef(DOC_PROP_FILE_PATH);
    CRLog::trace("path: %s", LCSTR(path));
    EXPECT_GT(path.length(), 0);
    EXPECT_TRUE(LVIsAbsolutePath(path));
    delete docView;
}
