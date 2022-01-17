/**
 *  crengine-ng
 *  Copyright (c) 2022 Aleksey Chernov
 *
 *  \file wtf8_tests.cpp
 *  \brief crengine-ng unit test module.
 *  Tests WTF-8 encoding/decoding.
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

#include <lvstring.h>

#include "gtest/gtest.h"

lChar32 uni_chars[] = {
    0x10000, // LINEAR B SYLLABLE B008 A
    0x2026,
    0x10123, // AEGEAN NUMBER TWO THOUSAND
    0x10081, // LINEAR B IDEOGRAM B102 WOMAN
    0x1F600, // GRINNING FACE
    0x1F601, // GRINNING FACE WITH SMILING EYES
    0x1F602, // FACE WITH TEARS OF JOY
    0x1F603, // SMILING FACE WITH OPEN MOUTH
    0x1F604, // SMILING FACE WITH OPEN MOUTH AND SMILING EYES
    0x1F605, // SMILING FACE WITH OPEN MOUTH AND COLD SWEAT
    0x1F606, // SMILING FACE WITH OPEN MOUTH AND TIGHTLY-CLOSED EYES
    0x1F607, // SMILING FACE WITH HALO
    0x1F608, // SMILING FACE WITH HORNS
    0x1F609, // WINKING FACE
    0x1F60A, // SMILING FACE WITH SMILING EYES
    0x1F60B  // FACE SAVOURING DELICIOUS FOOD
};

// units tests

TEST(WTF8EncodinsTests, WTF8ConversionTests) {
    lString32 src;
    for (size_t i = 0; i < sizeof(uni_chars) / sizeof(lUInt32); i++) {
        src.append(1, uni_chars[i]);
    }
    lString8 dst = UnicodeToUtf8(src);
    lString8 dstw = UnicodeToWtf8(src);
    // Back to unicode
    lString32 str2 = Wtf8ToUnicode(dstw);
    //   and compare...
    EXPECT_NE(dst.compare(dstw), 0);
    EXPECT_EQ(str2.compare(src), 0);
}
