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

#ifndef __LVTINYDOMUTILS_H_INCLUDED__
#define __LVTINYDOMUTILS_H_INCLUDED__

#include <lvstream.h>
#include <dtddef.h>

class ldomDocument;

//utils
/// extract authors from FB2 document, delimiter is lString32 by default
lString32 extractDocAuthors(ldomDocument* doc, lString32 delimiter = lString32::empty_str, bool shortMiddleName = true);
lString32 extractDocTitle(ldomDocument* doc);
lString32 extractDocLanguage(ldomDocument* doc);
/// returns "(Series Name #number)" if pSeriesNumber is NULL, separate name and number otherwise
lString32 extractDocSeries(ldomDocument* doc, int* pSeriesNumber = NULL);
lString32 extractDocKeywords(ldomDocument* doc);
lString32 extractDocDescription(ldomDocument* doc);

/// parse XML document from stream, returns NULL if failed
ldomDocument* LVParseXMLStream(LVStreamRef stream,
                               const elem_def_t* elem_table = NULL,
                               const attr_def_t* attr_table = NULL,
                               const ns_def_t* ns_table = NULL);

/// parse XML document from stream, returns NULL if failed
ldomDocument* LVParseHTMLStream(LVStreamRef stream,
                                const elem_def_t* elem_table = NULL,
                                const attr_def_t* attr_table = NULL,
                                const ns_def_t* ns_table = NULL);

#endif // __LVTINYDOMUTILS_H_INCLUDED__
