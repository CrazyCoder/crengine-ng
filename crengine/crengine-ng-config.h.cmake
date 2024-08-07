#ifndef CRENGINE_NG_CONFIG_H
#define CRENGINE_NG_CONFIG_H

#cmakedefine CRE_NG_DATADIR "@CRE_NG_DATADIR@"
#cmakedefine CRE_NG_VERSION "@CRE_NG_VERSION@"

#cmakedefine DOC_DATA_COMPRESSION_LEVEL @DOC_DATA_COMPRESSION_LEVEL@
#cmakedefine01 USE_LOCALE_DATA
#cmakedefine01 COLOR_BACKBUFFER
#cmakedefine GRAY_BACKBUFFER_BITS @GRAY_BACKBUFFER_BITS@
#cmakedefine01 LDOM_USE_OWN_MEM_MAN

#cmakedefine01 USE_ZLIB
#cmakedefine01 USE_LIBPNG
#cmakedefine01 USE_LIBJPEG
#cmakedefine01 USE_GIF
#cmakedefine01 USE_FREETYPE
#cmakedefine01 USE_HARFBUZZ
#cmakedefine01 USE_LIBUNIBREAK
#cmakedefine01 USE_FRIBIDI
#cmakedefine01 USE_ZSTD
#cmakedefine01 USE_FONTCONFIG
#cmakedefine01 USE_UTF8PROC
#cmakedefine01 USE_CMARK_GFM
#cmakedefine01 USE_NANOSVG
#cmakedefine01 USE_CHM
#cmakedefine01 USE_ANTIWORD
#cmakedefine01 USE_SHASUM
#cmakedefine01 USE_MD4C

#endif  // CRENGINE_NG_CONFIG_H
