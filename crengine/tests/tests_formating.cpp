/***************************************************************************
 *   crengine-ng, unit testing                                             *
 *   Copyright (C) 2008 Vadim Lopatin <coolreader.org@gmail.com>           *
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
 * \file tests_formating.cpp
 * \brief Testing the text formatter
 */

#include <lvtextfm.h>
#include <lvfntman.h>

#include "gtest/gtest.h"

void s_addLine(LFormattedText& txt, const lChar32* str, int flags, LVFontRef font) {
    lString32 s(str);
    txt.AddSourceLine(
            s.c_str(),  // pointer to unicode text string
            s.length(), // number of chars in text, 0 for auto(strlen)
            0x000000,   // text color
            0xFFFFFF,   // background color
            font.get(), // font to draw string
            NULL,       // lang_cfg
            flags,      // flags
            16,         // interline space, *16 (16=single, 32=double)
            0,          // drift y from baseline
            30,         // first line margin
            NULL,       // object
            0,          // offset
            0           // letter_spacing
    );
}

#if 0
void s_dump(LFormattedText& ftxt) {
    formatted_text_fragment_t* buf = ftxt.GetBuffer();
    for (lInt32 i = 0; i < buf->frmlinecount; i++) {
        formatted_line_t* frmline = buf->frmlines[i];
        printf("line[%d]\t ", i);
        for (lInt32 j = 0; j < frmline->word_count; j++) {
            formatted_word_t* word = &frmline->words[j];
            if (word->flags & LTEXT_WORD_IS_OBJECT) {
                // object
                printf("{%d..%d} object\t", (int)word->x, (int)(word->x + word->width));
            } else {
                // dump text
                src_text_fragment_t* src = &buf->srctext[word->src_text_index];
                lString32 txt = lString32(src->u.t.text + word->u.t.start, word->u.t.len);
                lString8 txt8 = UnicodeToUtf8(txt);
                printf("{%d..%d} \"%s\"\t", (int)word->x, (int)(word->x + word->width), (const char*)txt8.c_str());
            }
        }
        printf("\n");
    }
}
#endif

// units tests

TEST(FormattingTests, testFormatting) {
    LFormattedText ftxt;
    LVFontRef font0 = fontMan->GetFont(20, 400, false, css_ff_monospace, cs8("FreeMono"));
    LVFontRef font1 = fontMan->GetFont(20, 400, false, css_ff_sans_serif, cs8("FreeSans"));
    s_addLine(ftxt, U"   Testing preformatted\ntext processing.", LTEXT_ALIGN_LEFT | LTEXT_FLAG_PREFORMATTED | LTEXT_FLAG_OWNTEXT, font0);
    s_addLine(ftxt, U"   Testing preformatted\ntext processing.", LTEXT_ALIGN_WIDTH | LTEXT_FLAG_OWNTEXT, font1);
    s_addLine(ftxt, U"\nNewline.", LTEXT_FLAG_OWNTEXT, font1);
    s_addLine(ftxt, U"\n\n2 Newlines.", LTEXT_FLAG_OWNTEXT, font1);
#if 0
    LVFontRef font2 = fontMan->GetFont(20, 400, false, css_ff_serif, cs8("FreeSerif"));
    s_addLine(txt, U"Testing thisislonglongwordto", LTEXT_ALIGN_WIDTH | LTEXT_FLAG_OWNTEXT, font1);
    s_addLine(txt, U"Testing", LTEXT_ALIGN_WIDTH | LTEXT_FLAG_OWNTEXT, font1);
    s_addLine(txt, U"several", LTEXT_ALIGN_LEFT | LTEXT_FLAG_OWNTEXT, font2);
    s_addLine(txt, U" short", LTEXT_FLAG_OWNTEXT, font1);
    s_addLine(txt, U"words!", LTEXT_FLAG_OWNTEXT, font2);
    s_addLine(txt, U"Testing thisislonglongwordtohyphenate simple paragraph formatting. Just a test. ", LTEXT_ALIGN_WIDTH | LTEXT_FLAG_OWNTEXT, font1);
    s_addLine(txt, U"There is seldom reason to tag a file in isolation. A more common use is to tag all the files that constitute a module with the same tag at strategic points in the development life-cycle, such as when a release is made.", LTEXT_ALIGN_WIDTH | LTEXT_FLAG_OWNTEXT, font1);
    s_addLine(txt, U"There is seldom reason to tag a file in isolation. A more common use is to tag all the files that constitute a module with the same tag at strategic points in the development life-cycle, such as when a release is made.", LTEXT_ALIGN_WIDTH | LTEXT_FLAG_OWNTEXT, font1);
    s_addLine(txt, U"There is seldom reason to tag a file in isolation. A more common use is to tag all the files that constitute a module with the same tag at strategic points in the development life-cycle, such as when a release is made.", LTEXT_ALIGN_WIDTH | LTEXT_FLAG_OWNTEXT, font1);
    s_addLine(txt, U"Next paragraph: left-aligned. Blabla bla blabla blablabla hdjska hsdjkasld hsdjka sdjaksdl hasjkdl ahklsd hajklsdh jaksd hajks dhjksdhjshd sjkdajsh hasjdkh ajskd hjkhjksajshd hsjkadh sjk.", LTEXT_ALIGN_LEFT | LTEXT_FLAG_OWNTEXT, font1);
    s_addLine(txt, U"Testing thisislonglongwordtohyphenate simple paragraph formatting. Just a test. ", LTEXT_ALIGN_WIDTH | LTEXT_FLAG_OWNTEXT, font1);
    s_addLine(txt, U"Another fragment of text. ", LTEXT_FLAG_OWNTEXT, font1);
    s_addLine(txt, U"And the last one written with another font", LTEXT_FLAG_OWNTEXT, font2);
    s_addLine(txt, U"Next paragraph: left-aligned. ", LTEXT_ALIGN_LEFT | LTEXT_FLAG_OWNTEXT, font1);
    s_addLine(txt, U"One more sentence. Second sentence.", LTEXT_FLAG_OWNTEXT, font1);
#endif
    ftxt.Format(300, 400);
    //s_dump(ftxt);

    formatted_text_fragment_t* buf = ftxt.GetBuffer();
    ASSERT_EQ(buf->frmlinecount, 7);
    formatted_line_t* frmline0 = buf->frmlines[0];
    ASSERT_EQ(frmline0->word_count, 3);

    formatted_word_t* word00 = &frmline0->words[0];
    ASSERT_FALSE(word00->flags & LTEXT_WORD_IS_OBJECT);
    src_text_fragment_t* src = &buf->srctext[word00->src_text_index];
    lString8 txt = UnicodeToUtf8(lString32(src->u.t.text + word00->u.t.start, word00->u.t.len));
    EXPECT_STREQ(txt.c_str(), "   ");

    formatted_word_t* word01 = &frmline0->words[1];
    ASSERT_FALSE(word01->flags & LTEXT_WORD_IS_OBJECT);
    src = &buf->srctext[word01->src_text_index];
    txt = UnicodeToUtf8(lString32(src->u.t.text + word01->u.t.start, word01->u.t.len));
    EXPECT_STREQ(txt.c_str(), "Testing ");

    formatted_word_t* word02 = &frmline0->words[2];
    ASSERT_FALSE(word02->flags & LTEXT_WORD_IS_OBJECT);
    src = &buf->srctext[word02->src_text_index];
    txt = UnicodeToUtf8(lString32(src->u.t.text + word02->u.t.start, word02->u.t.len));
    EXPECT_STREQ(txt.c_str(), "preformatted");

    EXPECT_GT(word01->x, word00->x);
    EXPECT_GT(word02->x, word01->x);

    formatted_line_t* frmline2 = buf->frmlines[2];
    ASSERT_EQ(frmline2->word_count, 2);

    formatted_word_t* word20 = &frmline2->words[0];
    ASSERT_FALSE(word20->flags & LTEXT_WORD_IS_OBJECT);
    src = &buf->srctext[word20->src_text_index];
    txt = UnicodeToUtf8(lString32(src->u.t.text + word20->u.t.start, word20->u.t.len));
    EXPECT_STREQ(txt.c_str(), "Testing ");
}
