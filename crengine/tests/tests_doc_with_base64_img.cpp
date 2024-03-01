/***************************************************************************
 *   crengine-ng, unit testing                                             *
 *   Copyright (C) 2024 Aleksey Chernov <valexlin@gmail.com>               *
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
 * \file tests_doc_with_base64_img.cpp
 * \brief Testing documents with embedded base64 encoded images.
 */

#include <crlog.h>
#include <lvdocview.h>
#include <ldomdocument.h>
#include <ldomdoccache.h>
#include <lvrend.h>

#include "gtest/gtest.h"

#ifndef TESTS_DATADIR
#error Please define TESTS_DATADIR, which points to the directory with the data files for the tests
#endif

// Fixtures

class DocWithBase64ImgTests: public testing::Test
{
protected:
    bool m_initOK;
    LVDocView* m_view;
    CRPropRef m_props;
protected:
    DocWithBase64ImgTests()
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

    bool setCaching(bool caching) {
        if (caching) {
            return ldomDocCache::init(cs32("./cache"), 10000000UL);
        } else {
            return ldomDocCache::close();
        }
    }
};

// units tests

TEST_F(DocWithBase64ImgTests, TestDocWithBase64Img) {
    CRLog::info("=============================");
    CRLog::info("Starting TestDocWithBase64Img");
    ASSERT_TRUE(m_initOK);

    // set properties
    ASSERT_TRUE(setProperty(PROP_REQUESTED_DOM_VERSION, 20200824));
    ASSERT_TRUE(setProperty(PROP_RENDER_BLOCK_RENDERING_FLAGS, BLOCK_RENDERING_FLAGS_WEB));

    // open document & render into drawbuf
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "image-base64.html")); // load document
    ASSERT_TRUE(setCSS("htm.css"));

    // Test attributes
    ldomXPointer imgPtr = m_view->getDocument()->createXPointer(cs32("/html/body/img.0"));
    ASSERT_TRUE(!imgPtr.isNull());
    ldomNode* imgNode = imgPtr.getNode();
    ASSERT_TRUE(NULL != imgNode);
    lString32 srcValue = imgNode->getAttributeValue("src");
    EXPECT_EQ(srcValue.length(), 685006);

    CRLog::info("Finished TestDocWithBase64Img");
    CRLog::info("=============================");
}

TEST_F(DocWithBase64ImgTests, TestDocWithBase64ImgFromCache) {
    CRLog::info("======================================");
    CRLog::info("Starting TestDocWithBase64ImgFromCache");
    ASSERT_TRUE(m_initOK);

    // set properties
    ASSERT_TRUE(setProperty(PROP_REQUESTED_DOM_VERSION, 20200824));
    ASSERT_TRUE(setProperty(PROP_RENDER_BLOCK_RENDERING_FLAGS, BLOCK_RENDERING_FLAGS_WEB));

    ASSERT_TRUE(setCaching(true));

    // Cache document: open, render, save to cache
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "image-base64.html"));
    ASSERT_TRUE(setCSS("htm.css"));

    // Test attributes
    ldomXPointer imgPtr = m_view->getDocument()->createXPointer(cs32("/html/body/img.0"));
    ASSERT_TRUE(!imgPtr.isNull());
    ldomNode* imgNode = imgPtr.getNode();
    ASSERT_TRUE(NULL != imgNode);
    lString32 srcValue = imgNode->getAttributeValue("src");
    EXPECT_EQ(srcValue.length(), 685006);

    // build render data
    m_view->Render(1000, 1000);
    // Save to cache
    ASSERT_EQ(m_view->updateCache(), CR_DONE);
    // Close
    m_view->close();

    // open document using the cache
    ASSERT_TRUE(m_view->LoadDocument(TESTS_DATADIR "image-base64.html")); // load document
    ASSERT_TRUE(m_view->isOpenFromCache());

    // Test attributes
    ldomXPointer imgPtr2 = m_view->getDocument()->createXPointer(cs32("/html/body/img.0"));
    ASSERT_TRUE(!imgPtr2.isNull());
    ldomNode* imgNode2 = imgPtr2.getNode();
    ASSERT_TRUE(NULL != imgNode2);
    lString32 srcValue2 = imgNode2->getAttributeValue("src");
    EXPECT_EQ(srcValue2.length(), 685006);

    CRLog::info("Finished TestDocWithBase64ImgFromCache");
    CRLog::info("======================================");
}
