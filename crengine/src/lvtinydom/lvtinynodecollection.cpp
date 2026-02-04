/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2010-2012,2015 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2016 Yifei(Frank) ZHU <fredyifei@gmail.com>             *
 *   Copyright (C) 2020 Konstantin Potapov <pkbo@users.sourceforge.net>    *
 *   Copyright (C) 2021 ourairquality <info@ourairquality.org>             *
 *   Copyright (C) 2018-2021 poire-z <poire-z@users.noreply.github.com>    *
 *   Copyright (C) 2018-2023 Aleksey Chernov <valexlin@gmail.com>          *
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

#include <lvtinynodecollection.h>
#include <ldomdoccache.h>
#include <lvdocprops.h>
#include <lvrend.h>
#include <crlog.h>

#include "lvtinydom_private.h"
#include "ldomblobcache.h"
#include "tinyelement.h"
#include "cachefile.h"
#include "ldomdatastoragemanager.h"

#include "../textlang.h"

#define STYLE_HASH_TABLE_SIZE 512
#define FONT_HASH_TABLE_SIZE  256

//--------------------------------------------------------
// cache memory sizes
//--------------------------------------------------------
#define TEXT_CACHE_UNPACKED_SPACE  (25 * DOC_BUFFER_SIZE / 100)
#define TEXT_CACHE_CHUNK_SIZE      0x008000 // 32K
#define ELEM_CACHE_UNPACKED_SPACE  (45 * DOC_BUFFER_SIZE / 100)
#define ELEM_CACHE_CHUNK_SIZE      0x004000 // 16K
#define RECT_CACHE_UNPACKED_SPACE  (45 * DOC_BUFFER_SIZE / 100)
#define RECT_CACHE_CHUNK_SIZE      0x00F000 // 64K
#define STYLE_CACHE_UNPACKED_SPACE (10 * DOC_BUFFER_SIZE / 100)
#define STYLE_CACHE_CHUNK_SIZE     0x00C000 // 48K

// default is to compress to use smaller cache files (but slower rendering
// and page turns with big documents)
static CacheCompressionType _cacheCompressionType =
#if (USE_ZSTD == 1)
        CacheCompressionZSTD;
#elif (USE_ZLIB == 1)
        CacheCompressionZlib;
#else
        CacheCompressionNone;
#endif
void setCacheCompressionType(CacheCompressionType type) {
    _cacheCompressionType = type;
}

static const char* styles_magic = "CRSTYLES";

lUInt32 calcGlobalSettingsHash(int documentId, bool already_rendered);

img_scaling_option_t::img_scaling_option_t() {
    mode = (MAX_IMAGE_SCALE_MUL > 1) ? (ARBITRARY_IMAGE_SCALE_ENABLED == 1 ? IMG_FREE_SCALING : IMG_INTEGER_SCALING) : IMG_NO_SCALE;
    max_scale = (MAX_IMAGE_SCALE_MUL > 1) ? MAX_IMAGE_SCALE_MUL : 1;
}

img_scaling_options_t::img_scaling_options_t() {
    img_scaling_option_t option;
    zoom_in_inline = option;
    zoom_in_block = option;
    zoom_out_inline = option;
    zoom_out_block = option;
}

#define FONT_SIZE_BIG      32
#define FONT_SIZE_VERY_BIG 50
static bool updateScalingOption(img_scaling_option_t& v, CRPropRef props, int fontSize, bool zoomin, bool isInline) {
    lString8 propName("crengine.image.scaling.");
    propName << (zoomin ? "zoomin." : "zoomout.");
    propName << (isInline ? "inline." : "block.");
    lString8 propNameMode = propName + "mode";
    lString8 propNameScale = propName + "scale";
    img_scaling_option_t def;
    int currMode = props->getIntDef(propNameMode.c_str(), (int)def.mode);
    int currScale = props->getIntDef(propNameScale.c_str(), (int)def.max_scale);
    if (currScale == 0) {
        if (fontSize >= FONT_SIZE_VERY_BIG)
            currScale = 3;
        else if (fontSize >= FONT_SIZE_BIG)
            currScale = 2;
        else
            currScale = 1;
    }
    if (currScale == 1)
        currMode = 0;
    bool updated = false;
    if (v.max_scale != currScale) {
        updated = true;
        v.max_scale = currScale;
    }
    if (v.mode != (img_scaling_mode_t)currMode) {
        updated = true;
        v.mode = (img_scaling_mode_t)currMode;
    }
    props->setIntDef(propNameMode.c_str(), currMode);
    props->setIntDef(propNameScale.c_str(), currScale);
    return updated;
}

/// returns true if any changes occured
bool img_scaling_options_t::update(CRPropRef props, int fontSize) {
    bool updated = false;
    updated = updateScalingOption(zoom_in_inline, props, fontSize, true, true) || updated;
    updated = updateScalingOption(zoom_in_block, props, fontSize, true, false) || updated;
    updated = updateScalingOption(zoom_out_inline, props, fontSize, false, true) || updated;
    updated = updateScalingOption(zoom_out_block, props, fontSize, false, false) || updated;
    return updated;
}

//=================================================================
// tinyNodeCollection implementation
//=================================================================

/// minimize memory consumption
void tinyNodeCollection::compact() {
    _textStorage->compact(0xFFFFFF);
    _elemStorage->compact(0xFFFFFF);
    _rectStorage->compact(0xFFFFFF);
    _styleStorage->compact(0xFFFFFF);
}

/// allocate new tinyElement
ldomNode* tinyNodeCollection::allocTinyElement(ldomNode* parent, lUInt16 nsid, lUInt16 id) {
    ldomNode* node = allocTinyNode(ldomNode::NT_ELEMENT);
    tinyElement* elem = new tinyElement((ldomDocument*)this, parent, nsid, id);
    node->_data._elem_ptr = elem;
    return node;
}

/// dumps memory usage statistics to debug log
void tinyNodeCollection::dumpStatistics() {
    CRLog::info("*** Document memory usage: "
                "elements:%d, textNodes:%d, "
                "ptext=("
                "%d uncompressed), "
                "ptelems=("
                "%d uncompressed), "
                "rects=("
                "%d uncompressed), "
                "nodestyles=("
                "%d uncompressed), "
                "styles:%d, fonts:%d, renderedNodes:%d, "
                "totalNodes:%d(%dKb), mutableElements:%d(~%dKb)",
                _elemCount, _textCount,
                _textStorage->getUncompressedSize(),
                _elemStorage->getUncompressedSize(),
                _rectStorage->getUncompressedSize(),
                _styleStorage->getUncompressedSize(),
                _styles.length(), _fonts.length(),
                ((ldomDocument*)this)->_renderedBlockCache.length(),
                _itemCount, _itemCount * 16 / 1024,
                _tinyElementCount, _tinyElementCount * (sizeof(tinyElement) + 8 * 4) / 1024);
}
lString32 tinyNodeCollection::getStatistics() {
    lString32 s;
    s << "Elements: " << fmt::decimal(_elemCount) << ", " << fmt::decimal(_elemStorage->getUncompressedSize() / 1024) << " KB\n";
    s << "Text nodes: " << fmt::decimal(_textCount) << ", " << fmt::decimal(_textStorage->getUncompressedSize() / 1024) << " KB\n";
    s << "Styles: " << fmt::decimal(_styles.length()) << ", " << fmt::decimal(_styleStorage->getUncompressedSize() / 1024) << " KB\n";
    s << "Font instances: " << fmt::decimal(_fonts.length()) << "\n";
    s << "Rects: " << fmt::decimal(_rectStorage->getUncompressedSize() / 1024) << " KB\n";
    s << "Cached rendered blocks: " << fmt::decimal(((ldomDocument*)this)->_renderedBlockCache.length()) << "\n";
    s << "Total nodes: " << fmt::decimal(_itemCount) << ", " << fmt::decimal(_itemCount * 16 / 1024) << " KB\n";
    s << "Mutable elements: " << fmt::decimal(_tinyElementCount) << ", " << fmt::decimal(_tinyElementCount * (sizeof(tinyElement) + 8 * 4) / 1024) << " KB";
    return s;
}

tinyNodeCollection::tinyNodeCollection()
        : _textCount(0)
        , _textNextFree(0)
        , _elemCount(0)
        , _elemNextFree(0)
        , _styles(STYLE_HASH_TABLE_SIZE)
        , _fonts(FONT_HASH_TABLE_SIZE)
        , _tinyElementCount(0)
        , _itemCount(0)
        , _renderedBlockCache(256)
        , _cacheFile(NULL)
        , _cacheFileStale(true)
        , _cacheFileLeaveAsDirty(false)
        , _mapped(false)
        , _maperror(false)
        , _mapSavingStage(0)
        , _imgAutoRotate(false)
        , _docRotation(0)
        , _spaceWidthScalePercent(DEF_SPACE_WIDTH_SCALE_PERCENT)
        , _minSpaceCondensingPercent(DEF_MIN_SPACE_CONDENSING_PERCENT)
        , _unusedSpaceThresholdPercent(DEF_UNUSED_SPACE_THRESHOLD_PERCENT)
        , _maxAddedLetterSpacingPercent(DEF_MAX_ADDED_LETTER_SPACING_PERCENT)
        , _nodeStyleHash(0)
        , _nodeDisplayStyleHash(NODE_DISPLAY_STYLE_HASH_UNINITIALIZED)
        , _nodeDisplayStyleHashInitial(NODE_DISPLAY_STYLE_HASH_UNINITIALIZED)
        , _nodeStylesInvalidIfLoading(false)
        , _docProps(LVCreatePropsContainer())
        , _docFlags(DOC_FLAG_DEFAULTS)
        , _fontMap(113)
        , _hangingPunctuationEnabled(false)
        , _renderBlockRenderingFlags(BLOCK_RENDERING_FLAGS_DEFAULT)
        , _DOMVersionRequested(DOM_VERSION_CURRENT)
        , _interlineScaleFactor(INTERLINE_SCALE_FACTOR_NO_SCALE) {
    _textStorage = new ldomDataStorageManager(this, 't', TEXT_CACHE_UNPACKED_SPACE, TEXT_CACHE_CHUNK_SIZE);    // persistent text node data storage
    _elemStorage = new ldomDataStorageManager(this, 'e', ELEM_CACHE_UNPACKED_SPACE, ELEM_CACHE_CHUNK_SIZE);    // persistent element data storage
    _rectStorage = new ldomDataStorageManager(this, 'r', RECT_CACHE_UNPACKED_SPACE, RECT_CACHE_CHUNK_SIZE);    // element render rect storage
    _styleStorage = new ldomDataStorageManager(this, 's', STYLE_CACHE_UNPACKED_SPACE, STYLE_CACHE_CHUNK_SIZE); // element style info storage
    memset(_textList, 0, sizeof(_textList));
    memset(_elemList, 0, sizeof(_elemList));
    _blobCache = new ldomBlobCache;
    // _docIndex assigned in ldomDocument constructor
}

tinyNodeCollection::tinyNodeCollection(tinyNodeCollection& v)
        : _textCount(0)
        , _textNextFree(0)
        , _elemCount(0)
        , _elemNextFree(0)
        , _styles(STYLE_HASH_TABLE_SIZE)
        , _fonts(FONT_HASH_TABLE_SIZE)
        , _tinyElementCount(0)
        , _itemCount(0)
        , _renderedBlockCache(256)
        , _cacheFile(NULL)
        , _cacheFileStale(true)
        , _cacheFileLeaveAsDirty(false)
        , _mapped(false)
        , _maperror(false)
        , _mapSavingStage(0)
        , _imgAutoRotate(false)
        , _docRotation(0)
        , _spaceWidthScalePercent(DEF_SPACE_WIDTH_SCALE_PERCENT)
        , _minSpaceCondensingPercent(DEF_MIN_SPACE_CONDENSING_PERCENT)
        , _unusedSpaceThresholdPercent(DEF_UNUSED_SPACE_THRESHOLD_PERCENT)
        , _maxAddedLetterSpacingPercent(DEF_MAX_ADDED_LETTER_SPACING_PERCENT)
        , _nodeStyleHash(0)
        , _nodeDisplayStyleHash(NODE_DISPLAY_STYLE_HASH_UNINITIALIZED)
        , _nodeDisplayStyleHashInitial(NODE_DISPLAY_STYLE_HASH_UNINITIALIZED)
        , _nodeStylesInvalidIfLoading(false)
        , _docProps(LVCreatePropsContainer())
        , _docFlags(v._docFlags)
        , _stylesheet(v._stylesheet)
        , _fontMap(113)
        , _hangingPunctuationEnabled(v._hangingPunctuationEnabled)
        , _renderBlockRenderingFlags(v._renderBlockRenderingFlags)
        , _DOMVersionRequested(v._DOMVersionRequested)
        , _interlineScaleFactor(v._interlineScaleFactor) {
    _textStorage = new ldomDataStorageManager(this, 't', TEXT_CACHE_UNPACKED_SPACE, TEXT_CACHE_CHUNK_SIZE);    // persistent text node data storage
    _elemStorage = new ldomDataStorageManager(this, 'e', ELEM_CACHE_UNPACKED_SPACE, ELEM_CACHE_CHUNK_SIZE);    // persistent element data storage
    _rectStorage = new ldomDataStorageManager(this, 'r', RECT_CACHE_UNPACKED_SPACE, RECT_CACHE_CHUNK_SIZE);    // element render rect storage
    _styleStorage = new ldomDataStorageManager(this, 's', STYLE_CACHE_UNPACKED_SPACE, STYLE_CACHE_CHUNK_SIZE); // element style info storage
    memset(_textList, 0, sizeof(_textList));
    memset(_elemList, 0, sizeof(_elemList));
    _blobCache = new ldomBlobCache;
    // _docIndex assigned in ldomDocument constructor
}

bool tinyNodeCollection::addBlob(lString32 name, const lUInt8* data, int size) {
    _cacheFileStale = true;
    return _blobCache->addBlob(data, size, name);
}

LVStreamRef tinyNodeCollection::getBlob(lString32 name) {
    return _blobCache->getBlob(name);
}

bool tinyNodeCollection::setHangingPunctiationEnabled(bool value) {
    if (_hangingPunctuationEnabled != value) {
        _hangingPunctuationEnabled = value;
        return true;
    }
    return false;
}

bool tinyNodeCollection::setRenderBlockRenderingFlags(lUInt32 flags) {
    if (_renderBlockRenderingFlags != flags) {
        _renderBlockRenderingFlags = flags;
        // Check coherency and ensure dependencies of flags
        if (_renderBlockRenderingFlags & ~BLOCK_RENDERING_ENHANCED) // If any other flag is set,
            _renderBlockRenderingFlags |= BLOCK_RENDERING_ENHANCED; // set ENHANGED
        if (_renderBlockRenderingFlags & BLOCK_RENDERING_FLOAT_FLOATBOXES)
            _renderBlockRenderingFlags |= BLOCK_RENDERING_PREPARE_FLOATBOXES;
        if (_renderBlockRenderingFlags & BLOCK_RENDERING_PREPARE_FLOATBOXES)
            _renderBlockRenderingFlags |= BLOCK_RENDERING_WRAP_FLOATS;
        return true;
    }
    return false;
}

bool tinyNodeCollection::setDOMVersionRequested(lUInt32 version) {
    if (_DOMVersionRequested != version) {
        _DOMVersionRequested = version;
        return true;
    }
    return false;
}

bool tinyNodeCollection::setInterlineScaleFactor(int value) {
    if (_interlineScaleFactor != value) {
        _interlineScaleFactor = value;
        return true;
    }
    return false;
}

bool tinyNodeCollection::openCacheFile() {
    if (_cacheFile)
        return true;
    CacheFile* f = new CacheFile(_DOMVersionRequested, _cacheCompressionType);
    //lString32 cacheFileName("/tmp/cr3swap.tmp");

    lString32 fname = getProps()->getStringDef(DOC_PROP_FILE_NAME, "noname");
    //lUInt32 sz = (lUInt32)getProps()->getInt64Def(DOC_PROP_FILE_SIZE, 0);
    lUInt32 crc = (lUInt32)getProps()->getIntDef(DOC_PROP_FILE_CRC32, 0);

    if (!ldomDocCache::enabled()) {
        CRLog::error("Cannot open cached document: cache dir is not initialized");
        delete f;
        return false;
    }

    CRLog::info("ldomDocument::openCacheFile() - looking for cache file %s", UnicodeToUtf8(fname).c_str());

    lString32 cache_path;
    LVStreamRef map = ldomDocCache::openExisting(fname, crc, getPersistenceFlags(), cache_path);
    if (map.isNull()) {
        delete f;
        return false;
    }
    CRLog::info("ldomDocument::openCacheFile() - cache file found, trying to read index %s", UnicodeToUtf8(fname).c_str());

    if (!f->open(map)) {
        delete f;
        return false;
    }
    CRLog::info("ldomDocument::openCacheFile() - index read successfully %s", UnicodeToUtf8(fname).c_str());
    f->setCachePath(cache_path);
    _cacheFile = f;
    _textStorage->setCache(f);
    _elemStorage->setCache(f);
    _rectStorage->setCache(f);
    _styleStorage->setCache(f);
    _blobCache->setCacheFile(f);
    return true;
}

bool tinyNodeCollection::swapToCacheIfNecessary() {
    if (!_cacheFile || _mapped || _maperror)
        return false;
    return createCacheFile();
    //return swapToCache();
}

bool tinyNodeCollection::createCacheFile() {
    if (_cacheFile)
        return true;
    CacheFile* f = new CacheFile(_DOMVersionRequested, _cacheCompressionType);
    //lString32 cacheFileName("/tmp/cr3swap.tmp");

    lString32 fname = getProps()->getStringDef(DOC_PROP_FILE_NAME, "noname");
    lUInt64 sz = (lUInt64)getProps()->getInt64Def(DOC_PROP_FILE_SIZE, 0);
    lUInt32 crc = (lUInt32)getProps()->getIntDef(DOC_PROP_FILE_CRC32, 0);

    if (!ldomDocCache::enabled()) {
        CRLog::error("Cannot swap: cache dir is not initialized");
        delete f;
        return false;
    }

    CRLog::info("ldomDocument::createCacheFile() - initialized swapping of document %s to cache file", UnicodeToUtf8(fname).c_str());

    lString32 cache_path;
    // TODO: ldomDocCache::createNew(): change the "fileSize" argument type to lUInt64
    LVStreamRef map = ldomDocCache::createNew(fname, crc, getPersistenceFlags(), (lUInt32)sz, cache_path);
    if (map.isNull()) {
        CRLog::error("Cannot swap: failed to allocate cache map");
        delete f;
        return false;
    }

    if (!f->create(map)) {
        CRLog::error("Cannot swap: failed to create map file");
        delete f;
        return false;
    }
    f->setCachePath(cache_path);
    _cacheFile = f;
    _mapped = true;
    _textStorage->setCache(f);
    _elemStorage->setCache(f);
    _rectStorage->setCache(f);
    _styleStorage->setCache(f);
    _blobCache->setCacheFile(f);
    setCacheFileStale(true);
    return true;
}

lString32 tinyNodeCollection::getCacheFilePath() {
    return _cacheFile != NULL ? _cacheFile->getCachePath() : lString32::empty_str;
}

void tinyNodeCollection::clearNodeStyle(lUInt32 dataIndex) {
    ldomNodeStyleInfo info;
    _styleStorage->getStyleData(dataIndex, &info);
    _styles.release(info._styleIndex);
    _fonts.release(info._fontIndex);
    info._fontIndex = info._styleIndex = 0;
    _styleStorage->setStyleData(dataIndex, &info);
    _nodeStyleHash = 0;
}

void tinyNodeCollection::setNodeStyleIndex(lUInt32 dataIndex, lUInt16 index) {
    ldomNodeStyleInfo info;
    _styleStorage->getStyleData(dataIndex, &info);
    if (info._styleIndex != index) {
        info._styleIndex = index;
        _styleStorage->setStyleData(dataIndex, &info);
        _nodeStyleHash = 0;
    }
}

void tinyNodeCollection::setNodeFontIndex(lUInt32 dataIndex, lUInt16 index) {
    ldomNodeStyleInfo info;
    _styleStorage->getStyleData(dataIndex, &info);
    if (info._fontIndex != index) {
        info._fontIndex = index;
        _styleStorage->setStyleData(dataIndex, &info);
        _nodeStyleHash = 0;
    }
}

lUInt16 tinyNodeCollection::getNodeStyleIndex(lUInt32 dataIndex) {
    ldomNodeStyleInfo info;
    _styleStorage->getStyleData(dataIndex, &info);
    return info._styleIndex;
}

css_style_ref_t tinyNodeCollection::getNodeStyle(lUInt32 dataIndex) {
    ldomNodeStyleInfo info;
    _styleStorage->getStyleData(dataIndex, &info);
    css_style_ref_t res = _styles.get(info._styleIndex);
    if (!res.isNull())
        _styles.addIndexRef(info._styleIndex);
#if DEBUG_DOM_STORAGE == 1
    if (res.isNull() && info._styleIndex != 0) {
        CRLog::error("Null style returned for index %d", (int)info._styleIndex);
    }
#endif
    return res;
}

font_ref_t tinyNodeCollection::getNodeFont(lUInt32 dataIndex) {
    ldomNodeStyleInfo info;
    _styleStorage->getStyleData(dataIndex, &info);
    return _fonts.get(info._fontIndex);
}

void tinyNodeCollection::setNodeStyle(lUInt32 dataIndex, css_style_ref_t& v) {
    ldomNodeStyleInfo info;
    _styleStorage->getStyleData(dataIndex, &info);
    _styles.cache(info._styleIndex, v);
#if DEBUG_DOM_STORAGE == 1
    if (info._styleIndex == 0) {
        CRLog::error("tinyNodeCollection::setNodeStyle() styleIndex is 0 after caching");
    }
#endif
    _styleStorage->setStyleData(dataIndex, &info);
    _nodeStyleHash = 0;
}

void tinyNodeCollection::setNodeFont(lUInt32 dataIndex, font_ref_t& v) {
    ldomNodeStyleInfo info;
    _styleStorage->getStyleData(dataIndex, &info);
    _fonts.cache(info._fontIndex, v);
    _styleStorage->setStyleData(dataIndex, &info);
    _nodeStyleHash = 0;
}

lUInt16 tinyNodeCollection::getNodeFontIndex(lUInt32 dataIndex) {
    ldomNodeStyleInfo info;
    _styleStorage->getStyleData(dataIndex, &info);
    return info._fontIndex;
}

bool tinyNodeCollection::loadNodeData(lUInt16 type, ldomNode** list, int nodecount) {
    int count = ((nodecount + TNC_PART_LEN - 1) >> TNC_PART_SHIFT);
    for (lUInt16 i = 0; i < count; i++) {
        int offs = i * TNC_PART_LEN;
        int sz = TNC_PART_LEN;
        if (offs + sz > nodecount) {
            sz = nodecount - offs;
        }

        lUInt8* p;
        int buflen;
        if (!_cacheFile->read(type, i, p, buflen))
            return false;
        if (!p || (unsigned)buflen != sizeof(ldomNode) * sz)
            return false;
        ldomNode* buf = (ldomNode*)p;
        if (sz == TNC_PART_LEN)
            list[i] = buf;
        else {
            // buf contains `sz' ldomNode items
            // _elemList, _textList (as `list' argument) must always be TNC_PART_LEN size
            // add into `list' zero filled (TNC_PART_LEN - sz) items
            list[i] = (ldomNode*)realloc(buf, TNC_PART_LEN * sizeof(ldomNode));
            if (NULL == list[i]) {
                free(buf);
                CRLog::error("Not enough memory!");
                return false;
            }
            memset(list[i] + sz, 0, (TNC_PART_LEN - sz) * sizeof(ldomNode));
        }
        for (int j = 0; j < sz; j++) {
            list[i][j].setDocumentIndex(_docIndex);
            // validate loaded nodes: all non-null nodes should be marked as persistent, i.e. the actual node data: _data._pelem_addr, _data._ptext_addr,
            // NOT _data._elem_ptr, _data._text_ptr.
            // So we check this flag, but after setting document so that isNull() works correctly.
            // If the node is not persistent now, then _data._elem_ptr will be used, which then generate SEGFAULT.
            if (!list[i][j].isNull() && !list[i][j].isPersistent()) {
                CRLog::error("Invalid cached node, flag PERSISTENT are NOT set: segment=%d, index=%d", i, j);
                // list[i] will be freed in the caller method.
                return false;
            }
            if (list[i][j].isElement()) {
                // will be set by loadStyles/updateStyles
                //list[i][j]._data._pelem._styleIndex = 0;
                setNodeFontIndex(list[i][j]._handle._dataIndex, 0);
                //list[i][j]._data._pelem._fontIndex = 0;
            }
        }
    }
    return true;
}

bool tinyNodeCollection::hasRenderData() {
    return _rectStorage->hasChunks();
}

bool tinyNodeCollection::saveNodeData(lUInt16 type, ldomNode** list, int nodecount) {
    int count = ((nodecount + TNC_PART_LEN - 1) >> TNC_PART_SHIFT);
    for (lUInt16 i = 0; i < count; i++) {
        if (!list[i])
            continue;
        int offs = i * TNC_PART_LEN;
        int sz = TNC_PART_LEN;
        if (offs + sz > nodecount) {
            sz = nodecount - offs;
        }
        ldomNode buf[TNC_PART_LEN];
        memcpy(buf, list[i], sizeof(ldomNode) * sz);
        for (int j = 0; j < sz; j++) {
            buf[j].setDocumentIndex(_docIndex);
            // On 64bits builds, this serialized ldomNode may have some
            // random data at the end, for being:
            //   union { [...] tinyElement * _elem_ptr; [...] lUInt32 _ptext_addr; [...] lUInt32 _nextFreeIndex }
            // To get "reproducible" cache files with a same file checksum, we'd
            // rather have the remains of the _elem_ptr sets to 0
            if (sizeof(int*) == 8) {                       // 64bits
                lUInt32 tmp = buf[j]._data._nextFreeIndex; // save 32bits part
                buf[j]._data._elem_ptr = 0;                // clear 64bits area
                buf[j]._data._nextFreeIndex = tmp;         // restore 32bits part
            }
        }
        if (!_cacheFile->write(type, i, (lUInt8*)buf, sizeof(ldomNode) * sz, COMPRESS_NODE_DATA))
            crFatalError(-1, "Cannot write node data");
    }
    return true;
}

#define NODE_INDEX_MAGIC 0x19283746
bool tinyNodeCollection::saveNodeData() {
    SerialBuf buf(12, true);
    buf << (lUInt32)NODE_INDEX_MAGIC << (lUInt32)_elemCount << (lUInt32)_textCount;
    if (!saveNodeData(CBT_ELEM_NODE, _elemList, _elemCount + 1))
        return false;
    if (!saveNodeData(CBT_TEXT_NODE, _textList, _textCount + 1))
        return false;
    if (!_cacheFile->write(CBT_NODE_INDEX, buf, COMPRESS_NODE_DATA))
        return false;
    return true;
}

bool tinyNodeCollection::loadNodeData() {
    SerialBuf buf(0, true);
    if (!_cacheFile->read((lUInt16)CBT_NODE_INDEX, buf))
        return false;
    lUInt32 magic;
    lInt32 elemcount;
    lInt32 textcount;
    buf >> magic >> elemcount >> textcount;
    if (magic != NODE_INDEX_MAGIC) {
        return false;
    }
    if (elemcount <= 0)
        return false;
    if (textcount <= 0)
        return false;
    ldomNode* elemList[TNC_PART_COUNT] = { 0 };
    ldomNode* textList[TNC_PART_COUNT] = { 0 };
    if (!loadNodeData(CBT_ELEM_NODE, elemList, elemcount + 1)) {
        for (int i = 0; i < TNC_PART_COUNT; i++)
            if (elemList[i])
                free(elemList[i]);
        return false;
    }
    if (!loadNodeData(CBT_TEXT_NODE, textList, textcount + 1)) {
        for (int i = 0; i < TNC_PART_COUNT; i++)
            if (textList[i])
                free(textList[i]);
        // Also clean elemList previously successfully loaded, to avoid mem leak
        for (int i = 0; i < TNC_PART_COUNT; i++)
            if (elemList[i])
                free(elemList[i]);
        return false;
    }
    for (int i = 0; i < TNC_PART_COUNT; i++) {
        if (_elemList[i])
            free(_elemList[i]);
        if (_textList[i])
            free(_textList[i]);
    }
    memcpy(_elemList, elemList, sizeof(elemList));
    memcpy(_textList, textList, sizeof(textList));
    _elemCount = elemcount;
    _textCount = textcount;
    return true;
}

/// get ldomNode instance pointer
ldomNode* tinyNodeCollection::getTinyNode(lUInt32 index) const {
    if (!index)
        return NULL;
    if (index & 1) // element
        return &(_elemList[index >> TNC_PART_INDEX_SHIFT][(index >> 4) & TNC_PART_MASK]);
    else // text
        return &(_textList[index >> TNC_PART_INDEX_SHIFT][(index >> 4) & TNC_PART_MASK]);
}

/// allocate new tiny node
ldomNode* tinyNodeCollection::allocTinyNode(int type) {
    ldomNode* res;
    if (type & 1) {
        // allocate Element
        if (_elemNextFree) {
            // reuse existing free item
            int index = (_elemNextFree << 4) | type;
            res = getTinyNode(index);
            res->_handle._dataIndex = index;
            _elemNextFree = res->_data._nextFreeIndex;
        } else {
            // create new item
            _elemCount++;
            int idx = _elemCount >> TNC_PART_SHIFT;
            if (idx >= TNC_PART_COUNT)
                crFatalError(1003, "allocTinyNode: can't create any more element nodes (hard limit)");
            ldomNode* part = _elemList[idx];
            if (!part) {
                part = (ldomNode*)calloc(TNC_PART_LEN, sizeof(*part));
                _elemList[idx] = part;
            }
            res = &part[_elemCount & TNC_PART_MASK];
            res->setDocumentIndex(_docIndex);
            res->_handle._dataIndex = (_elemCount << 4) | type;
        }
        _itemCount++;
    } else {
        // allocate Text
        if (_textNextFree) {
            // reuse existing free item
            int index = (_textNextFree << 4) | type;
            res = getTinyNode(index);
            res->_handle._dataIndex = index;
            _textNextFree = res->_data._nextFreeIndex;
        } else {
            // create new item
            _textCount++;
            if (_textCount >= (TNC_PART_COUNT << TNC_PART_SHIFT))
                crFatalError(1003, "allocTinyNode: can't create any more text nodes (hard limit)");
            ldomNode* part = _textList[_textCount >> TNC_PART_SHIFT];
            if (!part) {
                part = (ldomNode*)calloc(TNC_PART_LEN, sizeof(*part));
                _textList[_textCount >> TNC_PART_SHIFT] = part;
            }
            res = &part[_textCount & TNC_PART_MASK];
            res->setDocumentIndex(_docIndex);
            res->_handle._dataIndex = (_textCount << 4) | type;
        }
        _itemCount++;
    }
    _nodeStyleHash = 0;
    return res;
}

void tinyNodeCollection::recycleTinyNode(lUInt32 index) {
    if (index & 1) {
        // element
        index >>= 4;
        ldomNode* part = _elemList[index >> TNC_PART_SHIFT];
        ldomNode* p = &part[index & TNC_PART_MASK];
        p->_handle._dataIndex = 0; // indicates NULL node
        p->_data._nextFreeIndex = _elemNextFree;
        _elemNextFree = index;
        _itemCount--;
    } else {
        // text
        index >>= 4;
        ldomNode* part = _textList[index >> TNC_PART_SHIFT];
        ldomNode* p = &part[index & TNC_PART_MASK];
        p->_handle._dataIndex = 0; // indicates NULL node
        p->_data._nextFreeIndex = _textNextFree;
        _textNextFree = index;
        _itemCount--;
    }
    _nodeStyleHash = 0;
}

tinyNodeCollection::~tinyNodeCollection() {
    if (_cacheFile)
        delete _cacheFile;
    // clear all elem parts
    for (int partindex = 0; partindex <= (_elemCount >> TNC_PART_SHIFT); partindex++) {
        ldomNode* part = _elemList[partindex];
        if (part) {
            int n0 = TNC_PART_LEN * partindex;
            for (int i = 0; i < TNC_PART_LEN && n0 + i <= _elemCount; i++)
                part[i].onCollectionDestroy();
            free(part);
            _elemList[partindex] = NULL;
        }
    }
    // clear all text parts
    for (int partindex = 0; partindex <= (_textCount >> TNC_PART_SHIFT); partindex++) {
        ldomNode* part = _textList[partindex];
        if (part) {
            int n0 = TNC_PART_LEN * partindex;
            for (int i = 0; i < TNC_PART_LEN && n0 + i <= _textCount; i++)
                part[i].onCollectionDestroy();
            free(part);
            _textList[partindex] = NULL;
        }
    }
    delete _blobCache;
    delete _textStorage;
    delete _elemStorage;
    delete _rectStorage;
    delete _styleStorage;
    // document unregistered in ldomDocument destructor
}

/// put all objects into persistent storage
void tinyNodeCollection::persist(CRTimerUtil& maxTime) {
    CRLog::info("lxmlDocBase::persist() invoked - converting all nodes to persistent objects");
    // elements
    for (int partindex = 0; partindex <= (_elemCount >> TNC_PART_SHIFT); partindex++) {
        ldomNode* part = _elemList[partindex];
        if (part) {
            int n0 = TNC_PART_LEN * partindex;
            for (int i = 0; i < TNC_PART_LEN && n0 + i <= _elemCount; i++)
                if (!part[i].isNull() && !part[i].isPersistent()) {
                    part[i].persist();
                    if (maxTime.expired())
                        return;
                }
        }
    }
    //_cacheFile->flush(false); // intermediate flush
    if (maxTime.expired())
        return;
    // texts
    for (int partindex = 0; partindex <= (_textCount >> TNC_PART_SHIFT); partindex++) {
        ldomNode* part = _textList[partindex];
        if (part) {
            int n0 = TNC_PART_LEN * partindex;
            for (int i = 0; i < TNC_PART_LEN && n0 + i <= _textCount; i++)
                if (!part[i].isNull() && !part[i].isPersistent()) {
                    //CRLog::trace("before persist");
                    part[i].persist();
                    //CRLog::trace("after persist");
                    if (maxTime.expired())
                        return;
                }
        }
    }
    //_cacheFile->flush(false); // intermediate flush
}

void tinyNodeCollection::dropStyles() {
    _styles.clear(-1);
    _fonts.clear(-1);
    resetNodeNumberingProps();

    int count = ((_elemCount + TNC_PART_LEN - 1) >> TNC_PART_SHIFT);
    for (int i = 0; i < count; i++) {
        int offs = i * TNC_PART_LEN;
        int sz = TNC_PART_LEN;
        if (offs + sz > _elemCount + 1) {
            sz = _elemCount + 1 - offs;
        }
        ldomNode* buf = _elemList[i];
        for (int j = 0; j < sz; j++) {
            if (buf[j].isElement()) {
                setNodeStyleIndex(buf[j]._handle._dataIndex, 0);
                setNodeFontIndex(buf[j]._handle._dataIndex, 0);
            }
        }
    }
    _nodeStyleHash = 0;
}

int tinyNodeCollection::calcFinalBlocks() {
    int cnt = 0;
    int count = ((_elemCount + TNC_PART_LEN - 1) >> TNC_PART_SHIFT);
    for (int i = 0; i < count; i++) {
        int offs = i * TNC_PART_LEN;
        int sz = TNC_PART_LEN;
        if (offs + sz > _elemCount + 1) {
            sz = _elemCount + 1 - offs;
        }
        ldomNode* buf = _elemList[i];
        for (int j = 0; j < sz; j++) {
            if (buf[j].isElement()) {
                int rm = buf[j].getRendMethod();
                if (rm == erm_final)
                    cnt++;
            }
        }
    }
    return cnt;
}

void tinyNodeCollection::setDocFlag(lUInt32 mask, bool value) {
    CRLog::debug("setDocFlag(%04x, %s)", mask, value ? "true" : "false");
    if (value)
        _docFlags |= mask;
    else
        _docFlags &= ~mask;
}

void tinyNodeCollection::setDocFlags(lUInt32 value) {
    CRLog::debug("setDocFlags(%04x)", value);
    _docFlags = value;
}

int tinyNodeCollection::getPersistenceFlags() {
    int format = 2; //getProps()->getIntDef(DOC_PROP_FILE_FORMAT, 0);
    int flag = (format == 2 && getDocFlag(DOC_FLAG_PREFORMATTED_TEXT)) ? 1 : 0;
    CRLog::trace("getPersistenceFlags() returned %d", flag);
    return flag;
}

bool tinyNodeCollection::saveStylesData() {
    SerialBuf stylebuf(0, true);
    lUInt32 stHash = _stylesheet.getHash();
    LVArray<css_style_ref_t>* list = _styles.getIndex();
    stylebuf.putMagic(styles_magic);
    stylebuf << stHash;
    stylebuf << (lUInt32)list->length(); // index
    for (int i = 0; i < list->length(); i++) {
        css_style_ref_t rec = list->get(i);
        if (!rec.isNull()) {
            stylebuf << (lUInt32)i;   // index
            rec->serialize(stylebuf); // style
        }
    }
    stylebuf << (lUInt32)0; // index=0 is end list mark
    stylebuf.putMagic(styles_magic);
    delete list;
    if (stylebuf.error())
        return false;
    CRLog::trace("Writing style data: %d bytes", stylebuf.pos());
    if (!_cacheFile->write(CBT_STYLE_DATA, stylebuf, COMPRESS_STYLE_DATA)) {
        return false;
    }
    return !stylebuf.error();
}

bool tinyNodeCollection::loadStylesData() {
    SerialBuf stylebuf(0, true);
    if (!_cacheFile->read(CBT_STYLE_DATA, stylebuf)) {
        CRLog::error("Error while reading style data");
        return false;
    }
    lUInt32 stHash = 0;
    lInt32 len = 0;

    // lUInt32 myHash = _stylesheet.getHash();
    // When loading from cache, this stylesheet was built with the
    // initial element name ids, which may have been replaced by
    // the one restored from the cache. So, its hash may be different
    // from the one we're going to load from cache.
    // This is not a failure, but a sign the stylesheet will have
    // to be regenerated (later, no need for it currently as we're
    // loading previously applied style data): this will be checked
    // in checkRenderContext() when comparing a combo hash
    // against _hdr.stylesheet_hash fetched from the cache.

    //LVArray<css_style_ref_t> * list = _styles.getIndex();
    stylebuf.checkMagic(styles_magic);
    stylebuf >> stHash;
    // Don't check for this:
    // if ( stHash != myHash ) {
    //     CRLog::info("tinyNodeCollection::loadStylesData() - stylesheet hash is changed: skip loading styles");
    //     return false;
    // }
    stylebuf >> len; // index
    if (stylebuf.error())
        return false;
    LVArray<css_style_ref_t> list(len, css_style_ref_t());
    for (int i = 0; i < list.length(); i++) {
        lUInt32 index = 0;
        stylebuf >> index; // index
        if (index <= 0 || (int)index >= len || stylebuf.error())
            break;
        css_style_ref_t rec(new css_style_rec_t());
        if (!rec->deserialize(stylebuf))
            break;
        list.set(index, rec);
    }
    stylebuf.checkMagic(styles_magic);
    if (stylebuf.error())
        return false;

    CRLog::trace("Setting style data: %d bytes", stylebuf.size());
    _styles.setIndex(list);

    return !stylebuf.error();
}

lUInt32 tinyNodeCollection::calcStyleHash(bool already_rendered) {
    CRLog::debug("calcStyleHash start");
    //    int maxlog = 20;
    lUInt32 res = 0; //_elemCount;
    lUInt32 globalHash = calcGlobalSettingsHash(getFontContextDocIndex(), already_rendered);
    lUInt32 docFlags = getDocFlags();
    //CRLog::info("Calculating style hash...  elemCount=%d, globalHash=%08x, docFlags=%08x", _elemCount, globalHash, docFlags);
    if (_nodeStyleHash) {
        // Re-use saved _nodeStyleHash if it has not been invalidated,
        // as the following loop can be expensive
        res = _nodeStyleHash;
        CRLog::debug("  using saved _nodeStyleHash %x", res);
    } else {
        // We also compute _nodeDisplayStyleHash from each node style->display. It
        // may not change as often as _nodeStyleHash, but if it does, it means
        // some nodes switched between 'block' and 'inline', and that some autoBoxing
        // that may have been added should no more be in the DOM for a correct
        // rendering: in that case, the user will have to reload the document, and
        // we should invalidate the cache so a new correct DOM is build on load.
        _nodeDisplayStyleHash = 0;

        int count = ((_elemCount + TNC_PART_LEN - 1) >> TNC_PART_SHIFT);
        for (int i = 0; i < count; i++) {
            int offs = i * TNC_PART_LEN;
            int sz = TNC_PART_LEN;
            if (offs + sz > _elemCount + 1) {
                sz = _elemCount + 1 - offs;
            }
            ldomNode* buf = _elemList[i];
            if (!buf)
                continue; // avoid clang-tidy warning
            for (int j = 0; j < sz; j++) {
                if (buf[j].isElement()) {
                    css_style_ref_t style = buf[j].getStyle();
                    lUInt32 sh = calcHash(style);
                    res = res * 31 + sh;
                    if (!style.isNull()) {
                        _nodeDisplayStyleHash = _nodeDisplayStyleHash * 31 + style.get()->display;
                        // Also account in this hash if this node is "white_space: pre" or alike.
                        // If white_space changes from/to "pre"-like to/from "normal"-like,
                        // the document will need to be reloaded so that the HTML text parts
                        // are parsed according the the PRE/not-PRE rules
                        if (style.get()->white_space >= css_ws_pre_line)
                            _nodeDisplayStyleHash += 29;
                        // Also account for style->float_, as it should create/remove new floatBox
                        // elements wrapping floats when toggling BLOCK_RENDERING_ENHANCED
                        if (style.get()->float_ > css_f_none)
                            _nodeDisplayStyleHash += 123;
                    }
                    //printf("element %d %d style hash: %x\n", i, j, sh);
                    LVFontRef font = buf[j].getFont();
                    lUInt32 fh = calcHash(font);
                    res = res * 31 + fh;
                    //printf("element %d %d font hash: %x\n", i, j, fh);
                    //                    if ( maxlog>0 && sh==0 ) {
                    //                        style = buf[j].getStyle();
                    //                        CRLog::trace("[%06d] : s=%08x f=%08x  res=%08x", offs+j, sh, fh, res);
                    //                        maxlog--;
                    //                    }
                }
            }
        }

        CRLog::debug("  COMPUTED _nodeStyleHash %x", res);
        _nodeStyleHash = res;
        CRLog::debug("  COMPUTED _nodeDisplayStyleHash %x (initial: %x)", _nodeDisplayStyleHash, _nodeDisplayStyleHashInitial);
    }
    CRLog::info("Calculating style hash...  elemCount=%d, globalHash=%08x, docFlags=%08x, nodeStyleHash=%08x", _elemCount, globalHash, docFlags, res);
    res = res * 31 + _imgScalingOptions.getHash();
    res = res * 31 + (_imgAutoRotate ? 1 : 0);
    res = res * 31 + _spaceWidthScalePercent;
    res = res * 31 + _minSpaceCondensingPercent;
    res = res * 31 + _unusedSpaceThresholdPercent;

    // _maxAddedLetterSpacingPercent does not need to be accounted, as, working
    // only on a laid out line, it does not need a re-rendering, but just
    // a _renderedBlockCache.clear() to reformat paragraphs and have the
    // word re-positioned (the paragraphs width & height do not change)

    // Hanging punctuation does not need to trigger a re-render, as
    // it's now ensured by alignLine() and won't change paragraphs height.
    // We just need to _renderedBlockCache.clear() when it changes.
    // if ( _hangingPunctuationEnabled )
    //     res = res * 75 + 1761;

    res = res * 31 + _renderBlockRenderingFlags;
    res = res * 31 + _interlineScaleFactor;

    res = (res * 31 + globalHash) * 31 + docFlags;
    //    CRLog::info("Calculated style hash = %08x", res);
    CRLog::debug("calcStyleHash done");
    return res;
}

static void validateChild(ldomNode* node) {
    // DEBUG TEST
    if (!node->isRoot() && node->getParentNode()->getChildIndex(node->getDataIndex()) < 0) {
        CRLog::error("Invalid parent->child relation for nodes %d->%d", node->getParentNode()->getDataIndex(), node->getParentNode()->getDataIndex());
    }
}

/// called on document loading end
bool tinyNodeCollection::validateDocument() {
    ((ldomDocument*)this)->getRootNode()->recurseElements(validateChild);
    int count = ((_elemCount + TNC_PART_LEN - 1) >> TNC_PART_SHIFT);
    bool res = true;
    for (int i = 0; i < count; i++) {
        int offs = i * TNC_PART_LEN;
        int sz = TNC_PART_LEN;
        if (offs + sz > _elemCount + 1) {
            sz = _elemCount + 1 - offs;
        }
        ldomNode* buf = _elemList[i];
        for (int j = 0; j < sz; j++) {
            buf[j].setDocumentIndex(_docIndex);
            if (buf[j].isElement()) {
                lUInt16 style = getNodeStyleIndex(buf[j]._handle._dataIndex);
                lUInt16 font = getNodeFontIndex(buf[j]._handle._dataIndex);
                ;
                if (!style) {
                    if (!buf[j].isRoot()) {
                        CRLog::error("styleId=0 for node <%s> %d", LCSTR(buf[j].getNodeName()), buf[j].getDataIndex());
                        res = false;
                    }
                } else if (_styles.get(style).isNull()) {
                    CRLog::error("styleId!=0, but absent in cache for node <%s> %d", LCSTR(buf[j].getNodeName()), buf[j].getDataIndex());
                    res = false;
                }
                if (!font) {
                    if (!buf[j].isRoot()) {
                        CRLog::error("fontId=0 for node <%s>", LCSTR(buf[j].getNodeName()));
                        res = false;
                    }
                } else if (_fonts.get(font).isNull()) {
                    CRLog::error("fontId!=0, but absent in cache for node <%s>", LCSTR(buf[j].getNodeName()));
                    res = false;
                }
            }
        }
    }
    return res;
}

bool tinyNodeCollection::updateLoadedStyles(bool enabled) {
    int count = ((_elemCount + TNC_PART_LEN - 1) >> TNC_PART_SHIFT);
    bool res = true;
    LVArray<css_style_ref_t>* list = _styles.getIndex();

    _fontMap.clear(); // style index to font index

    for (int i = 0; i < count; i++) {
        int offs = i * TNC_PART_LEN;
        int sz = TNC_PART_LEN;
        if (offs + sz > _elemCount + 1) {
            sz = _elemCount + 1 - offs;
        }
        ldomNode* buf = _elemList[i];
        for (int j = 0; j < sz; j++) {
            buf[j].setDocumentIndex(_docIndex);
            if (buf[j].isElement()) {
                lUInt16 style = getNodeStyleIndex(buf[j]._handle._dataIndex);
                if (enabled && style != 0) {
                    css_style_ref_t s = list->get(style);
                    if (!s.isNull()) {
                        lUInt16 fntIndex = _fontMap.get(style);
                        if (fntIndex == 0) {
                            LVFontRef fnt = getFont(&buf[j], s.get(), getFontContextDocIndex());
                            fntIndex = (lUInt16)_fonts.cache(fnt);
                            if (fnt.isNull()) {
                                CRLog::error("font not found for style!");
                            } else {
                                _fontMap.set(style, fntIndex);
                            }
                        } else {
                            _fonts.addIndexRef(fntIndex);
                        }
                        if (fntIndex <= 0) {
                            CRLog::error("font caching failed for style!");
                            res = false;
                        } else {
                            setNodeFontIndex(buf[j]._handle._dataIndex, fntIndex);
                            //buf[j]._data._pelem._fontIndex = fntIndex;
                        }
                    } else {
                        CRLog::error("Loaded style index %d not found in style collection", (int)style);
                        setNodeFontIndex(buf[j]._handle._dataIndex, 0);
                        setNodeStyleIndex(buf[j]._handle._dataIndex, 0);
                        //                        buf[j]._data._pelem._styleIndex = 0;
                        //                        buf[j]._data._pelem._fontIndex = 0;
                        res = false;
                    }
                } else {
                    setNodeFontIndex(buf[j]._handle._dataIndex, 0);
                    setNodeStyleIndex(buf[j]._handle._dataIndex, 0);
                    //                    buf[j]._data._pelem._styleIndex = 0;
                    //                    buf[j]._data._pelem._fontIndex = 0;
                }
            }
        }
    }
#ifdef TODO_INVESTIGATE
    if (enabled && res) {
        //_styles.setIndex( *list );
        // correct list reference counters

        for (int i = 0; i < list->length(); i++) {
            if (!list->get(i).isNull()) {
                // decrease reference counter
                // TODO:
                //_styles.release( list->get(i) );
            }
        }
    }
#endif
    delete list;
    //    getRootNode()->setFont( _def_font );
    //    getRootNode()->setStyle( _def_style );
    _nodeStyleHash = 0;
    return res;
}

lUInt32 calcGlobalSettingsHash(int documentId, bool already_rendered) {
    lUInt32 hash = FORMATTING_VERSION_ID;
    hash = hash * 31 + (int)fontMan->GetShapingMode();
    if (fontMan->GetKerning())
        hash = hash * 75 + 1761;
    hash = hash * 31 + fontMan->GetFontListHash(documentId);
    hash = hash * 31 + (int)fontMan->GetHintingMode();
    hash = hash * 31 + LVRendGetBaseFontWeight();
    hash = hash * 31 + fontMan->GetFallbackFontFaces().getHash();
    hash = hash * 31 + getGenericFontFamilyFace(css_ff_serif).getHash();
    hash = hash * 31 + getGenericFontFamilyFace(css_ff_sans_serif).getHash();
    hash = hash * 31 + getGenericFontFamilyFace(css_ff_cursive).getHash();
    hash = hash * 31 + getGenericFontFamilyFace(css_ff_fantasy).getHash();
    hash = hash * 31 + getGenericFontFamilyFace(css_ff_monospace).getHash();
    hash = hash * 31 + gRenderDPI;
    // If not yet rendered (initial loading with XML parsing), we can
    // ignore some global flags that have not yet produced any effect,
    // so they can possibly be updated between loading and rendering
    // without trigerring a drop of all the styles and rend methods
    // set up in the XML loading phase. This is mostly only needed
    // for TextLangMan::getHash(), as the lang can be set by frontend
    // code after the loading phase, once the book language is known
    // from its metadata, before the rendering that will use the
    // language set. (We could ignore some of the other settings
    // above if we ever need to reset them in between these phases;
    // just be certain they are really not used in the first phase.)
    if (already_rendered) {
        hash = hash * 31 + TextLangMan::getHash();
        hash = hash * 31 + HyphMan::getOverriddenLeftHyphenMin();
        hash = hash * 31 + HyphMan::getOverriddenRightHyphenMin();
        hash = hash * 31 + HyphMan::getTrustSoftHyphens();
    }
    return hash;
}
