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

    bool setCSS(const char* cssfile) {
        lString8 csscont;
        lString8 fname = cs8(CSS_DIR);
        fname += cssfile;
        bool res = LVLoadStylesheetFile(Utf8ToUnicode(fname), csscont);
        if (res)
            m_view->setStyleSheet(csscont);
        return res;
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
