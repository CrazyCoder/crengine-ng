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

#include "renderrectaccessor.h"

#include <ldomnode.h>

//#define DEBUG_RENDER_RECT_ACCESS
#ifdef DEBUG_RENDER_RECT_ACCESS
static signed char render_rect_flags[200000] = { 0 };
static void rr_lock(ldomNode* node) {
    int index = node->getDataIndex() >> 4;
    CRLog::debug("RenderRectAccessor(%d) lock", index);
    if (render_rect_flags[index])
        crFatalError(123, "render rect accessor: cannot get lock");
    render_rect_flags[index] = 1;
}
static void rr_unlock(ldomNode* node) {
    int index = node->getDataIndex() >> 4;
    CRLog::debug("RenderRectAccessor(%d) lock", index);
    if (!render_rect_flags[index])
        crFatalError(123, "render rect accessor: unlock w/o lock");
    render_rect_flags[index] = 0;
}
#endif

RenderRectAccessor::RenderRectAccessor(ldomNode* node)
        : _node(node)
        , _modified(false)
        , _dirty(false) {
#ifdef DEBUG_RENDER_RECT_ACCESS
    rr_lock(_node);
#endif
    _node->getRenderData(*this);
}

RenderRectAccessor::~RenderRectAccessor() {
    if (_modified)
        _node->setRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
    if (!_dirty)
        rr_unlock(_node);
#endif
}

void RenderRectAccessor::clear() {
    lvdomElementFormatRec::clear(); // will clear every field
    _modified = true;
    _dirty = false;
}

void RenderRectAccessor::push() {
    if (_modified) {
        _node->setRenderData(*this);
        _modified = false;
        _dirty = true;
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_unlock(_node);
#endif
    }
}

void RenderRectAccessor::setX(int x) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    if (_x != x) {
        _x = x;
        _modified = true;
    }
}
void RenderRectAccessor::setY(int y) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    if (_y != y) {
        _y = y;
        _modified = true;
    }
}
void RenderRectAccessor::setWidth(int w) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    if (_width != w) {
        _width = w;
        _modified = true;
    }
}
void RenderRectAccessor::setHeight(int h) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    if (_height != h) {
        _height = h;
        _modified = true;
    }
}

int RenderRectAccessor::getX() {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    return _x;
}
int RenderRectAccessor::getY() {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    return _y;
}
int RenderRectAccessor::getWidth() {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    return _width;
}
int RenderRectAccessor::getHeight() {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    return _height;
}
void RenderRectAccessor::getRect(lvRect& rc) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    rc.left = _x;
    rc.top = _y;
    rc.right = _x + _width;
    rc.bottom = _y + _height;
}

void RenderRectAccessor::setInnerX(int x) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    if (_inner_x != x) {
        _inner_x = x;
        _modified = true;
    }
}
void RenderRectAccessor::setInnerY(int y) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    if (_inner_y != y) {
        _inner_y = y;
        _modified = true;
    }
}
void RenderRectAccessor::setInnerWidth(int w) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    if (_inner_width != w) {
        _inner_width = w;
        _modified = true;
    }
}
int RenderRectAccessor::getInnerX() {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    return _inner_x;
}
int RenderRectAccessor::getInnerY() {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    return _inner_y;
}
int RenderRectAccessor::getInnerWidth() {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    return _inner_width;
}
int RenderRectAccessor::getUsableLeftOverflow() {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    return _usable_left_overflow;
}
int RenderRectAccessor::getUsableRightOverflow() {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    return _usable_right_overflow;
}
void RenderRectAccessor::setUsableLeftOverflow(int dx) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    if (dx < 0)
        dx = 0; // don't allow a negative value
    if (_usable_left_overflow != dx) {
        _usable_left_overflow = dx;
        _modified = true;
    }
}
void RenderRectAccessor::setUsableRightOverflow(int dx) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    if (dx < 0)
        dx = 0; // don't allow a negative value
    if (_usable_right_overflow != dx) {
        _usable_right_overflow = dx;
        _modified = true;
    }
}
int RenderRectAccessor::getTopOverflow() {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    return _top_overflow;
}
int RenderRectAccessor::getBottomOverflow() {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    return _bottom_overflow;
}
void RenderRectAccessor::setTopOverflow(int dy) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    if (dy < 0)
        dy = 0; // don't allow a negative value
    if (_top_overflow != dy) {
        _top_overflow = dy;
        _modified = true;
    }
}
void RenderRectAccessor::setBottomOverflow(int dy) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    if (dy < 0)
        dy = 0; // don't allow a negative value
    if (_bottom_overflow != dy) {
        _bottom_overflow = dy;
        _modified = true;
    }
}
int RenderRectAccessor::getBaseline() {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    return _baseline;
}
void RenderRectAccessor::setBaseline(int baseline) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    if (_baseline != baseline) {
        _baseline = baseline;
        _modified = true;
    }
}
int RenderRectAccessor::getListPropNodeIndex() {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    return _listprop_node_idx;
}
void RenderRectAccessor::setListPropNodeIndex(int idx) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    if (_listprop_node_idx != idx) {
        _listprop_node_idx = idx;
        _modified = true;
    }
}
int RenderRectAccessor::getLangNodeIndex() {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    return _lang_node_idx;
}
void RenderRectAccessor::setLangNodeIndex(int idx) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    if (_lang_node_idx != idx) {
        _lang_node_idx = idx;
        _modified = true;
    }
}
unsigned short RenderRectAccessor::getFlags() {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    return _flags;
}
void RenderRectAccessor::setFlags(unsigned short flags) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    if (_flags != flags) {
        _flags = flags;
        _modified = true;
    }
}
void RenderRectAccessor::getTopRectsExcluded(int& lw, int& lh, int& rw, int& rh) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    lw = _extra1 >> 16;    // Both stored in a single int slot (widths are
    rw = _extra1 & 0xFFFF; // constrained to lUint16 in many other places)
    lh = _extra2;
    rh = _extra3;
}
void RenderRectAccessor::setTopRectsExcluded(int lw, int lh, int rw, int rh) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    if (_extra2 != lh || _extra3 != rh || (_extra1 >> 16) != lw || (_extra1 & 0xFFFF) != rw) {
        _extra1 = (lw << 16) + rw;
        _extra2 = lh;
        _extra3 = rh;
        _modified = true;
    }
}
void RenderRectAccessor::getNextFloatMinYs(int& left, int& right) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    left = _extra4;
    right = _extra5;
}
void RenderRectAccessor::setNextFloatMinYs(int left, int right) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    if (_extra4 != left || _extra5 != right) {
        _extra4 = left;
        _extra5 = right;
        _modified = true;
    }
}
void RenderRectAccessor::getInvolvedFloatIds(int& float_count, lUInt32* float_ids) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    float_count = _extra0;
    if (float_count > 0)
        float_ids[0] = _extra1;
    if (float_count > 1)
        float_ids[1] = _extra2;
    if (float_count > 2)
        float_ids[2] = _extra3;
    if (float_count > 3)
        float_ids[3] = _extra4;
    if (float_count > 4)
        float_ids[4] = _extra5;
}
void RenderRectAccessor::setInvolvedFloatIds(int float_count, lUInt32* float_ids) {
    if (_dirty) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock(_node);
#endif
    }
    _extra0 = float_count;
    if (float_count > 0)
        _extra1 = float_ids[0];
    if (float_count > 1)
        _extra2 = float_ids[1];
    if (float_count > 2)
        _extra3 = float_ids[2];
    if (float_count > 3)
        _extra4 = float_ids[3];
    if (float_count > 4)
        _extra5 = float_ids[4];
    _modified = true;
}
