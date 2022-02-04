# crengine-ng
crengine-ng is cross-platform library designed to implement text viewers and e-book readers.

## Description
Supported document formats: fb2, fb3, epub (without DRM), rtf, doc, docx, odt, htm, chm, pdb, mobi (without DRM), txt, trc, prc.

In fact, this is a fork of the [CoolReader](https://github.com/buggins/coolreader) project.

The founder and developer of CoolReader is Vadim Lopatin. Other authors are listed in AUTHORS file.

Supported platforms: Windows, Linux, MacOS.

## Project goals
This project is in the experimental phase, the main goals are:
 * Main goal is the same as CoolReader's crengine - support & development fast and portable library which allows to create e-book readers for different platform including handheld devices.
 * Some minor improvements are possible.
 * Refactoring legacy code to make it easier to work on codebase. At the moment, the codebase in CoolReader is very cumbersome, poorly structured, poorly documented, there are huge source files, which are very difficult to work with.
 * Bugfixes.
 * Support for modern compilers.

## External dependencies
This library can use other libraries:
 * FreeType - font rendering
 * HarfBuzz - text shaping, font kerning, ligatures
 * ZLib - compressing library (cache compression, zip-archives support)
 * ZSTD - compressing library (cache compression)
 * libpng - PNG image format support
 * libjpeg - JPEG image format support
 * FriBiDi - RTL writing support
 * libunibreak - line breaking and word breaking algorithms
 * utf8proc - for unicode string comparision

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

   To use bundled third party:

   * USE_NANOSVG - Use nanosvg for svg image support; default ON
   * USE_CHM - Enable chm support via built-in chmlib; default ON
   * USE_ANTIWORD - Enable doc support via built-in antiword; default ON

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

## Authors and acknowledgment
The list of contributors can be found in the AUTHORS file.

This list of authors is obtained from git history using the script crengine/Tools/getauthors/getauthos.sh.
The first significant item in the git log is 'Mon Nov 9 16:33:34 2009 +0300', but the project started around 2000, so this list can be incomplete.

## License
This project is released under the [GNU General Public License Version 2](https://opensource.org/licenses/GPL-2.0). See LICENSE file.

The "thirdparty" directory contains some patched libraries with their own licenses (compatible with GPLv2):
 * chmlib - [LGPL-2.1](https://opensource.org/licenses/LGPL-2.1)
 * antiword - [GPL-2.0](https://opensource.org/licenses/GPL-2.0)
 * nanosvg - [ZLib](https://opensource.org/licenses/Zlib)
 * qimagescale - [imlib2](https://directory.fsf.org/wiki/License:Imlib2)
