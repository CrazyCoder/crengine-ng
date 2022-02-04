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

#ifndef __RENDERRECTACCESSOR_H_INCLUDED__
#define __RENDERRECTACCESSOR_H_INCLUDED__

#include <lvstyles.h>

class RenderRectAccessor: public lvdomElementFormatRec
{
    ldomNode* _node;
    bool _modified;
    bool _dirty;
public:
    //RenderRectAccessor & operator -> () { return *this; }
    int getX();
    int getY();
    int getWidth();
    int getHeight();
    void getRect(lvRect& rc);
    void setX(int x);
    void setY(int y);
    void setWidth(int w);
    void setHeight(int h);

    int getInnerWidth();
    int getInnerX();
    int getInnerY();
    void setInnerX(int x);
    void setInnerY(int y);
    void setInnerWidth(int w);

    int getUsableLeftOverflow();
    int getUsableRightOverflow();
    void setUsableLeftOverflow(int dx);
    void setUsableRightOverflow(int dx);

    int getTopOverflow();
    int getBottomOverflow();
    void setTopOverflow(int dy);
    void setBottomOverflow(int dy);

    int getBaseline();
    void setBaseline(int baseline);
    int getListPropNodeIndex();
    void setListPropNodeIndex(int idx);
    int getLangNodeIndex();
    void setLangNodeIndex(int idx);

    unsigned short getFlags();
    void setFlags(unsigned short flags);

    void getTopRectsExcluded(int& lw, int& lh, int& rw, int& rh);
    void setTopRectsExcluded(int lw, int lh, int rw, int rh);
    void getNextFloatMinYs(int& left, int& right);
    void setNextFloatMinYs(int left, int right);
    void getInvolvedFloatIds(int& float_count, lUInt32* float_ids);
    void setInvolvedFloatIds(int float_count, lUInt32* float_ids);

    void push();
    void clear();
    RenderRectAccessor(ldomNode* node);
    ~RenderRectAccessor();
};

#endif // __RENDERRECTACCESSOR_H_INCLUDED__
