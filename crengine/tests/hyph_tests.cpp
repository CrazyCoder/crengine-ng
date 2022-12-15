/**
 *  crengine-ng, unittest, hypenation tests.
 *  Copyright (c) 2022 Aleksey Chernov
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

#include <crhyphman.h>
#include <lvfnt.h>
#include <crlog.h>

#include "../src/textlang.h"

#include "gtest/gtest.h"

#ifndef TESTS_DATADIR
#error Please define TESTS_DATADIR, which points to the directory with the data files for the tests
#endif

#define MAX_WORD_SIZE 64

// Fixtures

class HyphenationTests: public testing::Test
{
protected:
    lString8 doHyphenation(HyphMethod* method, const char* word_utf8) {
        lUInt16 widths[MAX_WORD_SIZE + 1];
        lUInt16 flags[MAX_WORD_SIZE + 1];
        const lUInt16 hyphCharWidth = 5;
        const lUInt16 maxWidth = 10000;
        memset(widths, 0, sizeof(widths));
        memset(flags, 0, sizeof(flags));
        lString32 word = Utf8ToUnicode(word_utf8);

        int len = word.length();
        if (len > MAX_WORD_SIZE) {
            EXPECT_LE(len, MAX_WORD_SIZE);
            return lString8::empty_str;
        }
        for (int i = 0; i < len; i++) {
            widths[i] = 5;
            flags[i] = 0;
        }
        if (method->hyphenate(word.c_str(), len, widths, (lUInt8*)flags, hyphCharWidth, maxWidth, 2)) {
            lChar32 hyphenated[MAX_WORD_SIZE * 2 + 1];
            memset(hyphenated, 0, sizeof(hyphenated));
            int idx = 0;
            for (int i = 0; i < len; i++) {
                hyphenated[idx] = word[i];
                idx++;
                if (flags[i] & LCHAR_ALLOW_HYPH_WRAP_AFTER) {
                    hyphenated[idx] = '-';
                    idx++;
                }
            }
            return UnicodeToUtf8(hyphenated, idx);
        }
        return lString8(word_utf8);
    }
};

// units tests

// This does NOT test the hyphenation dictionary at all.
// This only tests the hyphenation algorithm using en-US dictionary.

TEST_F(HyphenationTests, HyphTestEnglishUS) {
    CRLog::info("===========================");
    CRLog::info("Starting HyphTestEnglishUS");

    HyphMethod* method = HyphMan::getHyphMethodForLang(cs32("en"));
    ASSERT_NE(method, nullptr);
    ASSERT_GT(method->getPatternsCount(), 0);

    EXPECT_STREQ(doHyphenation(method, "conversations").c_str(), "con-ver-sa-tions");
    EXPECT_STREQ(doHyphenation(method, "pleasure").c_str(), "plea-sure");
    EXPECT_STREQ(doHyphenation(method, "considering").c_str(), "con-sid-er-ing");
    EXPECT_STREQ(doHyphenation(method, "picture").c_str(), "pic-ture");
    EXPECT_STREQ(doHyphenation(method, "practice").c_str(), "prac-tice");
    EXPECT_STREQ(doHyphenation(method, "moment").c_str(), "mo-ment");
    EXPECT_STREQ(doHyphenation(method, "trying").c_str(), "try-ing");
    EXPECT_STREQ(doHyphenation(method, "shoulders").c_str(), "shoul-ders");
    EXPECT_STREQ(doHyphenation(method, "remember").c_str(), "re-mem-ber");
    EXPECT_STREQ(doHyphenation(method, "poison").c_str(), "poi-son");
    EXPECT_STREQ(doHyphenation(method, "however").c_str(), "how-ever");
    EXPECT_STREQ(doHyphenation(method, "history").c_str(), "his-tory");
    EXPECT_STREQ(doHyphenation(method, "natural").c_str(), "nat-ural");
    EXPECT_STREQ(doHyphenation(method, "submitted").c_str(), "sub-mit-ted");
    EXPECT_STREQ(doHyphenation(method, "insolence").c_str(), "in-so-lence");
    EXPECT_STREQ(doHyphenation(method, "finger").c_str(), "fin-ger");
    EXPECT_STREQ(doHyphenation(method, "looking").c_str(), "look-ing");
    EXPECT_STREQ(doHyphenation(method, "walking").c_str(), "walk-ing");
    EXPECT_STREQ(doHyphenation(method, "melancholy").c_str(), "melan-choly");
    EXPECT_STREQ(doHyphenation(method, "word").c_str(), "word");

    // Some exceptions from dictionary
    EXPECT_STREQ(doHyphenation(method, "associate").c_str(), "as-so-ciate");
    EXPECT_STREQ(doHyphenation(method, "associates").c_str(), "as-so-ciates");
    EXPECT_STREQ(doHyphenation(method, "declination").c_str(), "dec-li-na-tion");
    EXPECT_STREQ(doHyphenation(method, "obligatory").c_str(), "oblig-a-tory");
    EXPECT_STREQ(doHyphenation(method, "philanthropic").c_str(), "phil-an-thropic");
    EXPECT_STREQ(doHyphenation(method, "present").c_str(), "present");
    EXPECT_STREQ(doHyphenation(method, "presents").c_str(), "presents");
    EXPECT_STREQ(doHyphenation(method, "project").c_str(), "project");
    EXPECT_STREQ(doHyphenation(method, "projects").c_str(), "projects");
    EXPECT_STREQ(doHyphenation(method, "reciprocity").c_str(), "reci-procity");
    EXPECT_STREQ(doHyphenation(method, "recognizance").c_str(), "re-cog-ni-zance");
    EXPECT_STREQ(doHyphenation(method, "reformation").c_str(), "ref-or-ma-tion");
    EXPECT_STREQ(doHyphenation(method, "retribution").c_str(), "ret-ri-bu-tion");
    EXPECT_STREQ(doHyphenation(method, "table").c_str(), "ta-ble");

    CRLog::info("Finished HyphTestEnglishUS");
    CRLog::info("==========================");
}

TEST_F(HyphenationTests, HyphTestEnglishGB) {
    CRLog::info("===========================");
    CRLog::info("Starting HyphTestEnglishGB");

    HyphMethod* method = HyphMan::getHyphMethodForLang(cs32("en-GB"));
    ASSERT_NE(method, nullptr);
    ASSERT_GT(method->getPatternsCount(), 0);

    EXPECT_STREQ(doHyphenation(method, "conversations").c_str(), "con-ver-sa-tions");
    EXPECT_STREQ(doHyphenation(method, "pleasure").c_str(), "pleas-ure");
    EXPECT_STREQ(doHyphenation(method, "considering").c_str(), "con-sid-er-ing");
    EXPECT_STREQ(doHyphenation(method, "picture").c_str(), "pic-ture");
    EXPECT_STREQ(doHyphenation(method, "practice").c_str(), "prac-tice");
    EXPECT_STREQ(doHyphenation(method, "moment").c_str(), "mo-ment");
    EXPECT_STREQ(doHyphenation(method, "trying").c_str(), "try-ing");
    EXPECT_STREQ(doHyphenation(method, "shoulders").c_str(), "shoulders");
    EXPECT_STREQ(doHyphenation(method, "remember").c_str(), "re-mem-ber");
    EXPECT_STREQ(doHyphenation(method, "poison").c_str(), "poison");
    EXPECT_STREQ(doHyphenation(method, "however").c_str(), "how-ever");
    EXPECT_STREQ(doHyphenation(method, "history").c_str(), "his-tory");
    EXPECT_STREQ(doHyphenation(method, "natural").c_str(), "nat-ural");
    EXPECT_STREQ(doHyphenation(method, "submitted").c_str(), "sub-mit-ted");
    EXPECT_STREQ(doHyphenation(method, "insolence").c_str(), "in-solence");
    EXPECT_STREQ(doHyphenation(method, "finger").c_str(), "fin-ger");
    EXPECT_STREQ(doHyphenation(method, "looking").c_str(), "look-ing");
    EXPECT_STREQ(doHyphenation(method, "walking").c_str(), "walk-ing");
    EXPECT_STREQ(doHyphenation(method, "melancholy").c_str(), "mel-an-choly");
    EXPECT_STREQ(doHyphenation(method, "word").c_str(), "word");

    // Some exceptions from dictionary
    EXPECT_STREQ(doHyphenation(method, "university").c_str(), "uni-ver-sity");
    EXPECT_STREQ(doHyphenation(method, "universities").c_str(), "uni-ver-sit-ies");
    EXPECT_STREQ(doHyphenation(method, "manuscript").c_str(), "ma-nu-script");
    EXPECT_STREQ(doHyphenation(method, "manuscripts").c_str(), "ma-nu-scripts");
    EXPECT_STREQ(doHyphenation(method, "reciprocity").c_str(), "re-ci-pro-city");
    EXPECT_STREQ(doHyphenation(method, "throughout").c_str(), "through-out");
    EXPECT_STREQ(doHyphenation(method, "something").c_str(), "some-thing");

    CRLog::info("Finished HyphTestEnglishGB");
    CRLog::info("==========================");
}

TEST_F(HyphenationTests, HyphTestRussian) {
    CRLog::info("===========================");
    CRLog::info("Starting HyphTestEnglishRU");

    HyphMethod* method = HyphMan::getHyphMethodForLang(cs32("ru"));
    ASSERT_NE(method, nullptr);
    ASSERT_GT(method->getPatternsCount(), 0);

    EXPECT_STREQ(doHyphenation(method, "аквариум").c_str(), "ак-ва-ри-ум");
    EXPECT_STREQ(doHyphenation(method, "каблук").c_str(), "ка-блук");
    EXPECT_STREQ(doHyphenation(method, "осуществил").c_str(), "осу-ще-ствил");
    EXPECT_STREQ(doHyphenation(method, "ахгъ").c_str(), "ах-гъ");
    EXPECT_STREQ(doHyphenation(method, "акведук").c_str(), "ак-ве-дук");
    EXPECT_STREQ(doHyphenation(method, "угар").c_str(), "угар");
    EXPECT_STREQ(doHyphenation(method, "жужжать").c_str(), "жуж-жать");
    EXPECT_STREQ(doHyphenation(method, "масса").c_str(), "мас-са");
    EXPECT_STREQ(doHyphenation(method, "конный").c_str(), "кон-ный");
    EXPECT_STREQ(doHyphenation(method, "одежда").c_str(), "оде-жда");
    EXPECT_STREQ(doHyphenation(method, "просмотр").c_str(), "про-смотр");
    EXPECT_STREQ(doHyphenation(method, "кленовый").c_str(), "кле-но-вый");
    EXPECT_STREQ(doHyphenation(method, "подбегать").c_str(), "под-бе-гать");
    EXPECT_STREQ(doHyphenation(method, "прикрыть").c_str(), "при-крыть");
    EXPECT_STREQ(doHyphenation(method, "девятиграммовый").c_str(), "де-вя-ти-грам-мо-вый");
    EXPECT_STREQ(doHyphenation(method, "спецслужба").c_str(), "спец-служ-ба");
    EXPECT_STREQ(doHyphenation(method, "бойница").c_str(), "бой-ни-ца");
    EXPECT_STREQ(doHyphenation(method, "разыграть").c_str(), "ра-зы-грать");

    // In crengine-ng, CoolReader, KOReader from Russian hyphenation dictionary (and maybe other too)
    //  removed hack that prevents breaking after hyphen, so next test is failed.
    //  But in reality, before the text gets into the HyphMan module to break the text for hyphens,
    //  the already existing hyphens have been removed by this time.
    //  So this hack is not needed.
    //EXPECT_STREQ(doHyphenation(method, "аква-риум").c_str(), "ак-ва-ри-ум");

    // Abbreviations cannot be breaks in Russian (and some other languages) when hyphenated,
    //  but this is not implemented.
    //EXPECT_STREQ(doHyphenation(method, "ЧАВО").c_str(), "ЧАВО");

    CRLog::info("Finished HyphTestEnglishRU");
    CRLog::info("==========================");
}

TEST_F(HyphenationTests, SimpleHyphTest) {
    CRLog::info("=======================");
    CRLog::info("Starting SimpleHyphTest");

    HyphDictionary* dict;
    HyphMethod* method;

    // Add test dictionaries manually using `HyphMan::addDictionaryItem()`.

    // Dictionary with one pattern 'n1v2'.
    dict = new HyphDictionary(HDT_DICT_TEX, cs32("testhyph1"), cs32("id=testhyph1.pattern"), cs32("en-testhyph1"), cs32(TESTS_DATADIR "test-hyph/testhyph1.pattern"));
    ASSERT_TRUE(HyphMan::addDictionaryItem(dict));
    method = HyphMan::getHyphMethodForDictionary(cs32("id=testhyph1.pattern"));
    ASSERT_NE(method, nullptr);
    ASSERT_GT(method->getPatternsCount(), 0);
    EXPECT_STREQ(doHyphenation(method, "conversations").c_str(), "con-versations");

    // Dictionary with patterns 'n1v2', 'on2v2'.
    dict = new HyphDictionary(HDT_DICT_TEX, cs32("testhyph2"), cs32("id=testhyph2.pattern"), cs32("en-testhyph2"), cs32(TESTS_DATADIR "test-hyph/testhyph2.pattern"));
    ASSERT_TRUE(HyphMan::addDictionaryItem(dict));
    method = HyphMan::getHyphMethodForDictionary(cs32("id=testhyph2.pattern"));
    ASSERT_NE(method, nullptr);
    ASSERT_GT(method->getPatternsCount(), 0);
    EXPECT_STREQ(doHyphenation(method, "conversations").c_str(), "conversations");

    // Dictionary with patterns 'n1v2', 'on2v2', con1v2.
    dict = new HyphDictionary(HDT_DICT_TEX, cs32("testhyph3"), cs32("id=testhyph3.pattern"), cs32("en-testhyph3"), cs32(TESTS_DATADIR "test-hyph/testhyph3.pattern"));
    ASSERT_TRUE(HyphMan::addDictionaryItem(dict));
    method = HyphMan::getHyphMethodForDictionary(cs32("id=testhyph3.pattern"));
    ASSERT_NE(method, nullptr);
    ASSERT_GT(method->getPatternsCount(), 0);
    EXPECT_STREQ(doHyphenation(method, "conversations").c_str(), "conversations");

    // Dictionary with patterns 'n1v2', 'on2v2', con3v2.
    dict = new HyphDictionary(HDT_DICT_TEX, cs32("testhyph4"), cs32("id=testhyph4.pattern"), cs32("en-testhyph4"), cs32(TESTS_DATADIR "test-hyph/testhyph4.pattern"));
    ASSERT_TRUE(HyphMan::addDictionaryItem(dict));
    method = HyphMan::getHyphMethodForDictionary(cs32("id=testhyph4.pattern"));
    ASSERT_NE(method, nullptr);
    ASSERT_GT(method->getPatternsCount(), 0);
    EXPECT_STREQ(doHyphenation(method, "conversations").c_str(), "con-versations");

    CRLog::info("Finished SimpleHyphTest");
    CRLog::info("=======================");
}

TEST_F(HyphenationTests, GetHyphMethodForDictTest) {
    CRLog::info("=================================");
    CRLog::info("Starting GetHyphMethodForDictTest");

    HyphMethod* methodNone = HyphMan::getHyphMethodForDictionary(cs32(HYPH_DICT_ID_NONE));
    HyphMethod* methodAlgo = HyphMan::getHyphMethodForDictionary(cs32(HYPH_DICT_ID_ALGORITHM));
    HyphMethod* methodSoftHyphens = HyphMan::getHyphMethodForDictionary(cs32(HYPH_DICT_ID_SOFTHYPHENS));
    HyphMethod* methodDict_EN_US = HyphMan::getHyphMethodForDictionary(cs32("hyph-en-us.pattern"));
    HyphMethod* methodDict_INV = HyphMan::getHyphMethodForDictionary(cs32("_invalidX.XYZ"));

    ASSERT_NE(methodNone, nullptr);
    EXPECT_EQ(methodNone->getPatternsCount(), 0);
    EXPECT_NE(methodNone, methodAlgo);
    EXPECT_NE(methodNone, methodSoftHyphens);
    EXPECT_NE(methodNone, methodDict_EN_US);

    ASSERT_NE(methodAlgo, nullptr);
    EXPECT_EQ(methodAlgo->getPatternsCount(), 0);
    EXPECT_NE(methodAlgo, methodNone);
    EXPECT_NE(methodAlgo, methodSoftHyphens);
    EXPECT_NE(methodAlgo, methodDict_EN_US);

    ASSERT_NE(methodSoftHyphens, nullptr);
    EXPECT_EQ(methodSoftHyphens->getPatternsCount(), 0);
    EXPECT_NE(methodSoftHyphens, methodNone);
    EXPECT_NE(methodSoftHyphens, methodAlgo);
    EXPECT_NE(methodSoftHyphens, methodDict_EN_US);

    ASSERT_NE(methodDict_EN_US, nullptr);
    EXPECT_GT(methodDict_EN_US->getPatternsCount(), 3000);
    EXPECT_NE(methodDict_EN_US, methodNone);
    EXPECT_NE(methodDict_EN_US, methodAlgo);
    EXPECT_NE(methodDict_EN_US, methodSoftHyphens);

    EXPECT_NE(methodDict_INV, nullptr);
    EXPECT_EQ(methodDict_INV, methodNone);

    CRLog::info("Finished GetHyphMethodForDictTest");
    CRLog::info("=================================");
}

TEST_F(HyphenationTests, GetHyphMethodForLangTest) {
    CRLog::info("=================================");
    CRLog::info("Starting GetHyphMethodForLangTest");

    HyphMethod* methodNone = HyphMan::getHyphMethodForDictionary(cs32(HYPH_DICT_ID_NONE));
    ASSERT_NE(methodNone, nullptr);
    ASSERT_EQ(methodNone->getPatternsCount(), 0);

    HyphMethod* methodDict_EN = HyphMan::getHyphMethodForLang(cs32("en"));
    ASSERT_NE(methodDict_EN, nullptr);
    EXPECT_NE(methodDict_EN, methodNone);
    EXPECT_GT(methodDict_EN->getPatternsCount(), 3000);

    HyphMethod* methodDict_EN_US = HyphMan::getHyphMethodForLang(cs32("en-US"));
    ASSERT_NE(methodDict_EN_US, nullptr);
    EXPECT_NE(methodDict_EN_US, methodNone);
    EXPECT_GT(methodDict_EN_US->getPatternsCount(), 3000);

    // 'en' is alias for 'en-US'
    ASSERT_EQ(methodDict_EN, methodDict_EN_US);

    HyphMethod* methodDict_EN_GB = HyphMan::getHyphMethodForLang(cs32("en-GB"));
    ASSERT_NE(methodDict_EN_GB, nullptr);
    EXPECT_NE(methodDict_EN_GB, methodNone);
    EXPECT_NE(methodDict_EN_GB, methodDict_EN_US);
    EXPECT_GT(methodDict_EN_GB->getPatternsCount(), 6000);

    HyphMethod* methodDict_EL = HyphMan::getHyphMethodForLang(cs32("el"));
    ASSERT_NE(methodDict_EL, nullptr);
    EXPECT_NE(methodDict_EL, methodNone);
    EXPECT_NE(methodDict_EL, methodDict_EN_US);
    EXPECT_NE(methodDict_EL, methodDict_EN_GB);
    EXPECT_GT(methodDict_EL->getPatternsCount(), 500);

    HyphMethod* methodDict_invalid = HyphMan::getHyphMethodForLang(cs32("inv?-xINV"));
    ASSERT_NE(methodDict_invalid, nullptr);
    EXPECT_EQ(methodDict_invalid, methodNone);

    CRLog::info("Finished GetHyphMethodForLangTest");
    CRLog::info("=================================");
}

TEST_F(HyphenationTests, HyphTestOverrideHyphenMinTest) {
    CRLog::info("======================================");
    CRLog::info("Starting HyphTestOverrideHyphenMinTest");

    HyphMethod* method = HyphMan::getHyphMethodForLang(cs32("en"));
    ASSERT_NE(method, nullptr);
    ASSERT_GT(method->getPatternsCount(), 3000);

    // Override left & right hypnenmins
    HyphMan::overrideLeftHyphenMin(1);
    HyphMan::overrideRightHyphenMin(1);

    EXPECT_STREQ(doHyphenation(method, "conversations").c_str(), "con-ver-sa-tion-s");
    EXPECT_STREQ(doHyphenation(method, "moment").c_str(), "mo-men-t");
    EXPECT_STREQ(doHyphenation(method, "shoulders").c_str(), "shoul-der-s");
    EXPECT_STREQ(doHyphenation(method, "however").c_str(), "how-ev-er");
    EXPECT_STREQ(doHyphenation(method, "history").c_str(), "his-to-ry");
    EXPECT_STREQ(doHyphenation(method, "natural").c_str(), "nat-ur-al");

    // Set no override for left & right hypnenmins
    HyphMan::overrideLeftHyphenMin(0);
    HyphMan::overrideRightHyphenMin(0);

    EXPECT_STREQ(doHyphenation(method, "conversations").c_str(), "con-ver-sa-tions");
    EXPECT_STREQ(doHyphenation(method, "moment").c_str(), "mo-ment");
    EXPECT_STREQ(doHyphenation(method, "shoulders").c_str(), "shoul-ders");
    EXPECT_STREQ(doHyphenation(method, "however").c_str(), "how-ever");
    EXPECT_STREQ(doHyphenation(method, "history").c_str(), "his-tory");
    EXPECT_STREQ(doHyphenation(method, "natural").c_str(), "nat-ural");

    CRLog::info("Finished HyphTestOverrideHyphenMinTest");
    CRLog::info("======================================");
}

TEST_F(HyphenationTests, TextLangSetMainLangTest) {
    CRLog::info("================================");
    CRLog::info("Starting TextLangSetMainLangTest");

    TextLangMan::setHyphenationEnabled(true);
    TextLangMan::setHyphenationForceAlgorithmic(false);
    TextLangMan::setHyphenationSoftHyphensOnly(false);
    TextLangMan::setMainLang(U"en-GB");

    HyphMethod* methodNone = HyphMan::getHyphMethodForDictionary(cs32(HYPH_DICT_ID_NONE));
    HyphMethod* methodAlgo = HyphMan::getHyphMethodForDictionary(cs32(HYPH_DICT_ID_ALGORITHM));
    HyphMethod* methodSoftHyphens = HyphMan::getHyphMethodForDictionary(cs32(HYPH_DICT_ID_SOFTHYPHENS));
    HyphMethod* methodDict_en_GB = HyphMan::getHyphMethodForLang(cs32("en-GB"));
    ASSERT_NE(methodDict_en_GB, nullptr);
    ASSERT_NE(methodDict_en_GB, methodNone);
    ASSERT_NE(methodDict_en_GB, methodAlgo);
    ASSERT_NE(methodDict_en_GB, methodSoftHyphens);
    HyphMethod* methodDict_en_US = HyphMan::getHyphMethodForLang(cs32("en"));
    ASSERT_NE(methodDict_en_US, nullptr);
    ASSERT_NE(methodDict_en_US, methodNone);
    ASSERT_NE(methodDict_en_US, methodAlgo);
    ASSERT_NE(methodDict_en_US, methodSoftHyphens);
    HyphMethod* methodDict_RU = HyphMan::getHyphMethodForLang(cs32("ru"));
    ASSERT_NE(methodDict_RU, nullptr);
    ASSERT_NE(methodDict_RU, methodNone);
    ASSERT_NE(methodDict_RU, methodAlgo);
    ASSERT_NE(methodDict_RU, methodSoftHyphens);
    HyphMethod* methodDict_CS = HyphMan::getHyphMethodForLang(cs32("cs"));
    ASSERT_NE(methodDict_CS, nullptr);
    ASSERT_NE(methodDict_CS, methodNone);
    ASSERT_NE(methodDict_CS, methodAlgo);
    ASSERT_NE(methodDict_CS, methodSoftHyphens);

    // Without embedden language
    TextLangMan::setEmbeddedLangsEnabled(false);

    HyphMethod* mainHyphMethod = TextLangMan::getMainLangHyphMethod();
    ASSERT_NE(mainHyphMethod, nullptr);

    TextLangCfg* langCfgEN_US = TextLangMan::getTextLangCfg(U"en");
    ASSERT_NE(langCfgEN_US, nullptr);
    TextLangCfg* langCfgRU = TextLangMan::getTextLangCfg(U"ru");
    ASSERT_NE(langCfgRU, nullptr);
    TextLangCfg* langCfgCS = TextLangMan::getTextLangCfg(U"cs");
    ASSERT_NE(langCfgCS, nullptr);
    TextLangCfg* langCfgAB_CD = TextLangMan::getTextLangCfg(U"ab-cd");
    ASSERT_NE(langCfgAB_CD, nullptr);

    EXPECT_EQ(mainHyphMethod, methodDict_en_GB);
    EXPECT_EQ(langCfgEN_US->getHyphMethod(), methodDict_en_GB);
    EXPECT_EQ(langCfgRU->getHyphMethod(), methodDict_en_GB);
    EXPECT_EQ(langCfgCS->getHyphMethod(), methodDict_en_GB);
    // for unknown language also used main language
    EXPECT_EQ(langCfgAB_CD->getHyphMethod(), methodDict_en_GB);

    // With embedden language enabled
    TextLangMan::setEmbeddedLangsEnabled(true);

    mainHyphMethod = TextLangMan::getMainLangHyphMethod();
    ASSERT_NE(mainHyphMethod, nullptr);

    langCfgEN_US = TextLangMan::getTextLangCfg(U"en");
    ASSERT_NE(langCfgEN_US, nullptr);
    langCfgRU = TextLangMan::getTextLangCfg(U"ru");
    ASSERT_NE(langCfgRU, nullptr);
    langCfgCS = TextLangMan::getTextLangCfg(U"cs");
    ASSERT_NE(langCfgCS, nullptr);
    langCfgAB_CD = TextLangMan::getTextLangCfg(U"ab-cd");
    ASSERT_NE(langCfgAB_CD, nullptr);

    EXPECT_EQ(mainHyphMethod, methodDict_en_GB);
    EXPECT_NE(langCfgEN_US->getHyphMethod(), methodDict_en_GB);
    EXPECT_EQ(langCfgEN_US->getHyphMethod(), methodDict_en_US);
    EXPECT_NE(langCfgRU->getHyphMethod(), methodDict_en_GB);
    EXPECT_EQ(langCfgRU->getHyphMethod(), methodDict_RU);
    EXPECT_NE(langCfgCS->getHyphMethod(), methodDict_en_GB);
    EXPECT_EQ(langCfgCS->getHyphMethod(), methodDict_CS);
    // for unknown language used en-US
    EXPECT_NE(langCfgAB_CD->getHyphMethod(), methodDict_en_GB);
    EXPECT_EQ(langCfgAB_CD->getHyphMethod(), methodDict_en_US);

    CRLog::info("Finished TextLangSetMainLangTest");
    CRLog::info("================================");
}

TEST_F(HyphenationTests, TextLangCfgENTest) {
    CRLog::info("==========================");
    CRLog::info("Starting TextLangCfgENTest");

    TextLangMan::setHyphenationEnabled(true);
    TextLangMan::setHyphenationForceAlgorithmic(false);
    TextLangMan::setHyphenationSoftHyphensOnly(false);
    TextLangMan::setEmbeddedLangsEnabled(true);

    HyphMethod* methodNone = HyphMan::getHyphMethodForDictionary(cs32(HYPH_DICT_ID_NONE));
    HyphMethod* methodAlgo = HyphMan::getHyphMethodForDictionary(cs32(HYPH_DICT_ID_ALGORITHM));
    HyphMethod* methodSoftHyphens = HyphMan::getHyphMethodForDictionary(cs32(HYPH_DICT_ID_SOFTHYPHENS));
    HyphMethod* methodDict_en_US = HyphMan::getHyphMethodForLang(cs32("en"));
    ASSERT_NE(methodDict_en_US, nullptr);
    ASSERT_NE(methodDict_en_US, methodNone);
    ASSERT_NE(methodDict_en_US, methodAlgo);
    ASSERT_NE(methodDict_en_US, methodSoftHyphens);

    TextLangCfg* langCfg = TextLangMan::getTextLangCfg(U"en");

    ASSERT_NE(langCfg, nullptr);
    EXPECT_EQ(langCfg->getHyphMethod(), methodDict_en_US);

    TextLangMan::setMainLang(U"en-GB");

    // Regardless of the main language, we must use the specified one.
    langCfg = TextLangMan::getTextLangCfg(U"en");
    ASSERT_NE(langCfg, nullptr);
    EXPECT_EQ(langCfg->getHyphMethod(), methodDict_en_US);

    CRLog::info("Finished TextLangCfgENTest");
    CRLog::info("==========================");
}

TEST_F(HyphenationTests, TextLangCfgRUTest) {
    CRLog::info("==========================");
    CRLog::info("Starting TextLangCfgRUTest");

    TextLangMan::setHyphenationEnabled(true);
    TextLangMan::setHyphenationForceAlgorithmic(false);
    TextLangMan::setHyphenationSoftHyphensOnly(false);
    TextLangMan::setEmbeddedLangsEnabled(true);

    HyphMethod* methodNone = HyphMan::getHyphMethodForDictionary(cs32(HYPH_DICT_ID_NONE));
    HyphMethod* methodAlgo = HyphMan::getHyphMethodForDictionary(cs32(HYPH_DICT_ID_ALGORITHM));
    HyphMethod* methodSoftHyphens = HyphMan::getHyphMethodForDictionary(cs32(HYPH_DICT_ID_SOFTHYPHENS));
    HyphMethod* methodDict_RU = HyphMan::getHyphMethodForLang(cs32("ru"));
    ASSERT_NE(methodDict_RU, nullptr);
    ASSERT_NE(methodDict_RU, methodNone);
    ASSERT_NE(methodDict_RU, methodAlgo);
    ASSERT_NE(methodDict_RU, methodSoftHyphens);

    TextLangCfg* langCfg = TextLangMan::getTextLangCfg(U"ru");

    ASSERT_NE(langCfg, nullptr);
    EXPECT_EQ(langCfg->getHyphMethod(), methodDict_RU);

    TextLangMan::setMainLang(U"en-GB");

    // Regardless of the main language, we must use the specified one.
    langCfg = TextLangMan::getTextLangCfg(U"ru");
    ASSERT_NE(langCfg, nullptr);
    EXPECT_EQ(langCfg->getHyphMethod(), methodDict_RU);

    CRLog::info("Finished TextLangCfgRUTest");
    CRLog::info("==========================");
}
