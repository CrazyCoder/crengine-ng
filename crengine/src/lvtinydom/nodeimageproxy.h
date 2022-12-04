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
