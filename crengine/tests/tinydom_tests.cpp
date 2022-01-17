/**
 *  crengine-ng
 *
 *  \file tinydom_tests.cpp
 *  \brief crengine-ng unit test module.
 *  Tests some tinyDOM stuff.
 *  This tests extracted & adapted from lvtinydom.cpp
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

#include <crlog.h>
#include <lvdocview.h>
#include <lvtinydom.h>
#include <lvstreamutils.h>

#include "lvtinydom_cachefile.h"
#include "lvtinydom_private.h"

#include "gtest/gtest.h"

#ifndef TESTS_DATADIR
#error Please define TESTS_DATADIR, which points to the directory with the data files for the tests
#endif

#ifndef TESTS_TMPDIR
#define TESTS_TMPDIR "/tmp/"
#endif

// Fixtures

class TinyDOMTests: public testing::Test
{
protected:
    bool m_initOK;
protected:
    virtual void SetUp() override {
        m_initOK = LVCreateDirectory(U"cr3cache");
    }

    virtual void TearDown() override {
        LVDeleteFile(U"cr3cache/cr3cache.inx");
        LVDeleteDirectory(U"cr3cache");
    }
};

// units tests

TEST_F(TinyDOMTests, basicTinyDomUnitTests) {
    CRLog::info("==============================");
    CRLog::info("Starting basicTinyDomUnitTests");

    ldomDocument* doc = new ldomDocument();
    ldomNode* root = doc->getRootNode(); //doc->allocTinyElement( NULL, 0, 0 );
    ASSERT_TRUE(root != NULL);

    lUInt16 el_p = doc->getElementNameIndex(U"p");
    lUInt16 el_title = doc->getElementNameIndex(U"title");
    lUInt16 el_strong = doc->getElementNameIndex(U"strong");
    lUInt16 el_emphasis = doc->getElementNameIndex(U"emphasis");
    lUInt16 attr_id = doc->getAttrNameIndex(U"id");
    lUInt16 attr_name = doc->getAttrNameIndex(U"name");
    static lUInt16 path1[] = { el_title, el_p, 0 };
    static lUInt16 path2[] = { el_title, el_p, el_strong, 0 };

    CRLog::info("* simple DOM operations, tinyElement");
    EXPECT_TRUE(root->isRoot());                // root isRoot
    EXPECT_TRUE(root->getParentNode() == NULL); // root parent is null
    EXPECT_EQ(root->getParentIndex(), 0);       // root parent index == 0
    EXPECT_EQ(root->getChildCount(), 0);        // empty root child count
    ldomNode* el1 = root->insertChildElement(el_p);
    EXPECT_EQ(root->getChildCount(), 1);                    // root child count 1
    EXPECT_EQ(el1->getParentNode(), root);                  // element parent node
    EXPECT_EQ(el1->getParentIndex(), root->getDataIndex()); // element parent node index
    EXPECT_EQ(el1->getNodeId(), el_p);                      // node id
    EXPECT_EQ(el1->getNodeNsId(), LXML_NS_NONE);            // node nsid
    EXPECT_FALSE(el1->isRoot());                            // elem not isRoot
    ldomNode* el2 = root->insertChildElement(el_title);
    EXPECT_EQ(root->getChildCount(), 2);         // root child count 2
    EXPECT_EQ(el2->getNodeId(), el_title);       // node id
    EXPECT_EQ(el2->getNodeNsId(), LXML_NS_NONE); // node nsid
    lString32 nodename = el2->getNodeName();
    //CRLog::debug("node name: %s", LCSTR(nodename));
    EXPECT_TRUE(nodename == U"title"); // node name
    ldomNode* el21 = el2->insertChildElement(el_p);
    EXPECT_EQ(root->getNodeLevel(), 1);         // node level 1
    EXPECT_EQ(el2->getNodeLevel(), 2);          // node level 2
    EXPECT_EQ(el21->getNodeLevel(), 3);         // node level 3
    EXPECT_EQ(el21->getNodeIndex(), 0);         // node index single
    EXPECT_EQ(el1->getNodeIndex(), 0);          // node index first
    EXPECT_EQ(el2->getNodeIndex(), 1);          // node index last
    EXPECT_EQ(root->getNodeIndex(), 0);         // node index for root
    EXPECT_TRUE(root->getFirstChild() == el1);  // first child
    EXPECT_TRUE(root->getLastChild() == el2);   // last child
    EXPECT_TRUE(el2->getFirstChild() == el21);  // first single child
    EXPECT_TRUE(el2->getLastChild() == el21);   // last single child
    EXPECT_TRUE(el21->getFirstChild() == NULL); // first child - no children
    EXPECT_TRUE(el21->getLastChild() == NULL);  // last child - no children
    ldomNode* el0 = root->insertChildElement(1, LXML_NS_NONE, el_title);
    EXPECT_EQ(el1->getNodeIndex(), 0);
    EXPECT_EQ(el0->getNodeIndex(), 1);
    EXPECT_EQ(el2->getNodeIndex(), 2);
    EXPECT_TRUE(root->getChildNode(0) == el1); // child node 0
    EXPECT_TRUE(root->getChildNode(1) == el0); // child node 1
    EXPECT_TRUE(root->getChildNode(2) == el2); // child node 2
    ldomNode* removedNode = root->removeChild(1);
    EXPECT_EQ(removedNode, el0); // removed node
    el0->destroy();
    // TODO: investigate if we should remove el0 itself?
    EXPECT_TRUE(el0->isNull());                // destroyed node isNull
    EXPECT_TRUE(root->getChildNode(0) == el1); // child node 0, after removal
    EXPECT_TRUE(root->getChildNode(1) == el2); // child node 1, after removal
    ldomNode* el02 = root->insertChildElement(5, LXML_NS_NONE, el_emphasis);
    EXPECT_EQ(el02, el0); // removed node reusage
    {
        ldomNode* f1 = root->findChildElement(path1);
        EXPECT_TRUE(f1 == el21);          // find 1 on mutable - is el21
        EXPECT_EQ(f1->getNodeId(), el_p); // find 1 on mutable
        //ldomNode* f2 = root->findChildElement(path2);
        //EXPECT_TRUE(f2 != NULL);               // find 2 on mutable - not null
        //EXPECT_TRUE(f2 == el21);               // find 2 on mutable - is el21
        //EXPECT_EQ(f2->getNodeId(), el_strong); // find 2 on mutable
    }

    CRLog::info("* simple DOM operations, mutable text");
    lString32 sampleText("Some sample text.");
    lString32 sampleText2("Some sample text 2.");
    lString32 sampleText3("Some sample text 3.");
    ldomNode* text1 = el1->insertChildText(sampleText);
    EXPECT_TRUE(text1->getText() == sampleText); // sample text 1 match unicode
    EXPECT_EQ(text1->getNodeLevel(), 3);         // text node level
    EXPECT_EQ(text1->getNodeIndex(), 0);         // text node index
    EXPECT_TRUE(text1->isText());                // text node isText
    EXPECT_FALSE(text1->isElement());            // text node isElement
    EXPECT_FALSE(text1->isNull());               // text node isNull
    ldomNode* text2 = el1->insertChildText(0, sampleText2);
    EXPECT_EQ(text2->getNodeIndex(), 0);                          // text node index, insert at beginning
    EXPECT_TRUE(text2->getText() == sampleText2);                 // sample text 2 match unicode
    EXPECT_TRUE(text2->getText8() == UnicodeToUtf8(sampleText2)); // sample text 2 match utf8
    text1->setText(sampleText2);
    EXPECT_TRUE(text1->getText() == sampleText2); // sample text 1 match unicode, changed
    text1->setText8(UnicodeToUtf8(sampleText3));
    EXPECT_TRUE(text1->getText() == sampleText3);                 // sample text 1 match unicode, changed 8
    EXPECT_TRUE(text1->getText8() == UnicodeToUtf8(sampleText3)); // sample text 1 match utf8, changed

    EXPECT_TRUE(el1->getFirstTextChild() == text2); // firstTextNode
    EXPECT_TRUE(el1->getLastTextChild() == text1);  // lastTextNode
    EXPECT_TRUE(el21->getLastTextChild() == NULL);  // lastTextNode NULL

#if BUILD_LITE != 1
    CRLog::info("* style cache");
    {
        css_style_ref_t style1;
        style1 = css_style_ref_t(new css_style_rec_t);
        style1->display = css_d_block;
        style1->white_space = css_ws_normal;
        style1->text_align = css_ta_left;
        style1->text_align_last = css_ta_left;
        style1->text_decoration = css_td_none;
        style1->text_transform = css_tt_none;
        style1->hyphenate = css_hyph_auto;
        style1->color.type = css_val_unspecified;
        style1->color.value = 0x000000;
        style1->background_color.type = css_val_unspecified;
        style1->background_color.value = 0xFFFFFF;
        style1->page_break_before = css_pb_auto;
        style1->page_break_after = css_pb_auto;
        style1->page_break_inside = css_pb_auto;
        style1->vertical_align.type = css_val_unspecified;
        style1->vertical_align.value = css_va_baseline;
        style1->font_family = css_ff_sans_serif;
        style1->font_size.type = css_val_px;
        style1->font_size.value = 24 << 8;
        style1->font_name = cs8("FreeSerif");
        style1->font_weight = css_fw_400;
        style1->font_style = css_fs_normal;
        style1->font_features.type = css_val_unspecified;
        style1->font_features.value = 0;
        style1->text_indent.type = css_val_px;
        style1->text_indent.value = 0;
        style1->line_height.type = css_val_unspecified;
        style1->line_height.value = css_generic_normal; // line-height: normal
        style1->cr_hint.type = css_val_unspecified;
        style1->cr_hint.value = CSS_CR_HINT_NONE;

        css_style_ref_t style2;
        style2 = css_style_ref_t(new css_style_rec_t);
        style2->display = css_d_block;
        style2->white_space = css_ws_normal;
        style2->text_align = css_ta_left;
        style2->text_align_last = css_ta_left;
        style2->text_decoration = css_td_none;
        style2->text_transform = css_tt_none;
        style2->hyphenate = css_hyph_auto;
        style2->color.type = css_val_unspecified;
        style2->color.value = 0x000000;
        style2->background_color.type = css_val_unspecified;
        style2->background_color.value = 0xFFFFFF;
        style2->page_break_before = css_pb_auto;
        style2->page_break_after = css_pb_auto;
        style2->page_break_inside = css_pb_auto;
        style2->vertical_align.type = css_val_unspecified;
        style2->vertical_align.value = css_va_baseline;
        style2->font_family = css_ff_sans_serif;
        style2->font_size.type = css_val_px;
        style2->font_size.value = 24 << 8;
        style2->font_name = cs8("FreeSerif");
        style2->font_weight = css_fw_400;
        style2->font_style = css_fs_normal;
        style2->font_features.type = css_val_unspecified;
        style2->font_features.value = 0;
        style2->text_indent.type = css_val_px;
        style2->text_indent.value = 0;
        style2->line_height.type = css_val_unspecified;
        style2->line_height.value = css_generic_normal; // line-height: normal
        style2->cr_hint.type = css_val_unspecified;
        style2->cr_hint.value = CSS_CR_HINT_NONE;

        css_style_ref_t style3;
        style3 = css_style_ref_t(new css_style_rec_t);
        style3->display = css_d_block;
        style3->white_space = css_ws_normal;
        style3->text_align = css_ta_right;
        style3->text_align_last = css_ta_left;
        style3->text_decoration = css_td_none;
        style3->text_transform = css_tt_none;
        style3->hyphenate = css_hyph_auto;
        style3->color.type = css_val_unspecified;
        style3->color.value = 0x000000;
        style3->background_color.type = css_val_unspecified;
        style3->background_color.value = 0xFFFFFF;
        style3->page_break_before = css_pb_auto;
        style3->page_break_after = css_pb_auto;
        style3->page_break_inside = css_pb_auto;
        style3->vertical_align.type = css_val_unspecified;
        style3->vertical_align.value = css_va_baseline;
        style3->font_family = css_ff_sans_serif;
        style3->font_size.type = css_val_px;
        style3->font_size.value = 24 << 8;
        style3->font_name = cs8("FreeSerif");
        style3->font_weight = css_fw_400;
        style3->font_style = css_fs_normal;
        style3->font_features.type = css_val_unspecified;
        style3->font_features.value = 0;
        style3->text_indent.type = css_val_px;
        style3->text_indent.value = 0;
        style3->line_height.type = css_val_unspecified;
        style3->line_height.value = css_generic_normal; // line-height: normal
        style3->cr_hint.type = css_val_unspecified;
        style3->cr_hint.value = CSS_CR_HINT_NONE;

        el1->setStyle(style1);
        css_style_ref_t s1 = el1->getStyle();
        EXPECT_FALSE(s1.isNull()); // style is set
        el2->setStyle(style2);
        EXPECT_TRUE(*style1 == *style2);                             // identical styles : == is true
        EXPECT_EQ(calcHash(*style1), calcHash(*style2));             // identical styles have the same hashes
        EXPECT_TRUE(el1->getStyle().get() == el2->getStyle().get()); // identical styles reused
        el21->setStyle(style3);
        EXPECT_TRUE(el1->getStyle().get() != el21->getStyle().get()); // different styles not reused
    }

    CRLog::info("* font cache");
    {
        font_ref_t font1 = fontMan->GetFont(24, 400, false, css_ff_sans_serif, cs8("FreeSans"));
        font_ref_t font2 = fontMan->GetFont(24, 400, false, css_ff_sans_serif, cs8("FreeSans"));
        font_ref_t font3 = fontMan->GetFont(28, 800, false, css_ff_serif, cs8("FreeSerif"));
        EXPECT_TRUE(el1->getFont().isNull()); // font is not set
        el1->setFont(font1);
        EXPECT_TRUE(!el1->getFont().isNull()); // font is set
        el2->setFont(font2);
        EXPECT_TRUE(*font1 == *font2);                             // identical fonts : == is true
        EXPECT_EQ(calcHash(font1), calcHash(font2));               // identical styles have the same hashes
        EXPECT_TRUE(el1->getFont().get() == el2->getFont().get()); // identical fonts reused
        el21->setFont(font3);
        EXPECT_TRUE(el1->getFont().get() != el21->getFont().get()); // different fonts not reused
    }

    CRLog::info("* persistence test");
    el2->setAttributeValue(LXML_NS_NONE, attr_id, U"id1");
    el2->setAttributeValue(LXML_NS_NONE, attr_name, U"name1");
    EXPECT_EQ(el2->getNodeId(), el_title);                      // mutable node id
    EXPECT_EQ(el2->getNodeNsId(), LXML_NS_NONE);                // mutable node nsid
    EXPECT_TRUE(el2->getAttributeValue(attr_id) == U"id1");     // attr id1 mutable
    EXPECT_TRUE(el2->getAttributeValue(attr_name) == U"name1"); // attr name1 mutable
    EXPECT_EQ(el2->getAttrCount(), 2);                          // attr count mutable
    el2->persist();
    EXPECT_TRUE(el2->getAttributeValue(attr_id) == U"id1");     // attr id1 pers
    EXPECT_TRUE(el2->getAttributeValue(attr_name) == U"name1"); // attr name1 pers
    EXPECT_EQ(el2->getNodeId(), el_title);                      // persistent node id
    EXPECT_EQ(el2->getNodeNsId(), LXML_NS_NONE);                // persistent node nsid
    EXPECT_EQ(el2->getAttrCount(), 2);                          // attr count persist

    {
        ldomNode* f1 = root->findChildElement(path1);
        EXPECT_TRUE(f1 == el21);          // find 1 on mutable - is el21
        EXPECT_EQ(f1->getNodeId(), el_p); // find 1 on mutable
    }

    el2->modify();
    EXPECT_EQ(el2->getNodeId(), el_title);                      // mutable 2 node id
    EXPECT_EQ(el2->getNodeNsId(), LXML_NS_NONE);                // mutable 2 node nsid
    EXPECT_TRUE(el2->getAttributeValue(attr_id) == U"id1");     // attr id1 mutable 2
    EXPECT_TRUE(el2->getAttributeValue(attr_name) == U"name1"); // attr name1 mutable 2
    EXPECT_EQ(el2->getAttrCount(), 2);                          // attr count mutable 2

    {
        ldomNode* f1 = root->findChildElement(path1);
        EXPECT_TRUE(f1 == el21);          // find 1 on mutable - is el21
        EXPECT_EQ(f1->getNodeId(), el_p); // find 1 on mutable
    }

    CRLog::info("* convert to persistent");
    CRTimerUtil infinite;
    doc->persist(infinite);
    doc->dumpStatistics();

    EXPECT_TRUE(el21->getFirstChild() == NULL); // first child - no children
    EXPECT_TRUE(el21->isPersistent());          // persistent before insertChildElement
    ldomNode* el211 = el21->insertChildElement(el_strong);
    EXPECT_FALSE(el21->isPersistent()); // mutable after insertChildElement
    el211->persist();
    EXPECT_TRUE(el211->isPersistent()); // persistent before insertChildText
    el211->insertChildText(cs32(U"bla bla bla"));
    el211->insertChildText(cs32(U"bla bla blaw"));
    EXPECT_FALSE(el211->isPersistent()); // modifable after insertChildText
    //el21->insertChildElement(el_strong);
    EXPECT_EQ(el211->getChildCount(), 2); // child count, in mutable
    el211->persist();
    EXPECT_EQ(el211->getChildCount(), 2); // child count, in persistent
    el211->modify();
    EXPECT_EQ(el211->getChildCount(), 2); // child count, in mutable again
    CRTimerUtil infinite2;
    doc->persist(infinite2);

    ldomNode* f1 = root->findChildElement(path1);
    EXPECT_EQ(f1->getNodeId(), el_p); // find 1
    ldomNode* f2 = root->findChildElement(path2);
    EXPECT_EQ(f2->getNodeId(), el_strong); // find 2
    EXPECT_TRUE(f2 == el211);              // find 2, ref

    CRLog::info("* compacting");
    doc->compact();
    doc->dumpStatistics();
#endif

    delete doc;

    CRLog::info("Finished basicTinyDomUnitTests");
    CRLog::info("==============================");
}

#define TEST_FILE_NAME TESTS_TMPDIR "test-cache-file.dat"

TEST_F(TinyDOMTests, testCacheFileClass) {
#if BUILD_LITE != 1
    CRLog::info("===========================");
    CRLog::info("Starting testCacheFileClass");
    lUInt8 data1[] = { 'T', 'e', 's', 't', 'd', 'a', 't', 'a', 1, 2, 3, 4, 5, 6, 7 };
    lUInt8 data2[] = { 'T', 'e', 's', 't', 'd', 'a', 't', 'a', '2', 1, 2, 3, 4, 5, 6, 7 };
    lUInt8* buf1;
    lUInt8* buf2;
    int sz1;
    int sz2;
    lString32 fn(TEST_FILE_NAME);
    {
        lUInt8 data1[] = { 'T', 'e', 's', 't', 'D', 'a', 't', 'a', '1' };
        lUInt8 data2[] = { 'T', 'e', 's', 't', 'D', 'a', 't', 'a', '2', 1, 2, 3, 4, 5, 6, 7 };
        LVStreamRef s = LVOpenFileStream(fn.c_str(), LVOM_APPEND);
        ASSERT_TRUE(!s.isNull());
        s->SetPos(0);
        s->Write(data1, sizeof(data1), NULL);
        s->SetPos(4096);
        s->Write(data1, sizeof(data1), NULL);
        s->SetPos(8192);
        s->Write(data2, sizeof(data2), NULL);
        s->SetPos(4096);
        s->Write(data2, sizeof(data2), NULL);
        lUInt8 buf[16];
        s->SetPos(0);
        s->Read(buf, sizeof(data1), NULL);
        EXPECT_EQ(memcmp(buf, data1, sizeof(data1)), 0); // read 1 content
        s->SetPos(4096);
        s->Read(buf, sizeof(data2), NULL);
        EXPECT_EQ(memcmp(buf, data2, sizeof(data2)), 0); // read 2 content
    }
    // write
    {
        CacheFile f(gDOMVersionCurrent, CacheCompressionNone);
        EXPECT_FALSE(f.open(cs32(TESTS_TMPDIR "blabla-not-exits-file-name"))); // Wrong failed open result
        EXPECT_TRUE(f.create(fn));                                             // new file created
        EXPECT_TRUE(f.write(CBT_TEXT_DATA, 1, data1, sizeof(data1), true));    // write 1
        EXPECT_TRUE(f.write(CBT_ELEM_DATA, 3, data2, sizeof(data2), false));   // write 2

        EXPECT_TRUE(f.read(CBT_TEXT_DATA, 1, buf1, sz1)); // read 1
        ASSERT_TRUE(buf1 != NULL);
        EXPECT_TRUE(f.read(CBT_ELEM_DATA, 3, buf2, sz2)); // read 2
        ASSERT_TRUE(buf2 != NULL);
        EXPECT_EQ(sz1, sizeof(data1));                    // read 1 size
        EXPECT_EQ(memcmp(buf1, data1, sizeof(data1)), 0); // read 1 content
        EXPECT_EQ(sz2, sizeof(data2));                    // read 2 size
        EXPECT_EQ(memcmp(buf2, data2, sizeof(data2)), 0); // read 2 content
        CRTimerUtil inf;
        EXPECT_TRUE(f.flush(true, inf)); // flush
        free(buf2);
        free(buf1);
    }
    // read
    {
        CacheFile f(gDOMVersionCurrent, CacheCompressionNone);
        EXPECT_TRUE(f.open(fn));                          // Wrong failed open result
        EXPECT_TRUE(f.read(CBT_TEXT_DATA, 1, buf1, sz1)); // read 1
        ASSERT_TRUE(buf1 != NULL);
        EXPECT_TRUE(f.read(CBT_ELEM_DATA, 3, buf2, sz2)); // read 2
        ASSERT_TRUE(buf2 != NULL);
        EXPECT_EQ(sz1, sizeof(data1));                    // read 1 size
        EXPECT_EQ(memcmp(buf1, data1, sizeof(data1)), 0); // read 1 content
        EXPECT_EQ(sz2, sizeof(data2));                    // read 2 size
        EXPECT_EQ(memcmp(buf2, data2, sizeof(data2)), 0); // read 2 content
        free(buf2);
        free(buf1);
    }

    LVDeleteFile(fn);
    LVDeleteFile(cs32(TESTS_TMPDIR "blabla-not-exits-file-name"));

    CRLog::info("Finished testCacheFileClass");
    CRLog::info("===========================");
#endif
}

#define TEST_FN_TO_OPEN TESTS_DATADIR "example.fb2.zip"

TEST_F(TinyDOMTests, testDocumentCaching) {
#if BUILD_LITE != 1
    CRLog::info("============================");
    CRLog::info("Starting testDocumentCaching");
    ASSERT_TRUE(m_initOK);
    // init and clear cache
    ldomDocCache::init(cs32(TESTS_TMPDIR "cr3cache"), 100);
    EXPECT_TRUE(ldomDocCache::enabled()); // cache enabled
    {
        // open document and save to cache
        LVDocView view(4, false);
        view.Resize(600, 800);
        bool res = view.LoadDocument(TESTS_DATADIR "not-existing.fb2");
        EXPECT_FALSE(res); // try to load not existing document
        res = view.LoadDocument(TEST_FN_TO_OPEN);
        EXPECT_TRUE(res); // load document
        LVDocImageRef image = view.getPageImage(0);
        EXPECT_FALSE(image.isNull());
        view.getDocProps()->setInt(PROP_FORCED_MIN_FILE_SIZE_TO_CACHE, 30000);
        res = view.swapToCache();
        ASSERT_TRUE(res);
        view.getDocument()->dumpStatistics();
    }
    {
        // open document from cache
        // TODO: check is document is realy loaded from cache...
        LVDocView view(4, false);
        view.Resize(600, 800);
        bool res = view.LoadDocument(TEST_FN_TO_OPEN);
        EXPECT_TRUE(res); // load document
        view.getDocument()->dumpStatistics();
        LVDocImageRef image = view.getPageImage(0);
        EXPECT_FALSE(image.isNull());
    }
    EXPECT_TRUE(ldomDocCache::clear()); // cache cleared
    CRLog::info("Finished testDocumentCaching");
    CRLog::info("============================");
#endif
}
