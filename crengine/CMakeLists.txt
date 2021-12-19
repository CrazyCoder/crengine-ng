
configure_file(crengine-ng-config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/crengine-ng-config.h)

set(CRENGINE_NG_SOURCES
    src/cp_stats.cpp
    src/lvstring.cpp
    src/lvstring8collection.cpp
    src/lvstring32collection.cpp
    src/lvstring32hashedcollection.cpp
    src/serialbuf.cpp
    src/crlog.cpp
    src/props.cpp
    src/lstridmap.cpp
    src/rtfimp.cpp
    src/cri18n.cpp
    src/lvmemman.cpp
    src/lvstyles.cpp
    src/crtxtenc.cpp
    src/lvtinydom.cpp
    src/lvstsheet.cpp
    src/txtselector.cpp
    src/crtest.cpp
    src/textlang.cpp
    src/lvstream/lvdefstreambuffer.cpp
    src/lvstream/lvfilemappedstream.cpp
    src/lvstream/lvnamedstream.cpp
    src/lvstream/lvfilestream.cpp
    src/lvstream/lvdirectorycontainer.cpp
    src/lvstream/lvcachedstream.cpp
    src/lvstream/lvzipdecodestream.cpp
    src/lvstream/lvziparc.cpp
    src/lvstream/lvrararc.cpp
    src/lvstream/lvmemorystream.cpp
    src/lvstream/lvtcrstream.cpp
    src/lvstream/lvblockwritestream.cpp
    src/lvstream/lvstream.cpp
    src/lvstream/lvbase64stream.cpp
    src/lvstream/lvstreamutils.cpp
    src/lvxml/lvfileparserbase.cpp
    src/lvxml/lvtextfilebase.cpp
    src/lvxml/lvtextparser.cpp
    src/lvxml/lvtextlinequeue.cpp
    src/lvxml/pmltextimport.cpp
    src/lvxml/lvtextrobustparser.cpp
    src/lvxml/lvtextbookmarkparser.cpp
    src/lvxml/lvxmlparser.cpp
    src/lvxml/lvhtmlparser.cpp
    src/lvxml/fb2coverpageparsercallback.cpp
    src/lvxml/lvxmlutils.cpp
    src/lvbmpbuf.cpp
    src/hyphman.cpp
    src/crgui.cpp
    src/crskin.cpp
    src/lvdocview.cpp
    src/lvpagesplitter.cpp
    src/lvtextfm.cpp
    src/lvrend.cpp
    src/wolutil.cpp
    src/hist.cpp
    src/chmfmt.cpp
    src/epubfmt.cpp
    src/pdbfmt.cpp
    src/wordfmt.cpp
    src/lvopc.cpp
    src/docxfmt.cpp
    src/fb3fmt.cpp
    src/odtfmt.cpp
    src/odxutil.cpp
    src/crconcurrent.cpp
    src/mathml.cpp
    src/lvimg/lvcacheableobject.cpp
    src/lvimg/lvimagesource.cpp
    src/lvimg/crninepatchdecoder.cpp
    src/lvimg/lvjpegimagesource.cpp
    src/lvimg/lvpngimagesource.cpp
    src/lvimg/lvxpmimagesource.cpp
    src/lvimg/lvgifimagesource.cpp
    src/lvimg/lvgifframe.cpp
    src/lvimg/clzwdecoder.cpp
    src/lvimg/lvsvgimagesource.cpp
    src/lvimg/lvstretchimgsource.cpp
    src/lvimg/lvcolortransformimgsource.cpp
    src/lvimg/lvalphatransformimgsource.cpp
    src/lvimg/lvunpackedimgsource.cpp
    src/lvimg/lvdrawbufimgsource.cpp
    src/lvimg/lvimg.cpp
    src/lvfont/lvfont.cpp
    src/lvfont/lvfntman.cpp
    src/lvfont/lvfnt.cpp
    src/lvfont/lvembeddedfont.cpp
    src/lvfont/lvbasefont.cpp
    src/lvfont/lvbitmapfont.cpp
    src/lvfont/lvbitmapfontman.cpp
    src/lvfont/lvfontglyphcache.cpp
    src/lvfont/lvfontcache.cpp
    src/lvfont/lvfontboldtransform.cpp
    src/lvfont/lvfontdef.cpp
    src/lvfont/lvwin32font.cpp
    src/lvfont/lvwin32fontman.cpp
    src/lvfont/lvfreetypeface.cpp
    src/lvfont/lvfreetypefontman.cpp
    src/lvdrawbuf/lvbasedrawbuf.cpp
    src/lvdrawbuf/lvgraydrawbuf.cpp
    src/lvdrawbuf/lvcolordrawbuf.cpp
    src/lvdrawbuf/lvinkmeasurementdrawbuf.cpp
    src/lvdrawbuf/lvimagescaleddrawcallback.cpp
    src/lvdrawbuf/lvdrawbuf_utils.cpp
)

if(USE_LOCALE_DATA)
  list(APPEND CRENGINE_NG_SOURCES
      src/locale_data/fc-lang-data.c
      src/locale_data/crlocaledata.cpp)
endif()

set(PKG_CONFIG_REQUIRED_PRIVATE)
set(PKG_CONFIG_LIBS_PRIVATE)

if (USE_ZLIB)
  list(APPEND PKG_CONFIG_REQUIRED_PRIVATE "zlib")
endif ()
if (USE_LIBPNG)
  list(APPEND PKG_CONFIG_REQUIRED_PRIVATE "libpng")
endif ()
if (USE_LIBJPEG)
  list(APPEND PKG_CONFIG_REQUIRED_PRIVATE "libjpeg")
endif()
if (USE_FREETYPE)
  list(APPEND PKG_CONFIG_REQUIRED_PRIVATE "freetype2 >= ${FREETYPE_MIN_VERSION}")
endif()
if (USE_HARFBUZZ)
  list(APPEND PKG_CONFIG_REQUIRED_PRIVATE "harfbuzz >= ${HARFBUZZ_MIN_VERSION}")
endif()
if (USE_FRIBIDI)
  list(APPEND PKG_CONFIG_REQUIRED_PRIVATE "fribidi >= ${FRIBIDI_MIN_VERSION}")
endif()
if (USE_LIBUNIBREAK)
  list(APPEND PKG_CONFIG_REQUIRED_PRIVATE "libunibreak >= ${LIBUNIBREAK_MIN_VERSION}")
endif()
if (USE_ZSTD)
  list(APPEND PKG_CONFIG_REQUIRED_PRIVATE "libzstd >= ${ZSTD_MIN_VERSION}")
endif()
if (USE_UTF8PROC)
  list(APPEND PKG_CONFIG_REQUIRED_PRIVATE "libutf8proc")
endif()
if (USE_FONTCONFIG)
  list(APPEND PKG_CONFIG_REQUIRED_PRIVATE "fontconfig >= ${FONTCONFIG_MIN_VERSION}")
endif()


set(cre_install_libs)
if (CRE_BUILD_SHARED)
  add_library(crengine-ng SHARED ${CRENGINE_NG_SOURCES})
  set_target_properties(crengine-ng PROPERTIES OUTPUT_NAME crengine-ng)
  target_link_libraries(crengine-ng ${CRENGINE_NG_LIBS})
  list(APPEND cre_install_libs crengine-ng)
endif (CRE_BUILD_SHARED)
if (CRE_BUILD_STATIC)
  add_library(crengine-ng_static STATIC ${CRENGINE_NG_SOURCES})
  if(MSVC)
    set_target_properties(crengine-ng_static PROPERTIES OUTPUT_NAME crengine-ng_static)
  else()
    set_target_properties(crengine-ng_static PROPERTIES OUTPUT_NAME crengine-ng)
  endif()
  target_link_libraries(crengine-ng_static ${CRENGINE_NG_LIBS})
  list(APPEND cre_install_libs crengine-ng_static)
endif (CRE_BUILD_STATIC)
if (USE_CHM)
  list(APPEND cre_install_libs ${CHM_LIBRARIES})
endif()
if (USE_ANTIWORD)
  list(APPEND cre_install_libs ${ANTIWORD_LIBRARIES})
endif()
if (USE_UNRAR)
  list(APPEND cre_install_libs ${UNRAR_LIBRARIES})
endif()
list(APPEND cre_install_libs qimagescale)

if(BUILD_TOOLS)
  add_subdirectory(Tools)
endif(BUILD_TOOLS)

# Installation
include(GNUInstallDirs)

string(REPLACE ";" ", " PKG_CONFIG_REQUIRED_PRIVATE "${PKG_CONFIG_REQUIRED_PRIVATE}")

if(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL)
  install(
    # Note the trailing slash in the argument to `DIRECTORY'!
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/crengine-ng
      COMPONENT headers)
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/crengine-ng-config.h
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/crengine-ng
      COMPONENT headers)
endif()

if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL)
  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/crengine-ng-config-version.cmake
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    COMPATIBILITY SameMajorVersion)

  install(TARGETS ${cre_install_libs}
    EXPORT crengine-ng-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FRAMEWORK DESTINATION Library/Frameworks
    COMPONENT libraries)
  install(
    EXPORT crengine-ng-targets
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/crengine-ng
      FILE crengine-ng-config.cmake
      COMPONENT headers)
  install(
    FILES ${PROJECT_BINARY_DIR}/crengine-ng-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/crengine-ng
    COMPONENT headers)

  configure_file(crengine-ng.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/crengine-ng.pc @ONLY)
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/crengine-ng.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    COMPONENT pkgconfig)
endif()

if(NOT SKIP_INSTALL_DATA AND NOT SKIP_INSTALL_ALL)
    install(
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/css/
        DESTINATION ${CRE_NG_DATADIR}
        COMPONENT css)
    install(
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/hyph/
        DESTINATION ${CRE_NG_DATADIR}/hyph
        COMPONENT css)
endif()
