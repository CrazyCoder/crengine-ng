/***************************************************************************
 *   crengine-ng, unit testing                                             *
 *   Copyright (C) 2022,2024 Aleksey Chernov <valexlin@gmail.com>          *
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
 * \file tests_string_funcs.cpp
 * \brief Tests various functions of the lStringX classes.
 */

#include <crlog.h>
#include <lvstring.h>

#include <float.h>

#include "gtest/gtest.h"

static inline double my_fabs(double v) {
    return (v >= 0) ? v : -v;
}

static bool my_double_eq(double v1, double v2) {
    return my_fabs(v1 - v2) <= DBL_EPSILON;
}

// units tests

TEST(StringFuncsTests, Test_lString32_atod) {
    CRLog::info("============================");
    CRLog::info("Starting Test_lString32_atod");

    double val; // parsed double value

    EXPECT_TRUE(cs32("1.1").atod(val) && my_double_eq(val, 1.1));
    EXPECT_TRUE(cs32("+1.1").atod(val) && my_double_eq(val, 1.1));
    EXPECT_TRUE(cs32("-1.1").atod(val) && my_double_eq(val, -1.1));
    EXPECT_TRUE(cs32("8.1e15").atod(val) && my_double_eq(val, 8.1e15));
    EXPECT_TRUE(cs32("8.1e+15").atod(val) && my_double_eq(val, 8.1e15));
    EXPECT_TRUE(cs32("-8.1e15").atod(val) && my_double_eq(val, -8.1e15));
    EXPECT_TRUE(cs32("-8.1e+15").atod(val) && my_double_eq(val, -8.1e15));
    EXPECT_TRUE(cs32("8.1e-15").atod(val) && my_double_eq(val, 8.1e-15));
    EXPECT_TRUE(cs32("-8.1e-15").atod(val) && my_double_eq(val, -8.1e-15));
    // With alternative decimal separator
    EXPECT_TRUE(cs32("1,1").atod(val, ',') && my_double_eq(val, 1.1));
    EXPECT_TRUE(cs32("1*1").atod(val, '*') && my_double_eq(val, 1.1));
    EXPECT_TRUE(cs32(",1").atod(val, ',') && my_double_eq(val, 0.1));
    // With spaces at the beginning and/or end of the line
    EXPECT_TRUE(cs32("  1.1").atod(val) && my_double_eq(val, 1.1));
    EXPECT_TRUE(cs32("1.1   ").atod(val) && my_double_eq(val, 1.1));
    EXPECT_TRUE(cs32(" 1.1 ").atod(val) && my_double_eq(val, 1.1));
    EXPECT_TRUE(cs32(".33").atod(val) && my_double_eq(val, 0.33));
    EXPECT_TRUE(cs32("-.33").atod(val) && my_double_eq(val, -0.33));
    EXPECT_TRUE(cs32("+.33").atod(val) && my_double_eq(val, 0.33));
    // Short call
    EXPECT_TRUE(my_double_eq(cs32("1").atod(), 1.0));
    EXPECT_TRUE(my_double_eq(cs32(".1").atod(), 0.1));

    // Invalid numbers
    val = 1.2;
    EXPECT_FALSE(cs32("abc").atod(val));
    EXPECT_FALSE(cs32("abc123").atod(val));
    EXPECT_FALSE(cs32("123abc").atod(val));
    EXPECT_FALSE(cs32("1.1").atod(val, ','));
    EXPECT_FALSE(cs32("1,1").atod(val, '.'));
    EXPECT_FALSE(cs32("8.1e15.5").atod(val));
    EXPECT_FALSE(cs32(".").atod(val));
    EXPECT_FALSE(cs32(".").atod(val, ','));
    EXPECT_FALSE(cs32(",").atod(val, '.'));
    // Short call
    EXPECT_DOUBLE_EQ(cs32("abc").atod(), 0.0);
    EXPECT_DOUBLE_EQ(cs32("abc123").atod(), 0.0);
    EXPECT_DOUBLE_EQ(cs32("89abc").atod(), 0.0);

    // the value should not be changed in case of any failure
    EXPECT_TRUE(my_double_eq(val, 1.2));

    CRLog::info("Finished Test_lString32_atod");
    CRLog::info("============================");
}

TEST(StringFuncsTests, WTF8ConversionTests) {
    static const lChar32 uni_chars[] = {
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
