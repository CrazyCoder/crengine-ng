/***************************************************************************
 *   crengine-ng, unit testing                                             *
 *   Copyright (C) 2007 Vadim Lopatin <coolreader.org@gmail.com>           *
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
 * \file tests_props.cpp
 * \brief Property container testing
 */

#include <crprops.h>
#include <lvstreamutils.h>
#include <lvserialbuf.h>

#include "gtest/gtest.h"

#include "compare-two-textfiles.h"

#ifndef PROPS_REFERENCE_DIR
#error Please define PROPS_REFERENCE_DIR, which points to directory with property container reference files
#endif

// Fixtures

class PropertyTests: public testing::Test
{
protected:
    PropertyTests()
            : testing::Test() {
    }
    virtual void SetUp() override {
        CRPropRef props = LVCreatePropsContainer();
        props->setString("test.string.values.1", lString32("string value 1"));
        props->setString("test.string.values.2", lString32("string value 2 with extra chars(\\\r\n)"));
        props->setBool("test.string.boolean1", true);
        props->setBool("test.string.boolean2", false);
        props->setString("test.string.more_values.2", lString32("string more values (2)"));
        props->setString("test.string.values.22", lString32("string value 22"));
        props->setInt("test.int.value1", 1);
        props->setInt("test.int.value2", -2);
        props->setInt("test.int.value3", 3);
        props->setInt("test.int.value4", 4);
        props->setInt64("test.int.big-value1", 783142387267LL);
        props->setPoint("test.points.1", lvPoint(1, 2));
        props->setPoint("test.points.2", lvPoint(3, 4));
        props->setRect("test.rects.1", lvRect(1, 2, 3, 4));
        props->setRect("test.rects.2", lvRect(-1, -2, 3, 4));
        props->setRect("test.rects.1copy", props->getRectDef("test.rects.1", lvRect()));
        props->setPoint("test.points.1copy", props->getPointDef("test.points.1", lvPoint()));
        props->setColor("test.colors.1", 0x012345);
        props->setColor("test.colors.1copy", props->getColorDef("test.colors.1"));
        CRPropRef sub = props->getSubProps("test.string.");
        props->setString("test.results.values.1", sub->getStringDef("values.2"));
        props->setInt("test.results.str-items", sub->getCount());
        props->setString("test.results.item1-value", sub->getValue(1));
        props->setString("test.results.item2-name", Utf8ToUnicode(lString8(sub->getName(2))));
        props->setBool("test.results.compare-chars-eq", sub->getStringDef("values.2") == lString32("string value 2 with extra chars(\\\r\n)"));
        m_props = props;
    }
    virtual void TearDown() override {
        LVDeleteFile(cs8("props1.ini"));
        LVDeleteFile(cs8("props1_dup.ini"));
        LVDeleteFile(cs8("props2.ini"));
    }
protected:
    CRPropRef m_props;
};

// units tests

TEST_F(PropertyTests, LoadFromFile) {
    // 1. Save props to file
    LVStreamRef stream = LVOpenFileStream("props1.ini", LVOM_WRITE);
    ASSERT_TRUE(m_props->saveToStream(stream.get()));
    stream.Clear();

    // 2. Load props from file to new props container
    CRPropRef props = LVCreatePropsContainer();
    LVStreamRef istream = LVOpenFileStream("props1.ini", LVOM_READ);
    ASSERT_TRUE(!istream.isNull());
    EXPECT_TRUE(props->loadFromStream(istream.get()));
    istream.Clear();

    // 3. Save new props container to new file
    LVStreamRef ostream = LVOpenFileStream("props1_dup.ini", LVOM_WRITE);
    ASSERT_TRUE(!ostream.isNull());
    EXPECT_TRUE(props->saveToStream(ostream.get()));
    ostream.Clear();

    // 4. Compare new file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoTextFiles("props1_dup.ini", PROPS_REFERENCE_DIR "props1.ini"));
}

TEST_F(PropertyTests, SaveToFile) {
    LVStreamRef stream = LVOpenFileStream("props1.ini", LVOM_WRITE);
    ASSERT_TRUE(m_props->saveToStream(stream.get()));
    stream.Clear();
    // Compare with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoTextFiles("props1.ini", PROPS_REFERENCE_DIR "props1.ini"));
}

TEST_F(PropertyTests, AddPropToExistingFile) {
    // Testing adding a property to an existing properties file
    LVStreamRef stream = LVOpenFileStream("props1.ini", LVOM_WRITE);
    ASSERT_TRUE(m_props->saveToStream(stream.get()));
    stream.Clear();

    CRPropRef props = LVCreatePropsContainer();
    LVStreamRef istream = LVOpenFileStream("props1.ini", LVOM_READ);
    LVStreamRef ostream = LVOpenFileStream("props2.ini", LVOM_WRITE);
    ASSERT_TRUE(!istream.isNull() && !ostream.isNull());
    ASSERT_TRUE(props->loadFromStream(istream.get()));
    props->setBool("add.test.result", props->getIntDef("test.int.value2", 0) == -2);
    EXPECT_TRUE(props->saveToStream(ostream.get()));
    ostream.Clear();
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoTextFiles("props2.ini", PROPS_REFERENCE_DIR "props2.ini"));
}

TEST_F(PropertyTests, GetPropValues) {
    // Strings
    lString32 strValue;
    EXPECT_TRUE(m_props->getString("test.string.values.1", strValue));
    EXPECT_STREQ(LCSTR(strValue), "string value 1");
    EXPECT_TRUE(m_props->getString("test.string.values.2", strValue));
    EXPECT_STREQ(LCSTR(strValue), "string value 2 with extra chars(\\\r\n)");
    //   using defaults
    strValue = m_props->getStringDef("test.string.values.1", "_def_value_");
    EXPECT_STREQ(LCSTR(strValue), "string value 1");
    strValue = m_props->getStringDef("test.string.values._non_exists_", "_def_value_");
    EXPECT_STREQ(LCSTR(strValue), "_def_value_");

    // Bools
    bool boolValue;
    EXPECT_TRUE(m_props->getBool("test.string.boolean1", boolValue));
    EXPECT_EQ(boolValue, true);
    EXPECT_TRUE(m_props->getBool("test.string.boolean2", boolValue));
    EXPECT_EQ(boolValue, false);
    //   using defaults
    boolValue = m_props->getBoolDef("test.string.boolean1", false);
    EXPECT_EQ(boolValue, true);
    boolValue = m_props->getBoolDef("test.string.boolean1._non_exists_", false);
    EXPECT_EQ(boolValue, false);

    // Integers
    int intValue;
    EXPECT_TRUE(m_props->getInt("test.int.value1", intValue));
    EXPECT_EQ(intValue, 1);
    EXPECT_TRUE(m_props->getInt("test.int.value2", intValue));
    EXPECT_EQ(intValue, -2);
    EXPECT_TRUE(m_props->getInt("test.int.value3", intValue));
    EXPECT_EQ(intValue, 3);
    EXPECT_TRUE(m_props->getInt("test.int.value4", intValue));
    EXPECT_EQ(intValue, 4);
    //   using defaults
    intValue = m_props->getIntDef("test.int.value1", -1);
    EXPECT_EQ(intValue, 1);
    intValue = m_props->getIntDef("test.int.value1._non_exists_", -1);
    EXPECT_EQ(intValue, -1);

    // Int64
    lInt64 int64Value;
    EXPECT_TRUE(m_props->getInt64("test.int.big-value1", int64Value));
    EXPECT_EQ(int64Value, 783142387267LL);
    //   using defaults
    int64Value = m_props->getInt64Def("test.int.big-value1", -1);
    EXPECT_EQ(int64Value, 783142387267LL);
    int64Value = m_props->getInt64Def("test.int.big-value1._non_exists_", -1);
    EXPECT_EQ(int64Value, -1);

    // Points
    lvPoint pointValue;
    EXPECT_TRUE(m_props->getPoint("test.points.1", pointValue));
    EXPECT_EQ(pointValue.x, 1);
    EXPECT_EQ(pointValue.y, 2);
    EXPECT_TRUE(m_props->getPoint("test.points.2", pointValue));
    EXPECT_EQ(pointValue.x, 3);
    EXPECT_EQ(pointValue.y, 4);
    //   using defaults
    pointValue = m_props->getPointDef("test.points.1", lvPoint(-1, -1));
    EXPECT_EQ(pointValue.x, 1);
    EXPECT_EQ(pointValue.y, 2);
    pointValue = m_props->getPointDef("test.points.1_non_exists_", lvPoint(-1, -1));
    EXPECT_EQ(pointValue.x, -1);
    EXPECT_EQ(pointValue.y, -1);

    // Rects
    lvRect rectValue;
    EXPECT_TRUE(m_props->getRect("test.rects.1", rectValue));
    EXPECT_EQ(rectValue.left, 1);
    EXPECT_EQ(rectValue.top, 2);
    EXPECT_EQ(rectValue.right, 3);
    EXPECT_EQ(rectValue.bottom, 4);
    EXPECT_TRUE(m_props->getRect("test.rects.2", rectValue));
    EXPECT_EQ(rectValue.left, -1);
    EXPECT_EQ(rectValue.top, -2);
    EXPECT_EQ(rectValue.right, 3);
    EXPECT_EQ(rectValue.bottom, 4);
    //   using defaults
    rectValue = m_props->getRectDef("test.rects.1", lvRect(-1, -1, -1, -1));
    EXPECT_EQ(rectValue.left, 1);
    EXPECT_EQ(rectValue.top, 2);
    EXPECT_EQ(rectValue.right, 3);
    EXPECT_EQ(rectValue.bottom, 4);
    rectValue = m_props->getRectDef("test.rects.1_non_exists_", lvRect(-3, -3, -3, -3));
    EXPECT_EQ(rectValue.left, -3);
    EXPECT_EQ(rectValue.top, -3);
    EXPECT_EQ(rectValue.right, -3);
    EXPECT_EQ(rectValue.bottom, -3);

    // Colors
    lUInt32 colorValue;
    EXPECT_TRUE(m_props->getColor("test.colors.1", colorValue));
    EXPECT_EQ(colorValue, 0x012345);
    //   using defaults
    colorValue = m_props->getColorDef("test.colors.1", 0);
    EXPECT_EQ(colorValue, 0x012345);
    colorValue = m_props->getColorDef("test.colors.1._non_exists_", 0x112233);
    EXPECT_EQ(colorValue, 0x112233);
}
