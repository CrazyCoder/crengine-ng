/**
 *  crengine-ng
 *  Copyright (c) 2022 Aleksey Chernov
 *
 *  \file localenames_tests.cpp
 *  \brief crengine-ng unit test module.
 *  Tests detection and conversion of locale names.
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

#include <crsetup.h>

#if USE_LOCALE_DATA == 1

#include <crlog.h>
#include <crlocaledata.h>

#include "gtest/gtest.h"

// units tests

TEST(LocaleDataTests, TestLocaleParsing) {
    CRLog::info("==========================");
    CRLog::info("Starting TestLocaleParsing");

    // English codes variants
    {
        CRLocaleData loc("English");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "English");
        EXPECT_STREQ(loc.langTag().c_str(), "en");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_TRUE(loc.regionAlpha2().empty());
        EXPECT_TRUE(loc.regionAlpha3().empty());
        EXPECT_TRUE(loc.regionName().empty());
        EXPECT_EQ(loc.regionNumeric(), 0);
    }
    {
        CRLocaleData loc("english");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "English");
        EXPECT_STREQ(loc.langTag().c_str(), "en");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_TRUE(loc.regionAlpha2().empty());
        EXPECT_TRUE(loc.regionAlpha3().empty());
        EXPECT_TRUE(loc.regionName().empty());
        EXPECT_EQ(loc.regionNumeric(), 0);
    }
    {
        CRLocaleData loc("eng");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "English");
        EXPECT_STREQ(loc.langTag().c_str(), "en");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_TRUE(loc.regionAlpha2().empty());
        EXPECT_TRUE(loc.regionAlpha3().empty());
        EXPECT_TRUE(loc.regionName().empty());
        EXPECT_EQ(loc.regionNumeric(), 0);
    }
    {
        CRLocaleData loc("Eng");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "English");
        EXPECT_STREQ(loc.langTag().c_str(), "en");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_TRUE(loc.regionAlpha2().empty());
        EXPECT_TRUE(loc.regionAlpha3().empty());
        EXPECT_TRUE(loc.regionName().empty());
        EXPECT_EQ(loc.regionNumeric(), 0);
    }
    {
        CRLocaleData loc("en");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "English");
        EXPECT_STREQ(loc.langTag().c_str(), "en");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_TRUE(loc.regionAlpha2().empty());
        EXPECT_TRUE(loc.regionAlpha3().empty());
        EXPECT_TRUE(loc.regionName().empty());
        EXPECT_EQ(loc.regionNumeric(), 0);
    }
    {
        CRLocaleData loc("en-US");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "English");
        EXPECT_STREQ(loc.langTag().c_str(), "en-US");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "US");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "USA");
        EXPECT_STREQ(loc.regionName().c_str(), "United States of America (the)");
        EXPECT_EQ(loc.regionNumeric(), 840);
    }
    {
        CRLocaleData loc("eng-US");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "English");
        EXPECT_STREQ(loc.langTag().c_str(), "en-US");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "US");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "USA");
        EXPECT_STREQ(loc.regionName().c_str(), "United States of America (the)");
        EXPECT_EQ(loc.regionNumeric(), 840);
    }
    {
        CRLocaleData loc("en-USA");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "English");
        EXPECT_STREQ(loc.langTag().c_str(), "en-US");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "US");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "USA");
        EXPECT_STREQ(loc.regionName().c_str(), "United States of America (the)");
        EXPECT_EQ(loc.regionNumeric(), 840);
    }
    {
        CRLocaleData loc("eng-USA");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "English");
        EXPECT_STREQ(loc.langTag().c_str(), "en-US");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "US");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "USA");
        EXPECT_STREQ(loc.regionName().c_str(), "United States of America (the)");
        EXPECT_EQ(loc.regionNumeric(), 840);
    }
    {
        CRLocaleData loc("en-GB");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "English");
        EXPECT_STREQ(loc.langTag().c_str(), "en-GB");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "GB");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "GBR");
        EXPECT_STREQ(loc.regionName().c_str(), "United Kingdom of Great Britain and Northern Ireland (the)");
        EXPECT_EQ(loc.regionNumeric(), 826);
    }
    {
        CRLocaleData loc("eng-GB");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "English");
        EXPECT_STREQ(loc.langTag().c_str(), "en-GB");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "GB");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "GBR");
        EXPECT_STREQ(loc.regionName().c_str(), "United Kingdom of Great Britain and Northern Ireland (the)");
        EXPECT_EQ(loc.regionNumeric(), 826);
    }
    {
        CRLocaleData loc("en-GBR");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "English");
        EXPECT_STREQ(loc.langTag().c_str(), "en-GB");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "GB");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "GBR");
        EXPECT_STREQ(loc.regionName().c_str(), "United Kingdom of Great Britain and Northern Ireland (the)");
        EXPECT_EQ(loc.regionNumeric(), 826);
    }
    {
        CRLocaleData loc("eng-GBR");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "English");
        EXPECT_STREQ(loc.langTag().c_str(), "en-GB");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "GB");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "GBR");
        EXPECT_STREQ(loc.regionName().c_str(), "United Kingdom of Great Britain and Northern Ireland (the)");
        EXPECT_EQ(loc.regionNumeric(), 826);
    }
    {
        CRLocaleData loc("en_gb");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "English");
        EXPECT_STREQ(loc.langTag().c_str(), "en-GB");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "GB");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "GBR");
        EXPECT_STREQ(loc.regionName().c_str(), "United Kingdom of Great Britain and Northern Ireland (the)");
        EXPECT_EQ(loc.regionNumeric(), 826);
    }

    // Russian codes variants
    {
        CRLocaleData loc("Russian");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "Russian");
        EXPECT_STREQ(loc.langTag().c_str(), "ru");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_TRUE(loc.regionAlpha2().empty());
        EXPECT_TRUE(loc.regionAlpha3().empty());
        EXPECT_TRUE(loc.regionName().empty());
        EXPECT_EQ(loc.regionNumeric(), 0);
    }
    {
        CRLocaleData loc("russian");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "Russian");
        EXPECT_STREQ(loc.langTag().c_str(), "ru");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_TRUE(loc.regionAlpha2().empty());
        EXPECT_TRUE(loc.regionAlpha3().empty());
        EXPECT_TRUE(loc.regionName().empty());
        EXPECT_EQ(loc.regionNumeric(), 0);
    }
    {
        CRLocaleData loc("Rus");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "Russian");
        EXPECT_STREQ(loc.langTag().c_str(), "ru");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_TRUE(loc.regionAlpha2().empty());
        EXPECT_TRUE(loc.regionAlpha3().empty());
        EXPECT_TRUE(loc.regionName().empty());
        EXPECT_EQ(loc.regionNumeric(), 0);
    }
    {
        CRLocaleData loc("rus");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "Russian");
        EXPECT_STREQ(loc.langTag().c_str(), "ru");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_TRUE(loc.regionAlpha2().empty());
        EXPECT_TRUE(loc.regionAlpha3().empty());
        EXPECT_TRUE(loc.regionName().empty());
        EXPECT_EQ(loc.regionNumeric(), 0);
    }
    {
        CRLocaleData loc("ru");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "Russian");
        EXPECT_STREQ(loc.langTag().c_str(), "ru");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_TRUE(loc.regionAlpha2().empty());
        EXPECT_TRUE(loc.regionAlpha3().empty());
        EXPECT_TRUE(loc.regionName().empty());
        EXPECT_EQ(loc.regionNumeric(), 0);
    }
    {
        CRLocaleData loc("ru-RU");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "Russian");
        EXPECT_STREQ(loc.langTag().c_str(), "ru-RU");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "RU");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "RUS");
        EXPECT_STREQ(loc.regionName().c_str(), "Russian Federation (the)");
        EXPECT_EQ(loc.regionNumeric(), 643);
    }
    {
        CRLocaleData loc("rus-RU");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "Russian");
        EXPECT_STREQ(loc.langTag().c_str(), "ru-RU");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "RU");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "RUS");
        EXPECT_STREQ(loc.regionName().c_str(), "Russian Federation (the)");
        EXPECT_EQ(loc.regionNumeric(), 643);
    }
    {
        CRLocaleData loc("ru-RUS");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "Russian");
        EXPECT_STREQ(loc.langTag().c_str(), "ru-RU");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "RU");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "RUS");
        EXPECT_STREQ(loc.regionName().c_str(), "Russian Federation (the)");
        EXPECT_EQ(loc.regionNumeric(), 643);
    }
    {
        CRLocaleData loc("rus-RUS");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "Russian");
        EXPECT_STREQ(loc.langTag().c_str(), "ru-RU");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "RU");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "RUS");
        EXPECT_STREQ(loc.regionName().c_str(), "Russian Federation (the)");
        EXPECT_EQ(loc.regionNumeric(), 643);
    }
    {
        CRLocaleData loc("ru_ru");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "Russian");
        EXPECT_STREQ(loc.langTag().c_str(), "ru-RU");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "RU");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "RUS");
        EXPECT_STREQ(loc.regionName().c_str(), "Russian Federation (the)");
        EXPECT_EQ(loc.regionNumeric(), 643);
    }

    // Chinese variants
    {
        CRLocaleData loc("zh-CN");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "Chinese");
        EXPECT_STREQ(loc.langTag().c_str(), "zh-CN");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "CN");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "CHN");
        EXPECT_STREQ(loc.regionName().c_str(), "China");
        EXPECT_EQ(loc.regionNumeric(), 156);
    }
    {
        CRLocaleData loc("zho-CN");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "Chinese");
        EXPECT_STREQ(loc.langTag().c_str(), "zh-CN");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "CN");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "CHN");
        EXPECT_STREQ(loc.regionName().c_str(), "China");
        EXPECT_EQ(loc.regionNumeric(), 156);
    }
    {
        CRLocaleData loc("zho-CHN");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "Chinese");
        EXPECT_STREQ(loc.langTag().c_str(), "zh-CN");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "CN");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "CHN");
        EXPECT_STREQ(loc.regionName().c_str(), "China");
        EXPECT_EQ(loc.regionNumeric(), 156);
    }
    {
        CRLocaleData loc("zh-HK");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "Chinese");
        EXPECT_STREQ(loc.langTag().c_str(), "zh-HK");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "HK");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "HKG");
        EXPECT_STREQ(loc.regionName().c_str(), "Hong Kong");
        EXPECT_EQ(loc.regionNumeric(), 344);
    }
    {
        CRLocaleData loc("zho-HK");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "Chinese");
        EXPECT_STREQ(loc.langTag().c_str(), "zh-HK");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "HK");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "HKG");
        EXPECT_STREQ(loc.regionName().c_str(), "Hong Kong");
        EXPECT_EQ(loc.regionNumeric(), 344);
    }
    {
        CRLocaleData loc("zh-HKG");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "Chinese");
        EXPECT_STREQ(loc.langTag().c_str(), "zh-HK");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "HK");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "HKG");
        EXPECT_STREQ(loc.regionName().c_str(), "Hong Kong");
        EXPECT_EQ(loc.regionNumeric(), 344);
    }
    {
        CRLocaleData loc("zho-HKG");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "Chinese");
        EXPECT_STREQ(loc.langTag().c_str(), "zh-HK");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "HK");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "HKG");
        EXPECT_STREQ(loc.regionName().c_str(), "Hong Kong");
        EXPECT_EQ(loc.regionNumeric(), 344);
    }
    {
        CRLocaleData loc("zh_hk");
        EXPECT_TRUE(loc.isValid());
        EXPECT_STREQ(loc.langName().c_str(), "Chinese");
        EXPECT_STREQ(loc.langTag().c_str(), "zh-HK");
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_STREQ(loc.regionAlpha2().c_str(), "HK");
        EXPECT_STREQ(loc.regionAlpha3().c_str(), "HKG");
        EXPECT_STREQ(loc.regionName().c_str(), "Hong Kong");
        EXPECT_EQ(loc.regionNumeric(), 344);
    }
    // invalid langTag
    {
        CRLocaleData loc("abcd");
        EXPECT_FALSE(loc.isValid());
        EXPECT_TRUE(loc.scriptCode().empty());
        EXPECT_TRUE(loc.scriptName().empty());
        EXPECT_TRUE(loc.scriptAlias().empty());
        EXPECT_EQ(loc.scriptNumeric(), 0);
        EXPECT_TRUE(loc.regionAlpha2().empty());
        EXPECT_TRUE(loc.regionAlpha3().empty());
        EXPECT_TRUE(loc.regionName().empty());
        EXPECT_EQ(loc.regionNumeric(), 0);
    }

    CRLog::info("Finished TestLocaleParsing");
    CRLog::info("==========================");
}

#endif // USE_LOCALE_DATA==1
