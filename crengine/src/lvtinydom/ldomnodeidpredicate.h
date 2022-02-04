/**
 * CoolReader Engine
 *
 * (c) Konstantin Potapov <pkbo@users.sourceforge.net>, 2020
 *
 * This source code is distributed under the terms of
 * GNU General Public License version 2
 * See LICENSE file for details
 */

#ifndef __LDOMNODEIDPREDICATE_H_INCLUDED__
#define __LDOMNODEIDPREDICATE_H_INCLUDED__

#include <ldomnode.h>

struct ldomNodeIdPredicate
{
    lUInt16 m_id;
    ldomNodeIdPredicate(lUInt16 id)
            : m_id(id) { }
    bool operator()(ldomNode* node) {
        return (node && node->getNodeId() == m_id);
    }
};

#endif // __LDOMNODEIDPREDICATE_H_INCLUDED__
