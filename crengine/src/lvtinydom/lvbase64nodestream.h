/**
 * CoolReader Engine
 *
 * (c) Vadim Lopatin, 2000-2011
 * (c) Other CoolReader authors (See AUTHORS file)
 *
 * This source code is distributed under the terms of
 * GNU General Public License version 2
 * See LICENSE file for details
 */

#ifndef __LVBASE64NODESTREAM_H_INCLUDED__
#define __LVBASE64NODESTREAM_H_INCLUDED__

#include "../lvstream/lvnamedstream.h"

struct ldomNode;

#define BASE64_BUF_SIZE 128
class LVBase64NodeStream: public LVNamedStream
{
private:
    ldomNode* m_elem;
    ldomNode* m_curr_node;
    lString32 m_curr_text;
    int m_text_pos;
    lvsize_t m_size;
    lvpos_t m_pos;

    int m_iteration;
    lUInt32 m_value;

    lUInt8 m_bytes[BASE64_BUF_SIZE];
    int m_bytes_count;
    int m_bytes_pos;

    int readNextBytes();
    bool findNextTextNode();
    int bytesAvailable() {
        return m_bytes_count - m_bytes_pos;
    }
    bool rewind();
    bool skip(lvsize_t count);
public:
    LVBase64NodeStream(ldomNode* element);
    virtual ~LVBase64NodeStream() { }
    virtual bool Eof() {
        return m_pos >= m_size;
    }
    virtual lvsize_t GetSize() {
        return m_size;
    }

    virtual lvpos_t GetPos() {
        return m_pos;
    }

    virtual lverror_t GetPos(lvpos_t* pos) {
        if (pos)
            *pos = m_pos;
        return LVERR_OK;
    }

    virtual lverror_t Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t* newPos);
    virtual lverror_t Write(const void*, lvsize_t, lvsize_t*) {
        return LVERR_NOTIMPL;
    }
    virtual lverror_t Read(void* buf, lvsize_t size, lvsize_t* pBytesRead);
    virtual lverror_t SetSize(lvsize_t) {
        return LVERR_NOTIMPL;
    }
};

#endif // __LVBASE64NODESTREAM_H_INCLUDED__
