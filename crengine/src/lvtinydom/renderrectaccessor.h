/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2010 Vadim Lopatin <coolreader.org@gmail.com>           *
 *   Copyright (C) 2019,2020 poire-z <poire-z@users.noreply.github.com>    *
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
