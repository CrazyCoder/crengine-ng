/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007,2009,2011 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
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

#include "lvbase64nodestream.h"

#include <ldomnode.h>

// base64 decode table
static const signed char base64_decode_table[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //0..15
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //16..31   10
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, //32..47   20
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, //48..63   30
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,           //64..79   40
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, //80..95   50
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, //INDEX2..111  60
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1  //112..127 70
};

static bool FindNextNode(ldomNode*& node, ldomNode* root) {
    if (node->getChildCount() > 0) {
        // first child
        node = node->getChildNode(0);
        return true;
    }
    if (node->isRoot() || node == root)
        return false; // root node reached
    int index = node->getNodeIndex();
    ldomNode* parent = node->getParentNode();
    while (parent) {
        if (index < (int)parent->getChildCount() - 1) {
            // next sibling
            node = parent->getChildNode(index + 1);
            return true;
        }
        if (parent->isRoot() || parent == root)
            return false; // root node reached
        // up one level
        index = parent->getNodeIndex();
        parent = parent->getParentNode();
    }
    //if ( node->getNodeType() == LXML_TEXT_NODE )
    return false;
}

int LVBase64NodeStream::readNextBytes() {
    int bytesRead = 0;
    bool flgEof = false;
    while (bytesRead == 0 && !flgEof) {
        while (m_text_pos >= (int)m_curr_text.length()) {
            if (!findNextTextNode())
                return bytesRead;
        }
        int len = m_curr_text.length();
        const lChar32* txt = m_curr_text.c_str();
        for (; m_text_pos < len && m_bytes_count < BASE64_BUF_SIZE - 3; m_text_pos++) {
            lChar32 ch = txt[m_text_pos];
            if (ch < 128) {
                if (ch == '=') {
                    // end of stream
                    if (m_iteration == 2) {
                        m_bytes[m_bytes_count++] = (lUInt8)((m_value >> 4) & 0xFF);
                        bytesRead++;
                    } else if (m_iteration == 3) {
                        m_bytes[m_bytes_count++] = (lUInt8)((m_value >> 10) & 0xFF);
                        m_bytes[m_bytes_count++] = (lUInt8)((m_value >> 2) & 0xFF);
                        bytesRead += 2;
                    }
                    // stop!!!
                    //m_text_pos--;
                    m_iteration = 0;
                    flgEof = true;
                    break;
                } else {
                    int k = base64_decode_table[ch];
                    if (!(k & 0x80)) {
                        // next base-64 digit
                        m_value = (m_value << 6) | (k);
                        m_iteration++;
                        if (m_iteration == 4) {
                            //
                            m_bytes[m_bytes_count++] = (lUInt8)((m_value >> 16) & 0xFF);
                            m_bytes[m_bytes_count++] = (lUInt8)((m_value >> 8) & 0xFF);
                            m_bytes[m_bytes_count++] = (lUInt8)((m_value >> 0) & 0xFF);
                            m_iteration = 0;
                            m_value = 0;
                            bytesRead += 3;
                        }
                    } else {
                        //m_text_pos++;
                    }
                }
            }
        }
    }
    return bytesRead;
}

bool LVBase64NodeStream::findNextTextNode() {
    while (FindNextNode(m_curr_node, m_elem)) {
        if (m_curr_node->isText()) {
            m_curr_text = m_curr_node->getText();
            m_text_pos = 0;
            return true;
        }
    }
    return false;
}

bool LVBase64NodeStream::rewind() {
    m_curr_node = m_elem;
    m_pos = 0;
    m_bytes_count = 0;
    m_bytes_pos = 0;
    m_iteration = 0;
    m_value = 0;
    return findNextTextNode();
}

bool LVBase64NodeStream::skip(lvsize_t count) {
    while (count) {
        if (m_bytes_pos >= m_bytes_count) {
            m_bytes_pos = 0;
            m_bytes_count = 0;
            int bytesRead = readNextBytes();
            if (bytesRead == 0)
                return false;
        }
        int diff = (int)(m_bytes_count - m_bytes_pos);
        if (diff > (int)count)
            diff = (int)count;
        m_pos += diff;
        count -= diff;
    }
    return true;
}

LVBase64NodeStream::LVBase64NodeStream(ldomNode* element)
        : m_elem(element)
        , m_curr_node(element)
        , m_text_pos(0)
        , m_size(0)
        , m_pos(0) {
    // calculate size
    rewind();
    m_size = bytesAvailable();
    for (;;) {
        int bytesRead = readNextBytes();
        if (!bytesRead)
            break;
        m_bytes_count = 0;
        m_bytes_pos = 0;
        m_size += bytesRead;
    }
    // rewind
    rewind();
}

lverror_t LVBase64NodeStream::Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t* newPos) {
    lvpos_t npos = 0;
    lvpos_t currpos = GetPos();
    switch (origin) {
        case LVSEEK_SET:
            npos = offset;
            break;
        case LVSEEK_CUR:
            npos = currpos + offset;
            break;
        case LVSEEK_END:
            npos = m_size + offset;
            break;
    }
    if (npos > m_size)
        return LVERR_FAIL;
    if (npos != currpos) {
        if (npos < currpos) {
            if (!rewind() || !skip(npos))
                return LVERR_FAIL;
        } else {
            skip(npos - currpos);
        }
    }
    if (newPos)
        *newPos = npos;
    return LVERR_OK;
}

lverror_t LVBase64NodeStream::Read(void* buf, lvsize_t size, lvsize_t* pBytesRead) {
    lvsize_t bytesRead = 0;
    //fprintf( stderr, "Read()\n" );

    lUInt8* out = (lUInt8*)buf;

    while (size > 0) {
        int sz = bytesAvailable();
        if (!sz) {
            m_bytes_pos = m_bytes_count = 0;
            sz = readNextBytes();
            if (!sz) {
                if (!bytesRead || m_pos != m_size) //
                    return LVERR_FAIL;
                break;
            }
        }
        if (sz > (int)size)
            sz = (int)size;
        for (int i = 0; i < sz; i++)
            *out++ = m_bytes[m_bytes_pos++];
        size -= sz;
        bytesRead += sz;
        m_pos += sz;
    }

    if (pBytesRead)
        *pBytesRead = bytesRead;
    //fprintf( stderr, "    %d bytes read...\n", (int)bytesRead );
    return LVERR_OK;
}
