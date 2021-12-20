project(crengine-ng)
cmake_minimum_required(VERSION 3.4)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")

#  VERSION_DEV_A/CODE description:
# alphaX - 0X; beta X - 1X; rcX - 2X; releaseX - 3X (or empty)
set(VERSION_MAJOR 4)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
set(VERSION_DEV_A "rc1")
set(VERSION_DEV_CODE 21)
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
if (VERSION_DEV_A)
  set(VERSION "${VERSION}-${VERSION_DEV_A}")
endif()
set(PACKAGE_VERSION ${VERSION})
# for windows rc file
set(PRODUCTVERSION_V "${VERSION_MAJOR},${VERSION_MINOR},${VERSION_PATCH},${VERSION_DEV_CODE}")
set(FILEVERSION_V ${PRODUCTVERSION_V})

option(CRE_BUILD_SHARED "Building crengine-ng as a shared library" ON)
option(CRE_BUILD_STATIC "Building crengine-ng as a static library" ON)
option(ADD_DEBUG_EXTRA_OPTS "Add extra debug flags and technique" OFF)
set(DOC_DATA_COMPRESSION_LEVEL 1 CACHE STRING "Document buffer cache compression")
set_property(CACHE DOC_DATA_COMPRESSION_LEVEL PROPERTY STRINGS 0 1 2 3 4 5)
set(DOC_BUFFER_SIZE 0x400000 CACHE STRING "Total RAM buffers size for document")
set_property(CACHE DOC_BUFFER_SIZE PROPERTY STRINGS 0x10000 0x40000 0x100000 0x400000 0x800000 0x1000000)
set(MAX_IMAGE_SCALE_MUL 0 CACHE STRING "Maximum image scale multiplier")
set_property(CACHE MAX_IMAGE_SCALE_MUL PROPERTY STRINGS 0 1 2)
set(GRAY_BACKBUFFER_BITS 4 CACHE STRING "Gray depth for gray backbuffer")
set_property(CACHE GRAY_BACKBUFFER_BITS PROPERTY STRINGS 1 2 3 4 8)
option(ENABLE_LARGEFILE_SUPPORT "Enable large files support" ON)
option(USE_COLOR_BACKBUFFER "Use color backbuffer" ON)
option(USE_LOCALE_DATA "Use built-in locale data" ON)
option(LDOM_USE_OWN_MEM_MAN "Use own ldom memory manager" ON)

# Libraries support
set(WITH_LIBPNG AUTO CACHE STRING "Use libpng library for png image support")
set_property(CACHE WITH_LIBPNG PROPERTY STRINGS AUTO ON OFF)
set(WITH_LIBJPEG AUTO CACHE STRING "Use libjpeg library for jpeg image support")
set_property(CACHE WITH_LIBJPEG PROPERTY STRINGS AUTO ON OFF)
set(WITH_FREETYPE AUTO CACHE STRING "Use FreeType library for font rendering")
set_property(CACHE WITH_FREETYPE PROPERTY STRINGS AUTO ON OFF)
set(WITH_HARFBUZZ AUTO CACHE STRING "Use HarfBuzz library for text shaping")
set_property(CACHE WITH_HARFBUZZ PROPERTY STRINGS AUTO ON OFF)
set(WITH_LIBUNIBREAK AUTO CACHE STRING "Use libunibreak library")
set_property(CACHE WITH_LIBUNIBREAK PROPERTY STRINGS AUTO ON OFF)
set(WITH_FRIBIDI AUTO CACHE STRING "Use FriBiDi library for RTL text")
set_property(CACHE WITH_FRIBIDI PROPERTY STRINGS AUTO ON OFF)
set(WITH_ZSTD AUTO CACHE STRING "Use zstd for cache compression")
set_property(CACHE WITH_ZSTD PROPERTY STRINGS AUTO ON OFF)
set(WITH_UTF8PROC AUTO CACHE STRING "Use utf8proc for string comparison, etc")
set_property(CACHE WITH_UTF8PROC PROPERTY STRINGS AUTO ON OFF)
option(USE_GIF "Allow GIF support via embedded decoder" ON)
option(USE_NANOSVG "Use nanosvg for svg image support" ON)
option(USE_CHM "Enable chm support via built-in chmlib" ON)
option(USE_ANTIWORD "Enable doc support via built-in antiword" ON)
option(USE_FONTCONFIG "Use FontConfig to enumerate available fonts" ON)

# other
option(BUILD_TOOLS "Build some debug tools & utils" OFF)
option(BUILD_LITE  "Remove most features" OFF)

set(PUBLIC_COMPILE_DEFINITIONS)
set(PRIVATE_COMPILE_DEFINITIONS)

if( ${CMAKE_SYSTEM} MATCHES "Darwin" )
  set(MAC 1)
endif( ${CMAKE_SYSTEM} MATCHES "Darwin" )

if(NOT CRE_BUILD_SHARED AND NOT CRE_BUILD_STATIC)
  message(FATAL_ERROR "Nothing to build...")
endif(NOT CRE_BUILD_SHARED AND NOT CRE_BUILD_STATIC)

set(CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG}     -D_DEBUG=1 -DDEBUG=1")
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   -D_DEBUG=1 -DDEBUG=1")
set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}   -DNDEBUG=1")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG=1")

if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # Update if necessary
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-long-long -pedantic -std=gnu11 -fPIC -Wno-misleading-indentation")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic -std=c++11 -fPIC -Wno-misleading-indentation -Wno-reorder -Wno-format-overflow")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fomit-frame-pointer")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fomit-frame-pointer")
endif()

if(MAC)
  list(APPEND PUBLIC_COMPILE_DEFINITIONS -DMAC=1 -DLINUX=1 -D_LINUX=1 -DCR_EMULATE_GETTEXT=1)
elseif ( WIN32 )
  list(APPEND PUBLIC_COMPILE_DEFINITIONS -DWIN32=1 -D_WIN32=1 -DCR_EMULATE_GETTEXT=1 -DNOMINMAX=1 -D_CRT_SECURE_NO_WARNINGS)
else()
  list(APPEND PUBLIC_COMPILE_DEFINITIONS -DLINUX=1 -D_LINUX=1)
endif(MAC)

if(ADD_DEBUG_EXTRA_OPTS)
  add_definitions(-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC)
  set(DEBUG_EXTRA_FLAGS "-fstack-check -fstack-protector -fno-omit-frame-pointer -Wshift-overflow=2")
  if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    set (DEBUG_EXTRA_FLAGS "${DEBUG_EXTRA_FLAGS} -fsanitize=address -fsanitize=undefined -fno-sanitize-recover")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-check -fstack-protector")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-check -fstack-protector")
  endif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${DEBUG_EXTRA_FLAGS}")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${DEBUG_EXTRA_FLAGS}")
endif(ADD_DEBUG_EXTRA_OPTS)

if(BUILD_LITE)
  message("Enabled lite build, disabling most options...")
  set(WITH_ZSTD NO)
  set(USE_ZSTD NO)
  set(WITH_LIBJPEG NO)
  set(USE_LIBJPEG NO)
  set(WITH_LIBPNG NO)
  set(USE_LIBPNG NO)
  set(USE_GIF NO)
  set(WITH_FREETYPE NO)
  set(USE_FREETYPE NO)
  set(WITH_HARFBUZZ NO)
  set(USE_HARFBUZZ NO)
  set(WITH_FRIBIDI NO)
  set(USE_FRIBIDI NO)
  set(WITH_LIBUNIBREAK NO)
  set(USE_LIBUNIBREAK NO)
  set(USE_LOCALE_DATA NO)
  set(DOC_DATA_COMPRESSION_LEVEL 0)
  set(DOC_BUFFER_SIZE 0x10000)
endif()

# Document buffer cache / compression settings
if (NOT DEFINED DOC_DATA_COMPRESSION_LEVEL)
  message("-D DOC_DATA_COMPRESSION_LEVEL=0|1|2|3|4|5 parameter is not defined: will use default value (1)")
  set(DOC_DATA_COMPRESSION_LEVEL 1)
endif (NOT DEFINED DOC_DATA_COMPRESSION_LEVEL)
list(APPEND PRIVATE_COMPILE_DEFINITIONS -DDOC_DATA_COMPRESSION_LEVEL=${DOC_DATA_COMPRESSION_LEVEL})

# Total RAM buffers size for document
if (NOT DEFINED DOC_BUFFER_SIZE)
  message("-D DOC_BUFFER_SIZE=N parameter is not defined: will use default value (0x400000)")
  set(DOC_BUFFER_SIZE 0x400000)
endif (NOT DEFINED DOC_BUFFER_SIZE)
list(APPEND PRIVATE_COMPILE_DEFINITIONS -DDOC_BUFFER_SIZE=${DOC_BUFFER_SIZE})

# maximum image scale multiplier
if (NOT DEFINED MAX_IMAGE_SCALE_MUL)
  message("-D MAX_IMAGE_SCALE_MUL=N parameter is not defined: will use default value (0)")
  set(MAX_IMAGE_SCALE_MUL 0)
endif (NOT DEFINED MAX_IMAGE_SCALE_MUL)
list(APPEND PRIVATE_COMPILE_DEFINITIONS -DMAX_IMAGE_SCALE_MUL=${MAX_IMAGE_SCALE_MUL})

# Large file support
if (ENABLE_LARGEFILE_SUPPORT)
  list(APPEND PRIVATE_COMPILE_DEFINITIONS -D_LARGEFILE64_SOURCE=1 -D_FILE_OFFSET_BITS=64)
  list(APPEND PUBLIC_COMPILE_DEFINITIONS -DLVLONG_FILE_SUPPORT=1)
endif(ENABLE_LARGEFILE_SUPPORT)

if (NOT USE_COLOR_BACKBUFFER)
  if (NOT DEFINED GRAY_BACKBUFFER_BITS)
    set(GRAY_BACKBUFFER_BITS 4)
  endif (NOT DEFINED GRAY_BACKBUFFER_BITS)
endif (NOT USE_COLOR_BACKBUFFER)
set(COLOR_BACKBUFFER ${USE_COLOR_BACKBUFFER})

# Libraries

# zlib library
find_package(ZLIB REQUIRED)
set(USE_ZLIB ${ZLIB_FOUND} CACHE STRING "Use zlib library")

# libpng library
if(WITH_LIBPNG STREQUAL "AUTO")
  find_package(PNG)
elseif(WITH_LIBPNG)
  find_package(PNG REQUIRED)
endif()
set(USE_LIBPNG ${PNG_FOUND} CACHE STRING "Use libpng library")

# jpeg library
if(WITH_LIBJPEG STREQUAL "AUTO")
  find_package(JPEG)
elseif(WITH_LIBJPEG)
  find_package(JPEG REQUIRED)
endif()
set(USE_LIBJPEG ${JPEG_FOUND} CACHE STRING "Use jpeg library")

# FreeType library
set(FREETYPE_MIN_VERSION "2.10.0")
if(WITH_FREETYPE STREQUAL "AUTO")
  find_package(Freetype ${FREETYPE_MIN_VERSION})
elseif(WITH_FREETYPE)
  find_package(Freetype ${FREETYPE_MIN_VERSION} REQUIRED)
endif()
set(USE_FREETYPE ${FREETYPE_FOUND} CACHE STRING "Use FreeType library")

# HarfBuzz library
set(HARFBUZZ_MIN_VERSION "2.6.0")
if(WITH_HARFBUZZ STREQUAL "AUTO")
  find_package(HarfBuzz ${HARFBUZZ_MIN_VERSION})
elseif(WITH_HARFBUZZ)
  find_package(HarfBuzz ${HARFBUZZ_MIN_VERSION} REQUIRED)
endif()
set(USE_HARFBUZZ ${HarfBuzz_FOUND} CACHE STRING "Use HarfBuzz library")

# FriBiDi library
set(FRIBIDI_MIN_VERSION "1.0.0")
if(WITH_FRIBIDI STREQUAL "AUTO")
  find_package(FriBidi ${FRIBIDI_MIN_VERSION})
elseif(WITH_FRIBIDI)
  find_package(FriBidi ${FRIBIDI_MIN_VERSION} REQUIRED)
endif()
set(USE_FRIBIDI ${FRIBIDI_FOUND} CACHE STRING "Use FriBiDi library")

# libunibreak library
set(LIBUNIBREAK_MIN_VERSION "4.0")
if(WITH_LIBUNIBREAK STREQUAL "AUTO")
  find_package(libunibreak ${LIBUNIBREAK_MIN_VERSION})
elseif(WITH_LIBUNIBREAK)
  find_package(libunibreak ${LIBUNIBREAK_MIN_VERSION} REQUIRED)
endif()
set(USE_LIBUNIBREAK ${LIBUNIBREAK_FOUND} CACHE STRING "Use libunibreak library")

# zstd library
set(ZSTD_MIN_VERSION "1.4.0")
if(WITH_ZSTD STREQUAL "AUTO")
  find_package(ZSTD ${ZSTD_MIN_VERSION})
elseif(WITH_ZSTD)
  find_package(ZSTD ${ZSTD_MIN_VERSION} REQUIRED)
endif()
set(USE_ZSTD ${ZSTD_FOUND} CACHE STRING "Use zstd library")

# utf8proc library
if(WITH_UTF8PROC STREQUAL "AUTO")
  find_package(utf8proc)
elseif(WITH_UTF8PROC)
  find_package(utf8proc REQUIRED)
endif()
set(USE_UTF8PROC ${UTF8PROC_FOUND} CACHE STRING "Use utf8proc library")

# nanosvg library
if(USE_NANOSVG)
  include_directories(${CMAKE_SOURCE_DIR}/thirdparty/nanosvg/src)
endif()

# patched built-in libchm
if (USE_CHM)
  message("Will build patched LIBCHM library")
  include_directories(${CMAKE_SOURCE_DIR}/thirdparty/chmlib/src)
  add_subdirectory(thirdparty/chmlib)
  set(CHM_LIBRARIES cre_chmlib)
else()
  set(CHM_LIBRARIES)
endif(USE_CHM)

# patched built-in antiword
if (USE_ANTIWORD)
  message("Will build patched ANTIWORD library")
  include_directories( ${CMAKE_SOURCE_DIR}/thirdparty/antiword )
  add_subdirectory(thirdparty/antiword)
  set(ANTIWORD_LIBRARIES cre_antiword)
  target_compile_definitions(cre_antiword PRIVATE -DCR3_ANTIWORD_PATCH=1)
  list(APPEND PRIVATE_COMPILE_DEFINITIONS -DCR3_ANTIWORD_PATCH=1)
else()
  set(ANTIWORD_LIBRARIES)
endif()

# qimagescale library
add_subdirectory(thirdparty/qimagescale)
set(QIMAGESCALE_LIBRARIES cre_qimagescale)

set(FONTCONFIG_MIN_VERSION "2.11.0")
if (USE_FONTCONFIG)
  find_package(FontConfig ${FONTCONFIG_MIN_VERSION} REQUIRED)
else()
  set(FONTCONFIG_LIBRARIES)
endif()

include_directories(
  ${PNG_INCLUDE_DIR}
  ${JPEG_INCLUDE_DIR}
  ${ZLIB_INCLUDE_DIR}
  ${FREETYPE_INCLUDE_DIRS}
  ${HarfBuzz_INCLUDE_DIRS}
  ${ANTIWORD_INCLUDE_DIR}
  ${FRIBIDI_INCLUDE_DIR}
  ${LIBUNIBREAK_INCLUDE_DIR}
  ${ZSTD_INCLUDE_DIRS}
  ${UTF8PROC_INCLUDE_DIRS}
)

set(CRENGINE_NG_LIBS
  ${ZLIB_LIBRARIES}
  ${PNG_LIBRARIES}
  ${JPEG_LIBRARIES}
  ${FREETYPE_LIBRARIES}
  ${HarfBuzz_LIBRARY}
  ${FRIBIDI_LIBRARIES}
  ${LIBUNIBREAK_LIBRARIES}
  ${ZSTD_LIBRARIES}
  ${UTF8PROC_LIBRARIES}
  ${CHM_LIBRARIES}
  ${ANTIWORD_LIBRARIES}
  ${FONTCONFIG_LIBRARIES}
  ${UNRAR_LIBRARIES}
  ${QIMAGESCALE_LIBRARIES}
)

include_directories(crengine/include)
include_directories(${CMAKE_BINARY_DIR}/crengine)

add_subdirectory(crengine)
