/**
 *  crengine-ng
 *  Copyright (c) 2022 Aleksey Chernov
 *
 *  \file textrender_tests.cpp
 *  \brief crengine-ng unit test module.
 *  Tests various text rendering options.
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

#include <crsetup.h>

#if USE_FREETYPE == 1 && BUILD_LITE != 1

#include <crlog.h>
#include <lvdocview.h>
#include <lvtinydom.h>
#include <lvstreamutils.h>
#include <lvbyteorder.h>

#include "lvtinydom_cachefile.h"
#include "lvtinydom_private.h"
#include "savetobmp.h"
#include "compare-two-binfiles.h"

#include "gtest/gtest.h"

#ifndef TESTS_DATADIR
#error Please define TESTS_DATADIR, which points to the directory with the data files for the tests
#endif

#ifndef TESTS_TMPDIR
#define TESTS_TMPDIR "/tmp/"
#endif

#ifndef RENDER_REFERENCE_DIR
#error Please define RENDER_REFERENCE_DIR, which points to the directory with the render reference files
#endif

#if USE_LIMITED_FONT_SIZES_SET == 1
#error With (USE_LIMITED_FONT_SIZES_SET == 1) this test will be incorrect!
#endif

#if USE_ZLIB != 1
#error These unit tests do not work without zlib!
#endif

// Fixtures

class TextRenderTests: public testing::Test
{
protected:
    bool m_initOK;
    LVDocView* m_view;
    CRPropRef m_props;
protected:
    TextRenderTests()
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
            m_props->setString(PROP_FONT_FACE, "FreeSerif");
            m_props->setString(PROP_FONT_SIZE, "32");
            m_props->setString(PROP_FONT_GAMMA, "1.0");
            m_props->setString(PROP_FONT_HINTING, "0");
            m_props->setString(PROP_FONT_SHAPING, "0");
            m_props->setString(PROP_FONT_KERNING_ENABLED, "0");
            m_props->setString(PROP_FONT_BASE_WEIGHT, "400");
            m_props->setString(PROP_FONT_COLOR, "0x00000000");
            m_props->setString(PROP_BACKGROUND_COLOR, "0x00FFFFFF");
            m_props->setString(PROP_STATUS_LINE, "0");
            m_props->setString(PROP_FONT_ANTIALIASING, "3");
            m_props->setString(PROP_LANDSCAPE_PAGES, "1");
            m_props->setString(PROP_PAGE_MARGIN_TOP, "8");
            m_props->setString(PROP_PAGE_MARGIN_BOTTOM, "8");
            m_props->setString(PROP_PAGE_MARGIN_LEFT, "8");
            m_props->setString(PROP_PAGE_MARGIN_RIGHT, "8");
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

    bool setProperty(const char* propName, const char* value) {
        m_props->setString(propName, value);
        CRPropRef unknown = m_view->propsApply(m_props);
        return unknown->getCount() == 0;
    }
};

// units tests

TEST_F(TextRenderTests, RenderTestsAntialiasingNone) {
    CRLog::info("====================================");
    CRLog::info("Starting RenderTestsAntialiasingNone");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "FreeSerif"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "80"));
    // "0" - none; "3" - gray; "4" - RGB, "5" - BGR, "8" - RGB_V, "9" - BGR_V
    EXPECT_TRUE(setProperty(PROP_FONT_ANTIALIASING, "0"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "hello_fb2.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "antialiasing/01-hinting-no,shaping-simple,aa-none.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsAntialiasingNone");
    CRLog::info("====================================");
}

TEST_F(TextRenderTests, RenderTestsAntialiasingGray) {
    CRLog::info("====================================");
    CRLog::info("Starting RenderTestsAntialiasingGray");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "FreeSerif"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "80"));
    // "0" - none; "3" - gray; "4" - RGB, "5" - BGR, "8" - RGB_V, "9" - BGR_V
    EXPECT_TRUE(setProperty(PROP_FONT_ANTIALIASING, "3"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "hello_fb2.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "antialiasing/02-hinting-no,shaping-simple,aa-gray.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsAntialiasingGray");
    CRLog::info("====================================");
}

TEST_F(TextRenderTests, RenderTestsAntialiasingRGB) {
    CRLog::info("===================================");
    CRLog::info("Starting RenderTestsAntialiasingRGB");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "FreeSerif"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "80"));
    // "0" - none; "3" - gray; "4" - RGB, "5" - BGR, "8" - RGB_V, "9" - BGR_V
    EXPECT_TRUE(setProperty(PROP_FONT_ANTIALIASING, "4"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "hello_fb2.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    // FreeType can be compiled with either ClearType LCD subpixel antialiasing
    // or Harmony LCD subpixel antialiasing.
    // The result of such subpixel antialiasing will be different.
    EXPECT_TRUE(
            crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "antialiasing/03-hinting-no,shaping-simple,aa-rgb-ct.bmp.gz", "output.bmp") ||
            crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "antialiasing/03-hinting-no,shaping-simple,aa-rgb-hm.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsAntialiasingRGB");
    CRLog::info("===================================");
}

TEST_F(TextRenderTests, RenderTestsLigaturesOff) {
    CRLog::info("================================");
    CRLog::info("Starting RenderTestsLigaturesOff");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "FreeSerif"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "50"));
    // "0" - simple, "1" - harfbuzz light, "2" - harfbuzz full
    EXPECT_TRUE(setProperty(PROP_FONT_SHAPING, "0"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "ligatures.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "ligatures/01-hinting-no,shaping-simple,ligatures-off.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsLigaturesOff");
    CRLog::info("================================");
}

TEST_F(TextRenderTests, RenderTestsLigaturesOn) {
    CRLog::info("===============================");
    CRLog::info("Starting RenderTestsLigaturesOn");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "FreeSerif"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "50"));
    // "0" - simple, "1" - harfbuzz light, "2" - harfbuzz full
    EXPECT_TRUE(setProperty(PROP_FONT_SHAPING, "2"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "ligatures.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "ligatures/02-hinting-no,shaping-full,ligatures-on.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsLigaturesOn");
    CRLog::info("===============================");
}

TEST_F(TextRenderTests, RenderTestsShapingSimpleKerningOff) {
    CRLog::info("===========================================");
    CRLog::info("Starting RenderTestsShapingSimpleKerningOff");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "FreeSerif"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "130"));
    // "0" - simple, "1" - harfbuzz light, "2" - harfbuzz full
    EXPECT_TRUE(setProperty(PROP_FONT_SHAPING, "0"));
    // "0" - disable, "1" - enabled
    EXPECT_TRUE(setProperty(PROP_FONT_KERNING_ENABLED, "0"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "kerning.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "kerning/01-hinting-no,shaping-simple,kerning-off.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsShapingSimpleKerningOff");
    CRLog::info("===========================================");
}

TEST_F(TextRenderTests, RenderTestsShapingSimpleKerningOn) {
    CRLog::info("==========================================");
    CRLog::info("Starting RenderTestsShapingSimpleKerningOn");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "FreeSerif"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "130"));
    // "0" - simple, "1" - harfbuzz light, "2" - harfbuzz full
    EXPECT_TRUE(setProperty(PROP_FONT_SHAPING, "0"));
    // "0" - disable, "1" - enabled
    EXPECT_TRUE(setProperty(PROP_FONT_KERNING_ENABLED, "1"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "kerning.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "kerning/02-hinting-no,shaping-simple,kerning-on.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsShapingSimpleKerningOn");
    CRLog::info("==========================================");
}

TEST_F(TextRenderTests, RenderTestsShapingLightKerningOff) {
    CRLog::info("==========================================");
    CRLog::info("Starting RenderTestsShapingLightKerningOff");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "FreeSerif"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "130"));
    // "0" - simple, "1" - harfbuzz light, "2" - harfbuzz full
    EXPECT_TRUE(setProperty(PROP_FONT_SHAPING, "1"));
    // "0" - disable, "1" - enabled
    EXPECT_TRUE(setProperty(PROP_FONT_KERNING_ENABLED, "0"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "kerning.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "kerning/03-hinting-no,shaping-light,kerning-off.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsShapingLightKerningOff");
    CRLog::info("==========================================");
}

TEST_F(TextRenderTests, RenderTestsShapingLightKerningOn) {
    CRLog::info("=========================================");
    CRLog::info("Starting RenderTestsShapingLightKerningOn");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "FreeSerif"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "130"));
    // "0" - simple, "1" - harfbuzz light, "2" - harfbuzz full
    EXPECT_TRUE(setProperty(PROP_FONT_SHAPING, "1"));
    // "0" - disable, "1" - enabled
    EXPECT_TRUE(setProperty(PROP_FONT_KERNING_ENABLED, "1"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "kerning.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "kerning/04-hinting-no,shaping-light,kerning-on.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsShapingLightKerningOn");
    CRLog::info("=========================================");
}

TEST_F(TextRenderTests, RenderTestsShapingFullKerningOff) {
    CRLog::info("=========================================");
    CRLog::info("Starting RenderTestsShapingFullKerningOff");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "FreeSerif"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "130"));
    // "0" - simple, "1" - harfbuzz light, "2" - harfbuzz full
    EXPECT_TRUE(setProperty(PROP_FONT_SHAPING, "2"));
    // "0" - disable, "1" - enabled
    EXPECT_TRUE(setProperty(PROP_FONT_KERNING_ENABLED, "0"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "kerning.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "kerning/05-hinting-no,shaping-full,kerning-off.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsShapingFullKerningOff");
    CRLog::info("=========================================");
}

TEST_F(TextRenderTests, RenderTestsShapingFullKerningOn) {
    CRLog::info("========================================");
    CRLog::info("Starting RenderTestsShapingFullKerningOn");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "FreeSerif"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "130"));
    // "0" - simple, "1" - harfbuzz light, "2" - harfbuzz full
    EXPECT_TRUE(setProperty(PROP_FONT_SHAPING, "2"));
    // "0" - disable, "1" - enabled
    EXPECT_TRUE(setProperty(PROP_FONT_KERNING_ENABLED, "1"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "kerning.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "kerning/06-hinting-no,shaping-full,kerning-on.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsShapingFullKerningOn");
    CRLog::info("========================================");
}

TEST_F(TextRenderTests, RenderTestsAAGrayTextGamma_0_7) {
    CRLog::info("=======================================");
    CRLog::info("Starting RenderTestsAAGrayTextGamma_0_7");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "FreeSerif"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "20"));
    EXPECT_TRUE(setProperty(PROP_FONT_ANTIALIASING, "3")); // "3" - AA Gray
    EXPECT_TRUE(setProperty(PROP_FONT_GAMMA, "0.7"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "hello_fb2.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "font-gamma/01-aa-gray,gamma-0_7.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsAAGrayTextGamma_0_7");
    CRLog::info("=======================================");
}

TEST_F(TextRenderTests, RenderTestsAAGrayTextGamma_1_0) {
    CRLog::info("=======================================");
    CRLog::info("Starting RenderTestsAAGrayTextGamma_1_0");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "FreeSerif"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "20"));
    EXPECT_TRUE(setProperty(PROP_FONT_ANTIALIASING, "3")); // "3" - AA Gray
    EXPECT_TRUE(setProperty(PROP_FONT_GAMMA, "1.0"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "hello_fb2.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "font-gamma/02-aa-gray,gamma-1_0.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsAAGrayTextGamma_1_0");
    CRLog::info("=======================================");
}

TEST_F(TextRenderTests, RenderTestsAAGrayTextGamma_1_7) {
    CRLog::info("=======================================");
    CRLog::info("Starting RenderTestsAAGrayTextGamma_1_7");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "FreeSerif"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "20"));
    EXPECT_TRUE(setProperty(PROP_FONT_ANTIALIASING, "3")); // "3" - AA Gray
    EXPECT_TRUE(setProperty(PROP_FONT_GAMMA, "1.7"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "hello_fb2.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "font-gamma/03-aa-gray,gamma-1_7.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsAAGrayTextGamma_1_7");
    CRLog::info("=======================================");
}

TEST_F(TextRenderTests, RenderTestsSynthWeight100) {
    CRLog::info("==================================");
    CRLog::info("Starting RenderTestsSynthWeight100");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "Roboto"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "80"));
    EXPECT_TRUE(setProperty(PROP_FONT_BASE_WEIGHT, "100"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "hello_fb2.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "font-synth-weight/01-roboto-synth-weight-100.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsSynthWeight100");
    CRLog::info("==================================");
}

TEST_F(TextRenderTests, RenderTestsSynthWeight250) {
    CRLog::info("==================================");
    CRLog::info("Starting RenderTestsSynthWeight250");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "Roboto"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "80"));
    EXPECT_TRUE(setProperty(PROP_FONT_BASE_WEIGHT, "250"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "hello_fb2.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "font-synth-weight/02-roboto-synth-weight-250.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsSynthWeight250");
    CRLog::info("==================================");
}

TEST_F(TextRenderTests, RenderTestsRealWeight400) {
    CRLog::info("=================================");
    CRLog::info("Starting RenderTestsRealWeight400");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "Roboto"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "80"));
    EXPECT_TRUE(setProperty(PROP_FONT_BASE_WEIGHT, "400"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "hello_fb2.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "font-synth-weight/03-roboto-real-weight-400.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsRealWeight400");
    CRLog::info("=================================");
}

TEST_F(TextRenderTests, RenderTestsSynthWeight650) {
    CRLog::info("==================================");
    CRLog::info("Starting RenderTestsSynthWeight650");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "Roboto"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "80"));
    EXPECT_TRUE(setProperty(PROP_FONT_BASE_WEIGHT, "650"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "hello_fb2.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "font-synth-weight/04-roboto-synth-weight-650.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsSynthWeight650");
    CRLog::info("==================================");
}

TEST_F(TextRenderTests, RenderTestsSynthWeight900) {
    CRLog::info("==================================");
    CRLog::info("Starting RenderTestsSynthWeight900");
    ASSERT_TRUE(m_initOK);

    // set properties
    EXPECT_TRUE(setProperty(PROP_FONT_FACE, "Roboto"));
    EXPECT_TRUE(setProperty(PROP_FONT_SIZE, "80"));
    EXPECT_TRUE(setProperty(PROP_FONT_BASE_WEIGHT, "900"));

    // open document
    bool res = m_view->LoadDocument(TESTS_DATADIR "hello_fb2.fb2");
    EXPECT_TRUE(res); // load document
    LVDocImageRef image = m_view->getPageImage(0);
    EXPECT_FALSE(image.isNull());
    LVDrawBuf* drawBuf = image->getDrawBuf();
    res = crengine_ng::unittesting::saveToBMP("output.bmp", drawBuf);
    ASSERT_TRUE(res);

    // compare output file with reference
    EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinFiles(RENDER_REFERENCE_DIR "font-synth-weight/05-roboto-synth-weight-900.bmp.gz", "output.bmp"));

    CRLog::info("Finished RenderTestsSynthWeight900");
    CRLog::info("==================================");
}

#endif // USE_FREETYPE == 1 && BUILD_LITE != 1
