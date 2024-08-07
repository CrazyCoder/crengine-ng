# crengine-ng
crengine-ng is cross-platform library designed to implement text viewers and e-book readers.

## Description
Supported document formats: fb2, fb3, epub (without DRM), rtf, doc, docx, odt, htm, Markdown, chm, pdb, mobi (without DRM), txt, trc, prc.

In fact, this is a fork of the [CoolReader](https://github.com/buggins/coolreader) project.

The founder and developer of CoolReader is Vadim Lopatin. Other authors are listed in AUTHORS file.

Supported platforms: Windows, Linux, MacOS.

This repository is created from the official CoolReader repository by removing everything that does not belong to the "crengine" library using the "git-filter-repo" utility.

## Project goals
This project is in the experimental phase, the main goals are:
 * Main goal is the same as CoolReader's crengine - support & development fast and portable library which allows to create e-book readers for different platform including handheld devices.
 * Some minor improvements are possible.
 * Refactoring legacy code to make it easier to work on codebase. At the moment, the codebase in CoolReader is very cumbersome, poorly structured, poorly documented, there are huge source files, which are very difficult to work with.
 * Bugfixes.
 * Support for modern compilers.

## Applications
[crqt-ng](https://gitlab.com/coolreader-ng/crqt-ng) - cross-platform desktop e-book reader (using the Qt framework).

[crwx-ng](https://gitlab.com/coolreader-ng/crwx-ng) - cross-platform desktop e-book reader (using wxWidgets).

[LxReader](https://gitlab.com/coolreader-ng/lxreader) - e-book reader for Android.

## External dependencies
This library can use other libraries (links to licenses point to official websites whenever possible):
 * FreeType - font rendering (any of your choice [GPLv2](https://gitlab.freedesktop.org/freetype/freetype/-/blob/master/docs/GPLv2.TXT) or [FTL](https://gitlab.freedesktop.org/freetype/freetype/-/blob/master/docs/FTL.TXT))
 * HarfBuzz - text shaping, font kerning, ligatures ([Old MIT](https://github.com/harfbuzz/harfbuzz/blob/main/COPYING))
 * ZLib - compressing library (cache compression, zip-archives support) ([ZLib](https://www.zlib.net/zlib_license.html))
 * ZSTD - compressing library (cache compression) ([BSD](https://github.com/facebook/zstd/blob/dev/LICENSE) and [GPLv2](https://github.com/facebook/zstd/blob/dev/COPYING))
 * libpng - PNG image format support ([libpng](http://www.libpng.org/pub/png/src/libpng-LICENSE.txt))
 * libjpeg - JPEG image format support ([libjpeg](https://jpegclub.org/reference/libjpeg-license/))
 * FriBiDi - RTL writing support ([LGPL-2.1](https://github.com/fribidi/fribidi/blob/master/COPYING))
 * libunibreak - line breaking and word breaking algorithms ([Zlib](https://github.com/adah1972/libunibreak/blob/master/LICENCE))
 * utf8proc - for unicode string comparision ([utf8proc (MIT-like) + Unicode data license](https://github.com/JuliaStrings/utf8proc/blob/master/LICENSE.md))
 * GoogleTest - for unit testing, optional ([BSD-3-Clause](https://github.com/google/googletest/blob/main/LICENSE))

## Installation
1. Install dependencies (via package manager or compile from sources)

   To install these libraries for Ubuntu you can use the following command:
```sh
   $ sudo apt install zlib1g-dev libpng-dev libjpeg-dev libfreetype6-dev libfontconfig1-dev libharfbuzz-dev libfribidi-dev libunibreak-dev libutf8proc-dev libzstd-dev
```
   You may also need to install the main packages necessary for development:
```sh
   $ sudo apt install build-essential git cmake curl pkg-config
```

   Some distributions do not contain the required versions of certain libraries, in which case you can build these libraries yourself from sources. The procedure for installing such libraries from sources is outside the scope of this document.

2. Build using cmake.
   cmake options:
   * CRE_BUILD_SHARED - Building crengine-ng as a shared library; default ON
   * CRE_BUILD_STATIC - Building crengine-ng as a static library; default ON
   * ADD_DEBUG_EXTRA_OPTS - Add extra debug flags and technique (use sanitizer, etc); default OFF
   * DOC_DATA_COMPRESSION_LEVEL - Document buffer cache compression; default 1; variants: 0, 1, 2, 3, 4, 5
   * DOC_BUFFER_SIZE - Total RAM buffers size for document; default 0x400000
   * MAX_IMAGE_SCALE_MUL - Maximum image scale multiplier; default 0; variants: 0, 1, 2
   * GRAY_BACKBUFFER_BITS - Gray depth for gray backbuffer; default 4; variants: 1, 2, 3, 4, 8
   * ENABLE_LARGEFILE_SUPPORT - Enable large files support; default ON
   * USE_COLOR_BACKBUFFER - Use color backbuffer; default ON
   * USE_LOCALE_DATA - Use built-in locale data; default ON
   * LDOM_USE_OWN_MEM_MAN - Use own ldom memory manager; default ON
   * USE_GIF - Allow GIF support via embedded decoder; default ON
   * BUILD_TOOLS - Build some debug tools & utils; default OFF
   * ENABLE_UNITTESTING - Enable building unit tests using googletest; default OFF
   * OFFLINE_BUILD_MODE - Offline build mode; default OFF
   * ENABLE_LTO - Enable Link Time Optimization; default OFF

   To use bundled third party:

   * USE_NANOSVG - Use nanosvg for svg image support; default ON
   * USE_CHM - Enable chm support via built-in chmlib; default ON
   * USE_ANTIWORD - Enable doc support via built-in antiword; default ON
   * USE_SHASUM - Use sources from RFC6234 to calculate SHA sums (engine will create fingerprint for the document opened); default OFF
   * USE_CMARK_GFM - Enable Markdown support via built-in cmark-gfm; default OFF
   * USE_MD4C - Enable Markdown support via built-in MD4C; default ON

   To use external dependencies:

   * WITH_LIBPNG - Use libpng library for png image support; default AUTO; variants: AUTO, ON, OFF
   * WITH_LIBJPEG - Use libjpeg library for jpeg image support; default AUTO; variants: AUTO, ON, OFF
   * WITH_FREETYPE - Use FreeType library for font rendering; default AUTO; variants: AUTO, ON, OFF
   * WITH_HARFBUZZ - Use HarfBuzz library for text shaping; default AUTO; variants: AUTO, ON, OFF
   * WITH_LIBUNIBREAK - Use libunibreak library; default AUTO; variants: AUTO, ON, OFF
   * WITH_FRIBIDI - Use FriBiDi library for RTL text; default AUTO; variants: AUTO, ON, OFF
   * WITH_ZSTD - Use zstd for cache compression; default AUTO; variants: AUTO, ON, OFF
   * WITH_UTF8PROC - Use utf8proc for string comparison, etc; default AUTO; variants: AUTO, ON, OFF
   * USE_FONTCONFIG - Use FontConfig to enumerate available fonts; default ON

   For example, we are already in the top source directory, then:
```sh
$ mkdir ../crengine-ng-build
$ cd ../crengine-ng-build
$ cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release -DCRE_BUILD_SHARED=ON -DCRE_BUILD_STATIC=OFF ../crengine-ng
$ make -j10 VERBOSE=1
$ make install
```

Note about **OFFLINE_BUILD_MODE**: This mode only affects build of unit tests.
This mode uses the GoogleTest library installed in the system.
The GNU FreeFont fonts must first be copied to the "${CMAKE_BINARY_DIR}/crengine/tests/fonts" folder.

For example, in the build directory, you can run the following commands:
```sh
$ wget -c http://ftp.gnu.org/gnu/freefont/freefont-otf-20120503.tar.gz
$ tar -xvzf freefont-otf-20120503.tar.gz -C crengine/tests/fonts/ --strip-components=1 --wildcards '*.otf'
```
When this mode is disabled, all these components are downloaded automatically using cmake.

## Contributing
See [CONTRIBUTING.md](CONTRIBUTING.md) file.

## Authors and acknowledgment
The list of contributors can be found in the AUTHORS file.

## License
This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

## Embedded third party components
The "thirdparty" directory contains some patched libraries with their own licenses (compatible with GPLv2):
 * chmlib - [LGPL-2.1+](https://www.gnu.org/licenses/lgpl-2.1.html)
 * antiword - [GPL-2.0+](https://www.gnu.org/licenses/gpl-2.0.html)
 * nanosvg - [ZLib](https://directory.fsf.org/wiki/License:Zlib)
 * qimagescale - [imlib2](https://directory.fsf.org/wiki/License:Imlib2), [LGPL-3](https://www.gnu.org/licenses/lgpl-3.0.html), [GPL-2.0+](https://www.gnu.org/licenses/gpl-2.0.html)
 * rfc6234-shas - [BSD-3-Clause](https://directory.fsf.org/wiki/License:BSD-3-Clause)
 * cmark-gfm - [BSD-2-Clause](https://directory.fsf.org/wiki/License:BSD-2-Clause)
 * MD4C - [MIT/Expat](https://directory.fsf.org/wiki/License:MIT)

Other third party can be found in the sources and resources:
 * xxhash - [BSD-2-Clause](https://directory.fsf.org/wiki/License:BSD-2-Clause)
 * fc-lang - [FontConfig, MIT like](https://gitlab.freedesktop.org/fontconfig/fontconfig/-/blob/main/COPYING)
 * libiconv (fragments) - [LGPLv2+](https://www.gnu.org/licenses/old-licenses/lgpl-2.0.html)
