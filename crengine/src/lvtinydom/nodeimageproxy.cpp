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

#include "nodeimageproxy.h"
#include <ldomnode.h>
#include <ldomdocument.h>

bool NodeImageProxy::Decode(LVImageDecoderCallback* callback) {
    LVImageSourceRef img = _node->getDocument()->getObjectImageSource(_refName);
    if (img.isNull())
        return false;
    return img->Decode(callback);
}
