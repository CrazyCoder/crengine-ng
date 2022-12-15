/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2010 Vadim Lopatin <coolreader.org@gmail.com>           *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
 *   Copyright (C) 2020 NiLuJe <ninuje@gmail.com>                          *
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

#ifndef __NODEIMAGEPROXY_H_INCLUDED__
#define __NODEIMAGEPROXY_H_INCLUDED__

#include <lvimagesource.h>
#include <lvstring.h>
#include <ldomnode.h>
#include <ldomdocument.h>

class NodeImageProxy: public LVImageSource
{
    ldomNode* _node;
    lString32 _refName;
    int _dx;
    int _dy;
public:
    NodeImageProxy(ldomNode* node, lString32 refName, int dx, int dy)
            : _node(node)
            , _refName(refName)
            , _dx(dx)
            , _dy(dy) {
    }
    virtual ~NodeImageProxy() { }

    virtual ldomNode* GetSourceNode() {
        return NULL;
    }
    virtual LVStream* GetSourceStream() {
        return NULL;
    }

    virtual void Compact() { }
    virtual int GetWidth() const {
        return _dx;
    }
    virtual int GetHeight() const {
        return _dy;
    }
    virtual bool Decode(LVImageDecoderCallback* callback) {
        LVImageSourceRef img = _node->getDocument()->getObjectImageSource(_refName);
        if (img.isNull())
            return false;
        return img->Decode(callback);
    }
};

#endif // __NODEIMAGEPROXY_H_INCLUDED__
