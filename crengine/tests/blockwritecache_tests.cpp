/**
 *  crengine-ng
 *
 *  \file blockwritecache_tests.cpp
 *  \brief crengine-ng unit test module.
 *  Tests LVBlockWriteStream class.
 *  This test extracted & adapted from lvtinydom.cpp
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

#include <lvstreamutils.h>

#include <stdlib.h>

#include "gtest/gtest.h"

#ifndef TESTS_TMPDIR
#define TESTS_TMPDIR "/tmp/"
#endif

// auxilari methods

static bool makeTestFile(const char* fname, int size) {
    bool res = false;
    LVStreamRef s = LVOpenFileStream(fname, LVOM_WRITE);
    res = !s.isNull();
    if (res) {
        unsigned int seed = 0;
        lUInt8* buf = new lUInt8[size];
        for (int i = 0; i < size; i++) {
            buf[i] = (seed >> 9) & 255;
            seed = seed * 31 + 14323;
        }
        lverror_t err = s->Write(buf, size, NULL);
        res = err == LVERR_OK;
        delete[] buf;
    }
    return res;
}

// Fixtures

class BlockWriteCacheTests: public testing::Test
{
protected:
    bool m_initOK;
    const char* fn1;
    const char* fn2;
protected:
    void SetUp() {
        int sz = 2000000;
        fn1 = TESTS_TMPDIR "tf1.dat";
        fn2 = TESTS_TMPDIR "tf2.dat";
        m_initOK = makeTestFile(fn1, sz);
        if (m_initOK)
            m_initOK = makeTestFile(fn2, sz);
    }

    void TearDown() {
        LVDeleteFile(lString8(fn1));
        LVDeleteFile(lString8(fn2));
    }
};

// units tests

TEST_F(BlockWriteCacheTests, blockWriteCacheTest) {
    ASSERT_TRUE(m_initOK);

    LVStreamRef s1 = LVOpenFileStream(fn1, LVOM_APPEND);
    LVStreamRef s2 = LVCreateBlockWriteStream(LVOpenFileStream(fn2, LVOM_APPEND), 0x8000, 16);
    EXPECT_TRUE(!s1.isNull());
    EXPECT_TRUE(!s2.isNull());
    lUInt8 buf[0x100000];
    for (unsigned int i = 0; i < sizeof(buf); i++) {
        buf[i] = (lUInt8)(rand() & 0xFF);
    }
    lUInt8 buf2[0x100000];

    lverror_t res1;
    lverror_t res2;
    lvpos_t pos1;
    lvpos_t pos2;
    bool eof1;
    bool eof2;
    lvsize_t bw1 = 0;
    lvsize_t bw2 = 0;
    lvsize_t count;

    // 1. ss->SetPos(0)
    pos1 = s1->SetPos(0);
    pos2 = s2->SetPos(0);
    EXPECT_EQ(pos1, pos2);
    // 2. ss->Write(buf, 150, NULL)
    count = 150;
    res1 = s1->Write(buf, count, &bw1);
    res2 = s2->Write(buf, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, count);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    EXPECT_EQ(s1->Eof(), s2->Eof());
    // 3. ss->SetPos(0), repeat
    pos1 = s1->SetPos(0);
    pos2 = s2->SetPos(0);
    EXPECT_EQ(pos1, pos2);
    // 4. ss->Write(buf, 150, NULL), repeat
    count = 150;
    res1 = s1->Write(buf, count, &bw1);
    res2 = s2->Write(buf, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, count);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    EXPECT_EQ(s1->Eof(), s2->Eof());
    // 5. ss->SetPos(0), repeat
    pos1 = s1->SetPos(0);
    pos2 = s2->SetPos(0);
    EXPECT_EQ(pos1, pos2);
    // 6. ss->Write(buf, 150, NULL), repeat
    count = 150;
    res1 = s1->Write(buf, count, &bw1);
    res2 = s2->Write(buf, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, count);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);

    // 7. ss->SetPos(1000)
    pos1 = s1->SetPos(1000);
    pos2 = s2->SetPos(1000);
    EXPECT_EQ(pos1, pos2);
    // 8. ss->Read(buf, 5000, NULL)
    count = 5000;
    res1 = s1->Read(buf, count, &bw1);
    res2 = s2->Read(buf2, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);
    for (unsigned i = 0; i < count; i++)
        EXPECT_EQ(buf[i], buf2[i]);
    // 9. ss->SetPos(100000)
    pos1 = s1->SetPos(100000);
    pos2 = s2->SetPos(100000);
    EXPECT_EQ(pos1, pos2);
    // 10. ss->Read(buf + 10000, 150000, NULL)
    count = 150000;
    res1 = s1->Read(buf + 10000, count, &bw1);
    res2 = s2->Read(buf2, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);
    for (unsigned i = 0; i < count; i++)
        EXPECT_EQ(buf[10000 + i], buf2[i]);

    // 11. ss->SetPos(1000);
    pos1 = s1->SetPos(1000);
    pos2 = s2->SetPos(1000);
    EXPECT_EQ(pos1, pos2);
    // 12. ss->Write(buf, 15000, NULL);
    count = 15000;
    res1 = s1->Write(buf, count, &bw1);
    res2 = s2->Write(buf, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, count);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);
    // 13. ss->SetPos(1000);
    pos1 = s1->SetPos(1000);
    pos2 = s2->SetPos(1000);
    EXPECT_EQ(pos1, pos2);
    // 14. ss->Read(buf + 100000, 15000, NULL);
    count = 15000;
    res1 = s1->Read(buf + 100000, count, &bw1);
    res2 = s2->Read(buf2, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);
    for (unsigned i = 0; i < count; i++)
        EXPECT_EQ(buf[100000 + i], buf2[i]);
    // 15. ss->Read(buf, 1000000, NULL);
    count = 1000000;
    res1 = s1->Read(buf, count, &bw1);
    res2 = s2->Read(buf2, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);
    for (unsigned i = 0; i < count; i++)
        EXPECT_EQ(buf[i], buf2[i]);

    // 16. ss->SetPos(1000);
    pos1 = s1->SetPos(1000);
    pos2 = s2->SetPos(1000);
    EXPECT_EQ(pos1, pos2);
    // 17. ss->Write(buf, 15000, NULL);
    count = 15000;
    res1 = s1->Write(buf, count, &bw1);
    res2 = s2->Write(buf, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, count);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);
    // 18. ss->Write(buf, 15000, NULL);
    count = 15000;
    res1 = s1->Write(buf, count, &bw1);
    res2 = s2->Write(buf, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, count);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);
    // 19. ss->Write(buf, 15000, NULL);
    count = 15000;
    res1 = s1->Write(buf, count, &bw1);
    res2 = s2->Write(buf, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, count);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);
    // 20. ss->Write(buf, 15000, NULL);
    count = 15000;
    res1 = s1->Write(buf, count, &bw1);
    res2 = s2->Write(buf, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, count);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);

    // 21. ss->SetPos(100000);
    pos1 = s1->SetPos(100000);
    pos2 = s2->SetPos(100000);
    EXPECT_EQ(pos1, pos2);
    // 22. ss->Write(buf + 15000, 150000, NULL);
    count = 150000;
    res1 = s1->Write(buf + 15000, count, &bw1);
    res2 = s2->Write(buf + 15000, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, count);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);
    // 23. ss->SetPos(100000);
    pos1 = s1->SetPos(100000);
    pos2 = s2->SetPos(100000);
    EXPECT_EQ(pos1, pos2);
    // 24. ss->Read(buf + 25000, 200000, NULL);
    count = 200000;
    res1 = s1->Read(buf + 25000, count, &bw1);
    res2 = s2->Read(buf2, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);
    for (unsigned i = 0; i < count; i++)
        EXPECT_EQ(buf[25000 + i], buf2[i]);

    // 25. ss->SetPos(100000);
    pos1 = s1->SetPos(100000);
    pos2 = s2->SetPos(100000);
    EXPECT_EQ(pos1, pos2);
    // 26. ss->Read(buf + 55000, 200000, NULL);
    count = 200000;
    res1 = s1->Read(buf + 55000, count, &bw1);
    res2 = s2->Read(buf2, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);
    for (unsigned i = 0; i < count; i++)
        EXPECT_EQ(buf[55000 + i], buf2[i]);

    // 27. ss->SetPos(100000);
    pos1 = s1->SetPos(100000);
    pos2 = s2->SetPos(100000);
    EXPECT_EQ(pos1, pos2);
    // 28. ss->Write(buf + 1000, 250000, NULL);
    count = 250000;
    res1 = s1->Write(buf + 1000, count, &bw1);
    res2 = s2->Write(buf + 1000, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, count);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);
    // 29. ss->SetPos(150000);
    pos1 = s1->SetPos(150000);
    pos2 = s2->SetPos(150000);
    EXPECT_EQ(pos1, pos2);
    // 30. ss->Read(buf, 50000, NULL);
    count = 50000;
    res1 = s1->Read(buf, count, &bw1);
    res2 = s2->Read(buf2, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);
    for (unsigned i = 0; i < count; i++)
        EXPECT_EQ(buf[i], buf2[i]);
    // 31. ss->SetPos(1000000);
    pos1 = s1->SetPos(1000000);
    pos2 = s2->SetPos(1000000);
    // 32. ss->Write(buf, 500000, NULL);
    count = 500000;
    res1 = s1->Write(buf, count, &bw1);
    res2 = s2->Write(buf, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, count);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);
    for (int i = 0; i < 10; i++) {
        // 33. ss->Write(buf, 5000, NULL);
        count = 5000;
        res1 = s1->Write(buf, count, &bw1);
        res2 = s2->Write(buf, count, &bw2);
        EXPECT_EQ(res1, res2);
        EXPECT_EQ(bw1, count);
        EXPECT_EQ(bw1, bw2);
        pos1 = s1->GetPos();
        pos2 = s2->GetPos();
        EXPECT_EQ(pos1, pos2);
        eof1 = s1->Eof();
        eof2 = s2->Eof();
        EXPECT_EQ(eof1, eof2);
    }
    // 34. ss->Read(buf, 50000, NULL);
    count = 50000;
    res1 = s1->Read(buf, count, &bw1);
    res2 = s2->Read(buf2, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);
    for (unsigned i = 0; i < count; i++)
        EXPECT_EQ(buf[i], buf2[i]);

    // 35. ss->SetPos(5000000);
    pos1 = s1->SetPos(5000000);
    pos2 = s2->SetPos(5000000);
    EXPECT_EQ(pos1, pos2);
    // 36. ss->Write(buf, 500000, NULL);
    count = 500000;
    res1 = s1->Write(buf, count, &bw1);
    res2 = s2->Write(buf, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, count);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);
    // 37. ss->SetPos(4800000);
    pos1 = s1->SetPos(4800000);
    pos2 = s2->SetPos(4800000);
    EXPECT_EQ(pos1, pos2);
    // 38. ss->Read(buf, 500000, NULL);
    count = 500000;
    res1 = s1->Read(buf, count, &bw1);
    res2 = s2->Read(buf2, count, &bw2);
    EXPECT_EQ(res1, res2);
    EXPECT_EQ(bw1, bw2);
    pos1 = s1->GetPos();
    pos2 = s2->GetPos();
    EXPECT_EQ(pos1, pos2);
    eof1 = s1->Eof();
    eof2 = s2->Eof();
    EXPECT_EQ(eof1, eof2);
    for (unsigned i = 0; i < count; i++)
        EXPECT_EQ(buf[i], buf2[i]);

    for (int i = 0; i < 20; i++) {
        int op = (rand() & 15) < 5;
        long offset = (rand() & 0x7FFFF);
        long foffset = (rand() & 0x3FFFFF);
        long size = (rand() & 0x3FFFF);
        // 39. ss->SetPos(foffset);
        pos1 = s1->SetPos(foffset);
        pos2 = s2->SetPos(foffset);
        EXPECT_EQ(pos1, pos2);
        if (op == 0) {
            // read
            // 40. ss->Read(buf + offset, size, NULL);
            count = size;
            res1 = s1->Read(buf + offset, count, &bw1);
            res2 = s2->Read(buf2, count, &bw2);
            EXPECT_EQ(res1, res2);
            EXPECT_EQ(bw1, bw2);
            pos1 = s1->GetPos();
            pos2 = s2->GetPos();
            EXPECT_EQ(pos1, pos2);
            eof1 = s1->Eof();
            eof2 = s2->Eof();
            EXPECT_EQ(eof1, eof2);
            for (unsigned i = 0; i < count; i++)
                EXPECT_EQ(buf[i + offset], buf2[i]);
        } else {
            // 41. ss->Write(buf + offset, size, NULL);
            count = size;
            res1 = s1->Write(buf + offset, count, &bw1);
            res2 = s2->Write(buf + offset, count, &bw2);
            EXPECT_EQ(res1, res2);
            EXPECT_EQ(bw1, count);
            EXPECT_EQ(bw1, bw2);
            pos1 = s1->GetPos();
            pos2 = s2->GetPos();
            EXPECT_EQ(pos1, pos2);
            eof1 = s1->Eof();
            eof2 = s2->Eof();
            EXPECT_EQ(eof1, eof2);
        }
    }
}
