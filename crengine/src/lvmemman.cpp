/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007-2009,2011-2015 Vadim Lopatin <coolreader.org@gmail.com>
 *   Copyright (C) 2020 poire-z <poire-z@users.noreply.github.com>         *
 *   Copyright (C) 2020,2021,2024 Aleksey Chernov <valexlin@gmail.com>     *
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
 * \file lvmemman.cpp
 * \brief memory manager implementation
 */

#include <lvmemman.h>
#include <lvref.h>
#include <lvstreamutils.h>
#include <crlog.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _LINUX
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif
#include <signal.h>
#include <unistd.h>
#endif

#ifdef _DEBUG
#include <stdexcept>
#include <string>
#endif

#define MAX_FILENANE_LEN 2048
static char file_to_remove_on_crash[MAX_FILENANE_LEN] = "";

void crSetFileToRemoveOnFatalError(const char* filename) {
    if (NULL == filename) {
        file_to_remove_on_crash[0] = 0;
    } else {
        size_t len = strlen(filename);
        if (len < MAX_FILENANE_LEN)
            strncpy(file_to_remove_on_crash, filename, MAX_FILENANE_LEN);
        else
            file_to_remove_on_crash[0] = 0;
    }
}

#ifdef _LINUX
static struct sigaction old_sa[NSIG];

void cr_sigaction(int signal, siginfo_t* info, void* reserved) {
    CR_UNUSED2(info, reserved);
    if (file_to_remove_on_crash[0])
        unlink(file_to_remove_on_crash);
    CRLog::error("cr_sigaction(%d)", signal);
    old_sa[signal].sa_handler(signal);
}
#endif

void crSetSignalHandler() {
#ifdef _LINUX
    static bool signals_are_set = false;
    if (signals_are_set)
        return;
    signals_are_set = true;
    struct sigaction handler;
    memset(&handler, 0, sizeof(handler));
    handler.sa_sigaction = cr_sigaction;
    handler.sa_flags = SA_RESETHAND;
#define CATCHSIG(X) sigaction(X, &handler, &old_sa[X])
    CATCHSIG(SIGILL);
    CATCHSIG(SIGABRT);
    CATCHSIG(SIGBUS);
    CATCHSIG(SIGFPE);
    CATCHSIG(SIGSEGV);
    //CATCHSIG(SIGSTKFLT);
    CATCHSIG(SIGPIPE);
#endif // _LINUX
}

/// default fatal error handler: uses exit()
void lvDefFatalErrorHandler(int errorCode, const char* errorText) {
    char strbuff[16];
    snprintf(strbuff, 16, "%d", errorCode);
    fprintf(stderr, "FATAL ERROR #%s: %s\n", strbuff, errorText);
#ifdef _DEBUG
    std::string errstr = std::string("FATAL ERROR #") + std::string(strbuff) + std::string(": ") + std::string(errorText);
    throw std::runtime_error(errstr);
#endif
    exit(errorCode);
}

lv_FatalErrorHandler_t* lvFatalErrorHandler = &lvDefFatalErrorHandler;

void crFatalError(int code, const char* errorText) {
    if (file_to_remove_on_crash[0])
        LVDeleteFile(Utf8ToUnicode(lString8(file_to_remove_on_crash)));
    lvFatalErrorHandler(code, errorText);
}

/// set fatal error handler
void crSetFatalErrorHandler(lv_FatalErrorHandler_t* handler) {
    lvFatalErrorHandler = handler;
}

ref_count_rec_t ref_count_rec_t::null_ref(NULL);
ref_count_rec_t ref_count_rec_t::protected_null_ref(NULL);

#if (LDOM_USE_OWN_MEM_MAN == 1)
ldomMemManStorage* pmsREF = NULL;

ldomMemManStorage* block_storages[LOCAL_STORAGE_COUNT] = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

inline int blockSizeToStorageIndex(size_t n) {
    return (n + ((1 << BLOCK_SIZE_GRANULARITY) - 1)) >> BLOCK_SIZE_GRANULARITY;
}

void* ldomAlloc(size_t n) {
    n = blockSizeToStorageIndex(n);
    if (n < LOCAL_STORAGE_COUNT) {
        if (block_storages[n] == NULL) {
            block_storages[n] = new ldomMemManStorage((n + 1) * BLOCK_SIZE_GRANULARITY);
        }
        return block_storages[n]->alloc();
    } else {
        return malloc(n);
    }
}

void ldomFree(void* p, size_t n) {
    n = blockSizeToStorageIndex(n);
    if (n < LOCAL_STORAGE_COUNT) {
        if (block_storages[n] == NULL) {
            crFatalError();
        }
        block_storages[n]->free((ldomMemBlock*)p);
    } else {
        free(p);
    }
}
#endif
