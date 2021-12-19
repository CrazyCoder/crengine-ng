#ifndef CHMFMT_H
#define CHMFMT_H

#include "../include/crsetup.h"

#if USE_CHM==1

#include "../include/lvtinydom.h"

bool DetectCHMFormat( LVStreamRef stream );
bool ImportCHMDocument( LVStreamRef stream, ldomDocument * doc, LVDocViewCallback * progressCallback, CacheLoadingCallback * formatCallback );

/// opens CHM container
LVContainerRef LVOpenCHMContainer( LVStreamRef stream );

#endif

#endif // CHMFMT_H
