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

#include <lxmldocbase.h>
#include <ldomnode.h>
#include <lvdocprops.h>
#include <crlog.h>

/////////////////////////////////////////////////////////////////
/// lxmlDocument

lxmlDocBase::lxmlDocBase(int /*dataBufSize*/)
        : tinyNodeCollection()
        , _elementNameTable(MAX_ELEMENT_TYPE_ID)
        , _attrNameTable(MAX_ATTRIBUTE_TYPE_ID)
        , _nsNameTable(MAX_NAMESPACE_TYPE_ID)
        , _nextUnknownElementId(UNKNOWN_ELEMENT_TYPE_ID)
        , _nextUnknownAttrId(UNKNOWN_ATTRIBUTE_TYPE_ID)
        , _nextUnknownNsId(UNKNOWN_NAMESPACE_TYPE_ID)
        , _attrValueTable(DOC_STRING_HASH_SIZE)
        , _idNodeMap(8192)
        , _urlImageMap(1024)
        , _idAttrId(0)
        , _nameAttrId(0)
        //,_keepData(false)
        //,_mapped(false)
        , _pagesData(8192) {
    // create and add one data buffer
    _stylesheet.setDocument(this);
}

/// Copy constructor - copies ID tables contents
lxmlDocBase::lxmlDocBase(lxmlDocBase& doc)
        : tinyNodeCollection(doc)
        , _elementNameTable(doc._elementNameTable)         // Element Name<->Id map
        , _attrNameTable(doc._attrNameTable)               // Attribute Name<->Id map
        , _nsNameTable(doc._nsNameTable)                   // Namespace Name<->Id map
        , _nextUnknownElementId(doc._nextUnknownElementId) // Next Id for unknown element
        , _nextUnknownAttrId(doc._nextUnknownAttrId)       // Next Id for unknown attribute
        , _nextUnknownNsId(doc._nextUnknownNsId)           // Next Id for unknown namespace
        //lvdomStyleCache _styleCache;         // Style cache
        , _attrValueTable(doc._attrValueTable)
        , _idNodeMap(doc._idNodeMap)
        , _urlImageMap(1024)
        , _idAttrId(doc._idAttrId) // Id for "id" attribute name
                                   //,   _docFlags(doc._docFlags)
        , _pagesData(8192) {
}

/// Destructor
lxmlDocBase::~lxmlDocBase() {
}

void lxmlDocBase::onAttributeSet(lUInt16 attrId, lUInt32 valueId, ldomNode* node) {
    if (_idAttrId == 0)
        _idAttrId = _attrNameTable.idByName("id");
    if (_nameAttrId == 0)
        _nameAttrId = _attrNameTable.idByName("name");
    if (attrId == _idAttrId) {
        _idNodeMap.set(valueId, node->getDataIndex());
    } else if (attrId == _nameAttrId) {
        lString32 nodeName = node->getNodeName();
        if (nodeName == "a")
            _idNodeMap.set(valueId, node->getDataIndex());
    }
}

lUInt16 lxmlDocBase::getNsNameIndex(const lChar32* name) {
    const LDOMNameIdMapItem* item = _nsNameTable.findItem(name);
    if (item)
        return item->id;
    _nsNameTable.AddItem(_nextUnknownNsId, lString32(name), NULL);
    return _nextUnknownNsId++;
}

lUInt16 lxmlDocBase::getNsNameIndex(const lChar8* name) {
    const LDOMNameIdMapItem* item = _nsNameTable.findItem(name);
    if (item)
        return item->id;
    _nsNameTable.AddItem(_nextUnknownNsId, lString32(name), NULL);
    return _nextUnknownNsId++;
}

lUInt16 lxmlDocBase::getAttrNameIndex(const lChar32* name) {
    const LDOMNameIdMapItem* item = _attrNameTable.findItem(name);
    if (item)
        return item->id;
    _attrNameTable.AddItem(_nextUnknownAttrId, lString32(name), NULL);
    return _nextUnknownAttrId++;
}

lUInt16 lxmlDocBase::getAttrNameIndex(const lChar8* name) {
    const LDOMNameIdMapItem* item = _attrNameTable.findItem(name);
    if (item)
        return item->id;
    _attrNameTable.AddItem(_nextUnknownAttrId, lString32(name), NULL);
    return _nextUnknownAttrId++;
}

lUInt16 lxmlDocBase::getElementNameIndex(const lChar32* name) {
    const LDOMNameIdMapItem* item = _elementNameTable.findItem(name);
    if (item)
        return item->id;
    _elementNameTable.AddItem(_nextUnknownElementId, lString32(name), NULL);
    return _nextUnknownElementId++;
}

lUInt16 lxmlDocBase::findElementNameIndex(const lChar8* name) {
    const LDOMNameIdMapItem* item = _elementNameTable.findItem(name);
    if (item)
        return item->id;
    return 0;
}

lUInt16 lxmlDocBase::getElementNameIndex(const lChar8* name) {
    const LDOMNameIdMapItem* item = _elementNameTable.findItem(name);
    if (item)
        return item->id;
    _elementNameTable.AddItem(_nextUnknownElementId, lString32(name), NULL);
    return _nextUnknownElementId++;
}

/// create formatted text object with options set
LFormattedText* lxmlDocBase::createFormattedText() {
    LFormattedText* p = new LFormattedText();
    p->setImageScalingOptions(&_imgScalingOptions);
    p->setSpaceWidthScalePercent(_spaceWidthScalePercent);
    p->setMinSpaceCondensingPercent(_minSpaceCondensingPercent);
    p->setUnusedSpaceThresholdPercent(_unusedSpaceThresholdPercent);
    p->setMaxAddedLetterSpacingPercent(_maxAddedLetterSpacingPercent);
    p->setHighlightOptions(&_highlightOptions);
    return p;
}

/// returns main element (i.e. FictionBook for FB2)
ldomNode* lxmlDocBase::getRootNode() {
    return getTinyNode(17);
}

lString32 lxmlDocBase::getCodeBase() {
    return getProps()->getStringDef(DOC_PROP_CODE_BASE, "");
}

void lxmlDocBase::setCodeBase(const lString32& codeBase) {
    getProps()->setStringDef(DOC_PROP_CODE_BASE, codeBase);
}

void lxmlDocBase::setNodeTypes(const elem_def_t* node_scheme) {
    if (!node_scheme)
        return;
    for (; node_scheme && node_scheme->id != 0; ++node_scheme) {
        _elementNameTable.AddItem(
                node_scheme->id,              // ID
                lString32(node_scheme->name), // Name
                &node_scheme->props);         // ptr
    }
}

// set attribute types from table
void lxmlDocBase::setAttributeTypes(const attr_def_t* attr_scheme) {
    if (!attr_scheme)
        return;
    for (; attr_scheme && attr_scheme->id != 0; ++attr_scheme) {
        _attrNameTable.AddItem(
                attr_scheme->id,              // ID
                lString32(attr_scheme->name), // Name
                NULL);
    }
    _idAttrId = _attrNameTable.idByName("id");
}

// set namespace types from table
void lxmlDocBase::setNameSpaceTypes(const ns_def_t* ns_scheme) {
    if (!ns_scheme)
        return;
    for (; ns_scheme && ns_scheme->id != 0; ++ns_scheme) {
        _nsNameTable.AddItem(
                ns_scheme->id,              // ID
                lString32(ns_scheme->name), // Name
                NULL);
    }
}

// set node/attribute/namespace types by copying them from an other document
void lxmlDocBase::setAllTypesFrom(lxmlDocBase* d) {
    // Simpler to just serialize and deserialize them all
    SerialBuf buf(0, true);
    d->_elementNameTable.serialize(buf);
    buf << d->_nextUnknownElementId;
    d->_attrNameTable.serialize(buf);
    buf << d->_nextUnknownAttrId;
    d->_nsNameTable.serialize(buf);
    buf << d->_nextUnknownNsId;

    buf.setPos(0);
    _elementNameTable.deserialize(buf);
    buf >> _nextUnknownElementId;
    _attrNameTable.deserialize(buf);
    buf >> _nextUnknownAttrId;
    _nsNameTable.deserialize(buf);
    buf >> _nextUnknownNsId;
}

void lxmlDocBase::dumpUnknownEntities(const char* fname) {
    FILE* f = fopen(fname, "wt");
    if (!f)
        return;
    fprintf(f, "Unknown elements:\n");
    _elementNameTable.dumpUnknownItems(f, UNKNOWN_ELEMENT_TYPE_ID);
    fprintf(f, "-------------------------------\n");
    fprintf(f, "Unknown attributes:\n");
    _attrNameTable.dumpUnknownItems(f, UNKNOWN_ATTRIBUTE_TYPE_ID);
    fprintf(f, "-------------------------------\n");
    fprintf(f, "Unknown namespaces:\n");
    _nsNameTable.dumpUnknownItems(f, UNKNOWN_NAMESPACE_TYPE_ID);
    fprintf(f, "-------------------------------\n");
    fclose(f);
}

lString32Collection lxmlDocBase::getUnknownEntities() {
    lString32Collection unknown_entities;
    unknown_entities.add(_elementNameTable.getUnknownItems(UNKNOWN_ELEMENT_TYPE_ID));
    unknown_entities.add(_attrNameTable.getUnknownItems(UNKNOWN_ATTRIBUTE_TYPE_ID));
    unknown_entities.add(_nsNameTable.getUnknownItems(UNKNOWN_NAMESPACE_TYPE_ID));
    return unknown_entities;
}

static const char* id_map_list_magic = "MAPS";
static const char* elem_id_map_magic = "ELEM";
static const char* attr_id_map_magic = "ATTR";
static const char* attr_value_map_magic = "ATTV";
static const char* ns_id_map_magic = "NMSP";
static const char* node_by_id_map_magic = "NIDM";

typedef struct
{
    lUInt32 key;
    lUInt32 value;
} id_node_map_item;

int compare_id_node_map_items(const void* item1, const void* item2) {
    id_node_map_item* v1 = (id_node_map_item*)item1;
    id_node_map_item* v2 = (id_node_map_item*)item2;
    if (v1->key > v2->key)
        return 1;
    if (v1->key < v2->key)
        return -1;
    return 0;
}

/// serialize to byte array (pointer will be incremented by number of bytes written)
void lxmlDocBase::serializeMaps(SerialBuf& buf) {
    if (buf.error())
        return;
    int pos = buf.pos();
    buf.putMagic(id_map_list_magic);
    buf.putMagic(elem_id_map_magic);
    _elementNameTable.serialize(buf);
    buf << _nextUnknownElementId; // Next Id for unknown element
    buf.putMagic(attr_id_map_magic);
    _attrNameTable.serialize(buf);
    buf << _nextUnknownAttrId; // Next Id for unknown attribute
    buf.putMagic(ns_id_map_magic);
    _nsNameTable.serialize(buf);
    buf << _nextUnknownNsId; // Next Id for unknown namespace
    buf.putMagic(attr_value_map_magic);
    _attrValueTable.serialize(buf);

    int start = buf.pos();
    buf.putMagic(node_by_id_map_magic);
    lUInt32 cnt = 0;
    {
        LVHashTable<lUInt32, lInt32>::iterator ii = _idNodeMap.forwardIterator();
        for (LVHashTable<lUInt32, lInt32>::pair* p = ii.next(); p != NULL; p = ii.next()) {
            cnt++;
        }
    }
    // TODO: investigate why length() doesn't work as count
    if ((int)cnt != _idNodeMap.length())
        CRLog::error("_idNodeMap.length=%d doesn't match real item count %d", _idNodeMap.length(), cnt);
    buf << cnt;
    if (cnt > 0) {
        // sort items before serializing!
        id_node_map_item* array = new id_node_map_item[cnt];
        int i = 0;
        LVHashTable<lUInt32, lInt32>::iterator ii = _idNodeMap.forwardIterator();
        for (LVHashTable<lUInt32, lInt32>::pair* p = ii.next(); p != NULL; p = ii.next()) {
            array[i].key = (lUInt32)p->key;
            array[i].value = (lUInt32)p->value;
            i++;
        }
        qsort(array, cnt, sizeof(id_node_map_item), &compare_id_node_map_items);
        for (i = 0; i < (int)cnt; i++)
            buf << array[i].key << array[i].value;
        delete[] array;
    }
    buf.putMagic(node_by_id_map_magic);
    buf.putCRC(buf.pos() - start);

    buf.putCRC(buf.pos() - pos);
}

/// deserialize from byte array (pointer will be incremented by number of bytes read)
bool lxmlDocBase::deserializeMaps(SerialBuf& buf) {
    if (buf.error())
        return false;
    int pos = buf.pos();
    buf.checkMagic(id_map_list_magic);
    buf.checkMagic(elem_id_map_magic);
    _elementNameTable.deserialize(buf);
    buf >> _nextUnknownElementId; // Next Id for unknown element

    if (buf.error()) {
        CRLog::error("Error while deserialization of Element ID map");
        return false;
    }

    buf.checkMagic(attr_id_map_magic);
    _attrNameTable.deserialize(buf);
    buf >> _nextUnknownAttrId; // Next Id for unknown attribute

    if (buf.error()) {
        CRLog::error("Error while deserialization of Attr ID map");
        return false;
    }

    buf.checkMagic(ns_id_map_magic);
    _nsNameTable.deserialize(buf);
    buf >> _nextUnknownNsId; // Next Id for unknown namespace

    if (buf.error()) {
        CRLog::error("Error while deserialization of NS ID map");
        return false;
    }

    buf.checkMagic(attr_value_map_magic);
    _attrValueTable.deserialize(buf);

    if (buf.error()) {
        CRLog::error("Error while deserialization of AttrValue map");
        return false;
    }

    int start = buf.pos();
    buf.checkMagic(node_by_id_map_magic);
    lUInt32 idmsize;
    buf >> idmsize;
    _idNodeMap.clear();
    if (idmsize < 20000)
        _idNodeMap.resize(idmsize * 2);
    for (unsigned i = 0; i < idmsize; i++) {
        lUInt32 key;
        lUInt32 value;
        buf >> key;
        buf >> value;
        _idNodeMap.set(key, value);
        if (buf.error())
            return false;
    }
    buf.checkMagic(node_by_id_map_magic);

    if (buf.error()) {
        CRLog::error("Error while deserialization of ID->Node map");
        return false;
    }

    buf.checkCRC(buf.pos() - start);

    if (buf.error()) {
        CRLog::error("Error while deserialization of ID->Node map - CRC check failed");
        return false;
    }

    buf.checkCRC(buf.pos() - pos);

    return !buf.error();
}

static const char* doc_file_magic = "CR3\n";

bool lxmlDocBase::DocFileHeader::serialize(SerialBuf& hdrbuf) {
    int start = hdrbuf.pos();
    hdrbuf.putMagic(doc_file_magic);
    //CRLog::trace("Serializing render data: %d %d %d %d", render_dx, render_dy, render_docflags, render_style_hash);
    hdrbuf << render_dx << render_dy << render_docflags << render_style_hash << stylesheet_hash << node_displaystyle_hash;

    hdrbuf.putCRC(hdrbuf.pos() - start);

#if 0
    {
        lString8 s;
        s<<"SERIALIZED HDR BUF: ";
        for ( int i=0; i<hdrbuf.pos(); i++ ) {
            char tmp[20];
            sprintf(tmp, "%02x ", hdrbuf.buf()[i]);
            s<<tmp;
        }
        CRLog::trace(s.c_str());
    }
#endif
    return !hdrbuf.error();
}

bool lxmlDocBase::DocFileHeader::deserialize(SerialBuf& hdrbuf) {
    int start = hdrbuf.pos();
    hdrbuf.checkMagic(doc_file_magic);
    if (hdrbuf.error()) {
        CRLog::error("Swap file Magic signature doesn't match");
        return false;
    }
    hdrbuf >> render_dx >> render_dy >> render_docflags >> render_style_hash >> stylesheet_hash >> node_displaystyle_hash;
    //CRLog::trace("Deserialized render data: %d %d %d %d", render_dx, render_dy, render_docflags, render_style_hash);
    hdrbuf.checkCRC(hdrbuf.pos() - start);
    if (hdrbuf.error()) {
        CRLog::error("Swap file - header unpack error");
        return false;
    }
    return true;
}

void lxmlDocBase::setStyleSheet(const char* css, bool replace) {
    lString8 s(css);

    //CRLog::trace("lxmlDocBase::setStyleSheet(length:%d replace:%s css text hash: %x)", strlen(css), replace ? "yes" : "no", s.getHash());
    lUInt32 oldHash = _stylesheet.getHash();
    if (replace) {
        //CRLog::debug("cleaning stylesheet contents");
        _stylesheet.clear();
    }
    if (css && *css) {
        //CRLog::debug("appending stylesheet contents: \n%s", css);
        _stylesheet.parse(css, true);
        // We use override_important=true: we are the only code
        // that sets the main CSS (including style tweaks). We allow
        // any !important to override any previous !important.
        // Other calls to _stylesheet.parse() elsewhere are used to
        // include document embedded or inline CSS, with the default
        // of override_important=false, so they won't override
        // the ones we set here.
    }
    lUInt32 newHash = _stylesheet.getHash();
    if (oldHash != newHash) {
        CRLog::debug("New stylesheet hash: %08x", newHash);
    }
}
