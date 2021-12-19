
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
    #src/xutils.cpp
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

if(BUILD_TOOLS)
  add_subdirectory(Tools)
endif(BUILD_TOOLS)

# Installation
include(GNUInstallDirs)

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
  install(TARGETS ${cre_install_libs}
    EXPORT crengine-ng-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FRAMEWORK DESTINATION Library/Frameworks
  )
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
