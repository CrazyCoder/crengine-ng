############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

############################################################################
#
# crengine-ng debugging helper for Qt Creator
# Copyright (C) 2020,2022,2023 Aleksey Chernov <valexlin@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
############################################################################

# This is a place to add your own dumpers for testing purposes.
# Any contents here will be picked up by GDB, LLDB, and CDB based
# debugging in Qt Creator automatically.

# To add dumpers that don't get overwritten, copy this file here
# to a safe location outside the Qt Creator installation and
# make this location known to Qt Creator using the Debugger >
# Locals & Expressions > Extra Debugging Helpers setting.

# Example to display a simple type
# template<typename U, typename V> struct MapNode
# {
#     U key;
#     V data;
# }
#
# def qdump__MapNode(d, value):
#    d.putValue("This is the value column contents")
#    d.putNumChild(2)
#    if d.isExpanded():
#        with Children(d):
#            # Compact simple case.
#            d.putSubItem("key", value["key"])
#            # Same effect, with more customization possibilities.
#            with SubItem(d, "data")
#                d.putItem("data", value["data"])

# Check http://doc.qt.io/qtcreator/creator-debugging-helpers.html
# for more details or look at qttypes.py, stdtypes.py, boosttypes.py
# for more complex examples.

from dumper import *


def qdump__lString8(d, value):
    pchunk = value['pchunk'].dereference()
    length = pchunk['len'].integer()
    size = pchunk['size'].integer()
    refCount = pchunk['refCount'].integer()
    buf = pchunk['buf']
    buf8 = buf['_8']
    d.check(length >= 0 and size >= 0)
    elided, shown = d.computeLimit(length, d.displayStringLimit)
    # TODO: fix incorrect maximum bytes read: bytes count != characters count in UTF-8
    mem = d.readMemory(buf8, shown)
    d.putValue(mem, 'utf8', elided=elided)
    d.putNumChild(3)
    if d.isExpanded():
        with Children(d):
            d.putIntItem('len', length)
            d.putIntItem('size', size)
            d.putIntItem('refCount', refCount)

def qdump__lString16(d, value):
    pchunk = value['pchunk'].dereference()
    length = pchunk['len'].integer()
    size = pchunk['size'].integer()
    refCount = pchunk['refCount'].integer()
    buf = pchunk['buf']
    buf8 = buf['_16']
    d.check(length >= 0 and size >= 0)
    elided, shown = d.computeLimit(length, d.displayStringLimit)
    # TODO: fix incorrect maximum bytes read: bytes count != 2*{characters count} in UTF-16 (when using surrogates)
    mem = d.readMemory(buf8, shown*2)
    d.putValue(mem, 'ucs2', elided=elided)
    d.putNumChild(3)
    if d.isExpanded():
        with Children(d):
            d.putIntItem('len', length)
            d.putIntItem('size', size)
            d.putIntItem('refCount', refCount)

def qdump__lString32(d, value):
    pchunk = value['pchunk'].dereference()
    length = pchunk['len'].integer()
    size = pchunk['size'].integer()
    refCount = pchunk['refCount'].integer()
    buf = pchunk['buf']
    buf8 = buf['_32']
    d.check(length >= 0 and size >= 0)
    elided, shown = d.computeLimit(length, d.displayStringLimit)
    mem = d.readMemory(buf8, shown*4)
    d.putValue(mem, 'ucs4', elided=elided)
    d.putNumChild(3)
    if d.isExpanded():
        with Children(d):
            d.putIntItem('len', length)
            d.putIntItem('size', size)
            d.putIntItem('refCount', refCount)

def _lString32_to_str(d, value):
    if value is None:
        return 'null'
    pchunk = value['pchunk'].dereference()
    length = pchunk['len'].integer()
    size = pchunk['size'].integer()
    buf = pchunk['buf']
    buf32 = buf['_32']
    d.check(length >= 0 and size >= 0)
    elided, shown = d.computeLimit(length, d.displayStringLimit)
    mem = d.readRawMemory(buf32, shown*4)
    return str(mem, encoding="utf_32")

def qdump__ldomNode(d, value):
    # Inner fields
    _handle = value['_handle']
    _data = value['_data']
    _handle_dataIndex = _handle['_dataIndex'].integer()
    _handle_docIndex = _handle['_docIndex'].integer()
    # Some required type refs
    ldomNodePtrType = d.createType('ldomNode').pointer()
    ldomDocumentPtrType = d.createType('ldomDocument').pointer()
    # Obtain data
    #document = d.parseAndEvaluate('ldomNode::_documentInstances[' + str(_handle_docIndex) + ']')
    _documentInstances = d.parseAndEvaluate('ldomNode::_documentInstances')
    documentAddr = _documentInstances.address() + _handle_docIndex * d.ptrSize()
    document = d.createValue(documentAddr, ldomDocumentPtrType)
    isNull = _handle_dataIndex == 0 or document.address() == 0
    dataIndex = _handle_dataIndex & 0xFFFFFFF1
    #isPersistent = (_handle_dataIndex & 0x02) != 0
    isTextNode = (_handle_dataIndex & 0x01) == 0
    isElementNode = (_handle_dataIndex & 0x01) == 1
    _nodeType = _handle_dataIndex & 0x0F
    nodeType = ''
    data_field_name = '_data.???'
    data_field_value = _data['_text_ptr']
    if isNull:
        nodeType = 'null'
    else:
        if _nodeType == 0:
            nodeType = 'text'
            data_field_name = '_data._text_ptr'
            data_field_value = _data['_text_ptr']
        elif _nodeType == 1:
            nodeType = 'element'
            data_field_name = '_data._elem_ptr'
            data_field_value = _data['_elem_ptr']
        elif _nodeType == 2:
            nodeType = 'p:text'
            data_field_name = '_data._ptext_addr'
            data_field_value = _data['_ptext_addr']
        elif _nodeType == 3:
            nodeType = 'p:element'
            data_field_name = '_data._pelem_addr'
            data_field_value = _data['_pelem_addr']
        else:
            nodeType = 'unknown'
    # Value & properties count
    strValue = nodeType + ': ???'
    d.putNumChild(5)
    d.putValue(strValue, 'ucs2')
    if d.isExpanded():
        with Children(d):
            d.putSubItem('_handle', _handle)
            d.putSubItem(data_field_name, data_field_value)
            d.putSubItem('document', document)
            with SubItem(d, 'nodeType'):
                d.putName('nodeType')
                d.putValue(nodeType, 'ucs2')
            d.putIntItem('dataIndex', dataIndex)

def qdump__ldomXPointer(d, value):
    # Inner fields
    _dataPtr = value['_data']
    _data = _dataPtr.dereference()
    _data_dataIndex = _data['_dataIndex'].integer()
    _data_offset = _data['_offset'].integer()
    document = _data['_doc']
    documentPtrInt = document.address()
    # Obtain data
    isNull = _data_dataIndex == 0 or documentPtrInt == 0
    ptype = 'null'
    isPointer = not isNull and _data_offset >= 0
    isPath = not isNull and _data_offset == -1
    if isPath:
        ptype = 'path'
    elif isPointer:
        ptype = 'ptr'
    # Value & properties
    strValue = ptype
    d.putValue(strValue, 'ucs2')
    d.putNumChild(3)
    if d.isExpanded():
        with Children(d):
            d.putSubItem('_data', _dataPtr)
            d.putSubItem('document', document)
            with SubItem(d, 'type'):
                d.putName('type')
                d.putValue(ptype, 'ucs2')
