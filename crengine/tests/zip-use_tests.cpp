/***************************************************************************
 *   crengine-ng, unit testing                                             *
 *   Copyright (C) 2022 Aleksey Chernov <valexlin@gmail.com>               *
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
 * \file zip-use_tests.cpp
 * \brief Tests zip-archive usage.
 */

#include <lvstream.h>
#include <lvstreamutils.h>
#include <lvcontaineriteminfo.h>
#include <crlog.h>

#include "gtest/gtest.h"

#include "make-rand-file.h"
#include "compare-two-binfiles.h"

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

#ifndef ZIP_PROG
#error Please define ZIP_PROG, which points to the zip program (Info-ZIP) executable
#endif

static const int s_rand1_files_count = 4;
static const char* s_rand1_files_name[s_rand1_files_count] = {
    "text_text_file1.txt",
    "text_bin_file1.bin",
    "text_text_file2.txt",
    "text_text_file3.txt"
};
static const bool s_rand1_files_type[s_rand1_files_count] = {
    true,
    false,
    true,
    true
};
static const int s_rand1_files_size[s_rand1_files_count] = {
    10 * 1024,
    100 * 1024,
    25 * 1024,
    50 * 1024
};
static const char* s_archive1_name = "archive1.zip";

static const int s_rand2_files_count = 1;
static const char* s_rand2_files_name[s_rand2_files_count] = {
    "кириллица1.txt",
};
static const bool s_rand2_files_type[s_rand2_files_count] = {
    true,
};
static const int s_rand2_files_size[s_rand2_files_count] = {
    10 * 1024,
};
static const char* s_archive2_name = "кириллица1.txt.zip";

static const char* s_archive3_name = "archive1-trunc.zip";

// Fixtures

class ZipUsageTests: public testing::Test
{
protected:
    bool m_initOK;
protected:
    ZipUsageTests()
            : testing::Test() {
        m_initOK = false;
    }

    virtual void SetUp() override {
        bool res = true;
        for (int i = 0; i < s_rand1_files_count; i++) {
            res = crengine_ng::unittesting::makeRandomFile(s_rand1_files_name[i], s_rand1_files_type[i], s_rand1_files_size[i]);
            if (!res)
                break;
        }
        if (res)
            res = makeZipArchive(s_archive1_name, s_rand1_files_name, s_rand1_files_count);
        if (res) {
            res = false;
            LVStreamRef stream = LVOpenFileStream(s_archive1_name, LVOM_READ);
            if (!stream.isNull()) {
                lvsize_t size = stream->GetSize();
                lvsize_t bit_size = size / 2;
                void* buffer = malloc(bit_size);
                if (buffer) {
                    lvsize_t dw;
                    lverror_t err = stream->Read(buffer, bit_size, &dw);
                    if (LVERR_OK == err) {
                        LVStreamRef bit_stream = LVOpenFileStream(s_archive3_name, LVOM_WRITE);
                        if (!bit_stream.isNull()) {
                            err = bit_stream->Write(buffer, bit_size, &dw);
                            res = LVERR_OK == err;
                        }
                    }
                    free(buffer);
                }
            }
        }
        if (res) {
            for (int i = 0; i < s_rand2_files_count; i++) {
                res = crengine_ng::unittesting::makeRandomFile(s_rand2_files_name[i], s_rand2_files_type[i], s_rand2_files_size[i]);
                if (!res)
                    break;
            }
        }
        if (res)
            res = makeZipArchive(s_archive2_name, s_rand2_files_name, s_rand2_files_count);
        m_initOK = res;
    }

    virtual void TearDown() override {
        LVDeleteFile(lString8(s_archive1_name));
        for (int i = 0; i < s_rand1_files_count; i++) {
            LVDeleteFile(lString8(s_rand1_files_name[i]));
        }
        LVDeleteFile(lString8(s_archive2_name));
        for (int i = 0; i < s_rand2_files_count; i++) {
            LVDeleteFile(lString8(s_rand2_files_name[i]));
        }
        LVDeleteFile(lString8(s_archive3_name));
    }
private:
    bool makeZipArchive(const char* arcName, const char** files, size_t files_count) {
        // create zip-archive using zip program (Info-ZIP)
        bool res = false;
        const char** args = (const char**)malloc((files_count + 4) * sizeof(const char*));
        args[0] = ZIP_PROG;
        args[1] = "-9";
        args[2] = arcName;
        for (size_t i = 0; i < files_count; i++) {
            args[3 + i] = files[i];
        }
        args[3 + files_count] = NULL;
        res = execute_process(ZIP_PROG, args);
        free((void*)args);
        return res;
    }

    bool execute_process(const char* prog_exe, const char** args) {
        bool res = false;
#ifdef _WIN32
        lString8 commandLine;
        const char** ptr = args;
        commandLine = commandLine.append(*ptr);
        ptr++;
        while (*ptr) {
            commandLine = commandLine.append(" ");
            commandLine = commandLine.append(*ptr);
            ptr++;
        }
        lString16 commandLine16 = UnicodeToUtf16(Utf8ToUnicode(commandLine));
        STARTUPINFOW startup_info;
        PROCESS_INFORMATION proc_info;
        memset(&startup_info, 0, sizeof(startup_info));
        startup_info.cb = sizeof(startup_info);
        startup_info.dwFlags = STARTF_USESTDHANDLES;
        memset(&proc_info, 0, sizeof(proc_info));
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;
        HANDLE nul_handle = CreateFile("NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        startup_info.hStdInput = NULL;
        startup_info.hStdError = nul_handle;
        startup_info.hStdOutput = nul_handle;
        BOOL ret = CreateProcessW(NULL, (LPWSTR)commandLine16.c_str(), NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &startup_info, &proc_info);
        if (0 != ret) {
            DWORD ret2 = WaitForSingleObject(proc_info.hProcess, INFINITE);
            CloseHandle(proc_info.hProcess);
            CloseHandle(proc_info.hThread);
            res = WAIT_OBJECT_0 == ret2;
        } else {
            DWORD err = GetLastError();
            CRLog::error("execute_process() failed: error=%d", err);
        }
        CloseHandle(nul_handle);
#else
        int pid = fork();
        if (0 == pid) {
            // child process
            execv(prog_exe, (char* const*)args);
        } else if (-1 == pid) {
            // error
            CRLog::error("fork() failed!");
            fprintf(stderr, "fork() failed!\n");
            res = false;
        } else {
            // this (main) process
            int stat_loc;
            int ret;
            do {
                ret = waitpid(pid, &stat_loc, 0);
            } while (ret == -1 && errno == EINTR);
            if (ret == pid) {
                res = true;
            } else {
                CRLog::error("waitpid() failed, errno=%d!", errno);
                fprintf(stderr, "waitpid() failed, errno=%d!\n", errno);
            }
        }
#endif
        return res;
    }
};

// units tests

TEST_F(ZipUsageTests, ListContents) {
    CRLog::info("=====================");
    CRLog::info("Starting ListContents");
    ASSERT_TRUE(m_initOK);

    LVStreamRef stream = LVOpenFileStream(s_archive1_name, LVOM_READ);
    ASSERT_TRUE(!stream.isNull());
    LVContainerRef arc = LVOpenArchieve(stream);
    ASSERT_TRUE(!arc.isNull());
    EXPECT_EQ(arc->GetObjectCount(), s_rand1_files_count);
    for (int i = 0; i < arc->GetObjectCount(); i++) {
        const LVContainerItemInfo* item = arc->GetObjectInfo(i);
        EXPECT_STREQ(s_rand1_files_name[i], UnicodeToUtf8(item->GetName()).c_str());
        EXPECT_EQ(s_rand1_files_size[i], item->GetSize());
        if (item->GetSize() > 0)
            EXPECT_GT(item->GetPackSize(), 0);
    }

    CRLog::info("Finished ListContents");
    CRLog::info("=====================");
}

TEST_F(ZipUsageTests, ReadCyrillicEntries) {
    CRLog::info("============================");
    CRLog::info("Starting ReadCyrillicEntries");
    ASSERT_TRUE(m_initOK);

    // This is the same as 'ListContents', but there are entries in the archive with names in Cyrillic.
    // Here we check that the UTF8 encoded entry names are read correctly.
    LVStreamRef stream = LVOpenFileStream(s_archive2_name, LVOM_READ);
    ASSERT_TRUE(!stream.isNull());
    LVContainerRef arc = LVOpenArchieve(stream);
    ASSERT_TRUE(!arc.isNull());
    EXPECT_EQ(arc->GetObjectCount(), s_rand2_files_count);
    for (int i = 0; i < arc->GetObjectCount(); i++) {
        const LVContainerItemInfo* item = arc->GetObjectInfo(i);
        EXPECT_STREQ(s_rand2_files_name[i], UnicodeToUtf8(item->GetName()).c_str());
        EXPECT_EQ(s_rand2_files_size[i], item->GetSize());
        if (item->GetSize() > 0)
            EXPECT_GT(item->GetPackSize(), 0);
    }

    CRLog::info("Finished ReadCyrillicEntries");
    CRLog::info("============================");
}

TEST_F(ZipUsageTests, ListContentsInTruncArc) {
    CRLog::info("===============================");
    CRLog::info("Starting ListContentsInTruncArc");
    ASSERT_TRUE(m_initOK);

    // This is the same as 'ListContents', but with a truncated (broken) archive.
    // Here we check that we can work with such an archive.
    LVStreamRef stream = LVOpenFileStream(s_archive3_name, LVOM_READ);
    ASSERT_TRUE(!stream.isNull());
    LVContainerRef arc = LVOpenArchieve(stream);
    ASSERT_TRUE(!arc.isNull());
    EXPECT_TRUE(arc->GetObjectCount() > 0);
    EXPECT_TRUE(arc->GetObjectCount() < s_rand1_files_count);
    for (int i = 0; i < arc->GetObjectCount(); i++) {
        const LVContainerItemInfo* item = arc->GetObjectInfo(i);
        EXPECT_STREQ(s_rand1_files_name[i], UnicodeToUtf8(item->GetName()).c_str());
        EXPECT_EQ(s_rand1_files_size[i], item->GetSize());
        if (item->GetSize() > 0)
            EXPECT_GT(item->GetPackSize(), 0);
    }

    CRLog::info("Finished ListContentsInTruncArc");
    CRLog::info("===============================");
}

TEST_F(ZipUsageTests, ExtractAll) {
    CRLog::info("===================");
    CRLog::info("Starting ExtractAll");
    ASSERT_TRUE(m_initOK);

    LVStreamRef stream = LVOpenFileStream(s_archive1_name, LVOM_READ);
    ASSERT_TRUE(!stream.isNull());
    LVContainerRef arc = LVOpenArchieve(stream);
    ASSERT_TRUE(!arc.isNull());
    EXPECT_EQ(arc->GetObjectCount(), s_rand1_files_count);
    for (int i = 0; i < arc->GetObjectCount(); i++) {
        const LVContainerItemInfo* item = arc->GetObjectInfo(i);
        EXPECT_STREQ(s_rand1_files_name[i], UnicodeToUtf8(item->GetName()).c_str());
        EXPECT_EQ(s_rand1_files_size[i], item->GetSize());
        LVStreamRef inner_stream = arc->OpenStream(item->GetName(), LVOM_READ);
        ASSERT_TRUE(!inner_stream.isNull());
        LVStreamRef out_stream = LVOpenFileStream("temp.dat", LVOM_WRITE);
        ASSERT_TRUE(!out_stream.isNull());
        lvsize_t dw = LVPumpStream(out_stream, inner_stream);
        inner_stream.Clear(); // close inner stream
        out_stream.Clear();   // close output file
        EXPECT_EQ(dw, item->GetSize());
        EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinaryFiles("temp.dat", s_rand1_files_name[i]));
        ASSERT_TRUE(LVDeleteFile(cs8("temp.dat")));
    }

    CRLog::info("Finished ExtractAll");
    CRLog::info("===================");
}

TEST_F(ZipUsageTests, ExtractInTruncArc) {
    CRLog::info("==========================");
    CRLog::info("Starting ExtractInTruncArc");
    ASSERT_TRUE(m_initOK);

    // This is the same as 'ExtractAll', but with a truncated (broken) archive.
    // Here we check that we can work with such an archive.
    LVStreamRef stream = LVOpenFileStream(s_archive3_name, LVOM_READ);
    ASSERT_TRUE(!stream.isNull());
    LVContainerRef arc = LVOpenArchieve(stream);
    ASSERT_TRUE(!arc.isNull());
    EXPECT_TRUE(arc->GetObjectCount() > 0);
    EXPECT_TRUE(arc->GetObjectCount() < s_rand1_files_count);
    for (int i = 0; i < arc->GetObjectCount(); i++) {
        const LVContainerItemInfo* item = arc->GetObjectInfo(i);
        EXPECT_STREQ(s_rand1_files_name[i], UnicodeToUtf8(item->GetName()).c_str());
        EXPECT_EQ(s_rand1_files_size[i], item->GetSize());
        LVStreamRef inner_stream = arc->OpenStream(item->GetName(), LVOM_READ);
        ASSERT_TRUE(!inner_stream.isNull());
        LVStreamRef out_stream = LVOpenFileStream("temp.dat", LVOM_WRITE);
        ASSERT_TRUE(!out_stream.isNull());
        lvsize_t dw = LVPumpStream(out_stream, inner_stream);
        inner_stream.Clear(); // close inner stream
        out_stream.Clear();   // close output file
        EXPECT_EQ(dw, item->GetSize());
        EXPECT_TRUE(crengine_ng::unittesting::compareTwoBinaryFiles("temp.dat", s_rand1_files_name[i]));
        ASSERT_TRUE(LVDeleteFile(cs8("temp.dat")));
    }

    CRLog::info("Finished ExtractInTruncArc");
    CRLog::info("==========================");
}

TEST_F(ZipUsageTests, PackSize) {
    CRLog::info("=================");
    CRLog::info("Starting PackSize");
    ASSERT_TRUE(m_initOK);

    LVStreamRef stream = LVOpenFileStream(s_archive1_name, LVOM_READ);
    ASSERT_TRUE(!stream.isNull());
    LVContainerRef arc = LVOpenArchieve(stream);
    ASSERT_TRUE(!arc.isNull());
    EXPECT_EQ(arc->GetObjectCount(), s_rand1_files_count);
    lvsize_t total_unpack_size = 0;
    lvsize_t total_pack_size = 0;
    for (int i = 0; i < arc->GetObjectCount(); i++) {
        const LVContainerItemInfo* item = arc->GetObjectInfo(i);
        total_unpack_size += item->GetSize();
        total_pack_size += item->GetPackSize();
        EXPECT_STREQ(s_rand1_files_name[i], UnicodeToUtf8(item->GetName()).c_str());
        EXPECT_EQ(s_rand1_files_size[i], item->GetSize());
        if (item->GetSize() > 0)
            EXPECT_GT(item->GetPackSize(), 0);
    }
    EXPECT_GT(total_pack_size, 0);
    EXPECT_LT(total_pack_size, total_unpack_size);

    CRLog::info("Finished PackSize");
    CRLog::info("=================");
}
