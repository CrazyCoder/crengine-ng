/**
 *  crengine-ng
 *
 *  \file lvtinydom_private.h
 *  \brief crengine-ng unit test module.
 *  Some private stuff extracted from lvtinydom.cpp
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LVTINYDOM_PRIVATE_H
#define LVTINYDOM_PRIVATE_H

enum CacheFileBlockType
{
    CBT_FREE = 0,
    CBT_INDEX = 1,
    CBT_TEXT_DATA,
    CBT_ELEM_DATA,
    CBT_RECT_DATA, //4
    CBT_ELEM_STYLE_DATA,
    CBT_MAPS_DATA,
    CBT_PAGE_DATA, //7
    CBT_PROP_DATA,
    CBT_NODE_INDEX,
    CBT_ELEM_NODE,
    CBT_TEXT_NODE,
    CBT_REND_PARAMS, //12
    CBT_TOC_DATA,
    CBT_PAGEMAP_DATA,
    CBT_STYLE_DATA,
    CBT_BLOB_INDEX, //16
    CBT_BLOB_DATA,
    CBT_FONT_DATA //18
};

#endif // LVTINYDOM_PRIVATE_H
