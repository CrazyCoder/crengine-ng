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

#ifndef __LXMLATTRIBUTE_H_INCLUDED__
#define __LXMLATTRIBUTE_H_INCLUDED__

#include <lvtinydom_common.h>

struct lxmlAttribute
{
    //
    lUInt16 nsid;
    lUInt16 id;
    lUInt32 index;
    inline bool compare(lUInt16 nsId, lUInt16 attrId) {
        return (nsId == nsid || nsId == LXML_NS_ANY) && (id == attrId);
    }
    inline void setData(lUInt16 nsId, lUInt16 attrId, lUInt32 valueIndex) {
        nsid = nsId;
        id = attrId;
        index = valueIndex;
    }
};

#endif // __LXMLATTRIBUTE_H_INCLUDED__
