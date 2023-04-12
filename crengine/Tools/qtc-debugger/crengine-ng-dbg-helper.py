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
    lxmlAttributePtrType = d.createType('lxmlAttribute').pointer()
    boolType = d.createType('bool')
    lUInt8Type = d.createType('lUInt8')
    lUInt16Type = d.createType('lUInt16')
    lString32Type = d.createType('lString32')
    # Obtain data
    brokenValue = False
    document = None
    try:
        #document = d.parseAndEvaluate('ldomNode::_documentInstances[' + str(_handle_docIndex) + ']')
        _documentInstances = d.parseAndEvaluate('ldomNode::_documentInstances')
        documentAddr = _documentInstances.address() + _handle_docIndex * d.ptrSize()
        document = d.createValue(documentAddr, ldomDocumentPtrType)
        isNull = _handle_dataIndex == 0 or document.address() == 0
    except:
        document = None
        isNull = True
        brokenValue = True
    dataIndex = _handle_dataIndex & 0xFFFFFFF1
    #isPersistent = (_handle_dataIndex & 0x02) != 0
    isTextNode = not isNull and (_handle_dataIndex & 0x01) == 0
    isElementNode = not isNull and (_handle_dataIndex & 0x01) == 1
    _nodeType = _handle_dataIndex & 0x0F
    nodeType = ''
    data_field_name = '_data.???'
    data_field_value = _data['_text_ptr']
    nodeLevel = -1
    try:
        nodeLevellUInt8 = d.call(lUInt8Type, value, 'getNodeLevel')
        nodeLevel = nodeLevellUInt8.cast(d.intType())
    except:
        brokenValue = True
    if isNull or brokenValue:
        if brokenValue:
            nodeType = 'broken'
        else:
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
    childNodeCount = 0;
    attrCount = 0
    nodeNsName = None
    nodeName = None
    nodeText = None
    xpathSegment = None
    try:
        xpathSegment = d.call(lString32Type, value, 'getXPathSegment')
        childNodeCount = d.call(d.intType(), value, 'getChildCount').integer()
        attrCount = d.call(d.intType(), value, 'getAttrCount').integer()
        if isElementNode:
            nodeNsName = d.call(lString32Type, value, 'getNodeNsName')
            nodeName = d.call(lString32Type, value, 'getNodeName')
        elif isTextNode:
            nodeText = d.call(lString32Type, value, 'getText', '0', '0')
    except:
        brokenValue = True
    # Value & properties count
    strValue = nodeType + ': '
    strValue += '`' + _lString32_to_str(d, xpathSegment) + '`'
    strValue += ': '
    if isElementNode:
        nodeNsNameStr = _lString32_to_str(d, nodeNsName)
        if len(nodeNsNameStr) > 0:
            strValue += nodeNsNameStr + ':'
        strValue += '\'' + _lString32_to_str(d, nodeName) + '\''
        d.putNumChild(17)
    elif isTextNode:
        strValue += '\'' + _lString32_to_str(d, nodeText) + '\''
        d.putNumChild(12)
    else:
        # null/broken
        d.putNumChild(10)
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
            d.putCallItem('isRoot', boolType, value, 'isRoot')
            d.putIntItem('nodeLevel', nodeLevel)
            d.putCallItem('parentIndex', d.intType(), value, 'getParentIndex')
            d.putCallItem('nodeIndex', d.intType(), value, 'getNodeIndex')
            if isElementNode:
                d.putCallItem('nodeId', lUInt16Type, value, 'getNodeId')
                d.putCallItem('nodeNsId', lUInt16Type, value, 'getNodeNsId')
                d.putSubItem('nodeName', nodeName)
                d.putSubItem('nodeNsName', nodeNsName)
            if isTextNode:
                d.putSubItem('text', nodeText)
            d.putSubItem('xpathSegment', xpathSegment)
            if isElementNode:
                # children
                with SubItem(d, 'children'):
                    d.putName('children')
                    d.putValue('count=' + str(childNodeCount), 'ucs2')
                    if childNodeCount > 0:
                        d.putNumChild(childNodeCount)
                        if d.isExpanded():
                            with Children(d):
                                for i in range(childNodeCount):
                                    d.putCallItem(i, ldomNodePtrType, value, 'getChildNode', str(i))
                # attributes
                with SubItem(d, 'attributes'):
                    d.putName('attributes')
                    d.putValue('count=' + str(attrCount), 'ucs2')
                    if attrCount > 0:
                        d.putNumChild(attrCount)
                        if d.isExpanded():
                            with Children(d):
                                for i in range(attrCount):
                                    d.putCallItem(i, lxmlAttributePtrType, value, 'getAttribute', str(i))
            d.putCallItem('parent', ldomNodePtrType, value, 'getParentNode')

def qdump__ldomXPointer(d, value):
    # Inner fields
    _dataPtr = value['_data']
    _data = _dataPtr.dereference()
    _data_dataIndex = _data['_dataIndex'].integer()
    _data_offset = _data['_offset'].integer()
    document = _data['_doc']
    documentPtrInt = document.address()
    # Some required type refs
    ldomNodePtrType = d.createType('ldomNode').pointer()
    lString32Type = d.createType('lString32')
    # Obtain data
    node = None
    try:
        node = d.call(ldomNodePtrType, value, 'getNode')
    except:
        pass
    isNull = _data_dataIndex == 0 or documentPtrInt == 0
    ptype = 'null'
    isPointer = not isNull and _data_offset >= 0
    isPath = not isNull and _data_offset == -1
    if isPath:
        ptype = 'path'
    elif isPointer:
        ptype = 'ptr'
    strRepr = None
    try:
        strRepr = d.call(lString32Type, value, 'toString', 'XPATH_USE_NAMES')
    except:
        pass
    # Value & properties
    strValue = ptype
    if not isNull:
        strValue += ": " + _lString32_to_str(d, strRepr)
    d.putValue(strValue, 'ucs2')
    d.putNumChild(5)
    if d.isExpanded():
        with Children(d):
            d.putSubItem('_data', _dataPtr)
            d.putSubItem('document', document)
            d.putSubItem('node', node)
            with SubItem(d, 'type'):
                d.putName('type')
                d.putValue(ptype, 'ucs2')
            d.putSubItem(ptype, strRepr)
