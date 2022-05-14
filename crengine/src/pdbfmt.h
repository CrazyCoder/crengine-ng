/** @file lvcontaineriteminfo.h

    CoolReader Engine

    (c) Vadim Lopatin, 2011-2014

    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.

 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#ifndef PDBFMT_H
#define PDBFMT_H

#include <crsetup.h>
#include <lvcontainer.h>
#include <crbookformats.h>

class ldomDocument;
class LVDocViewCallback;
class CacheLoadingCallback;

// creates PDB decoder stream for stream
//LVStreamRef LVOpenPDBStream( LVStreamRef srcstream, int &format );

bool DetectPDBFormat(LVStreamRef stream, doc_format_t& contentFormat);
bool ImportPDBDocument(LVStreamRef& stream, ldomDocument* doc, LVDocViewCallback* progressCallback, CacheLoadingCallback* formatCallback, doc_format_t& contentFormat);
LVStreamRef GetPDBCoverpage(LVStreamRef stream);

#endif // PDBFMT_H
