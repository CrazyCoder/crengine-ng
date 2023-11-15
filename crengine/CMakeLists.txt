
set(CRENGINE_NG_SOURCES
    src/cp_stats.cpp
    src/lvstring.cpp
    src/lvstring8collection.cpp
    src/lvstring32collection.cpp
    src/lvstring32hashedcollection.cpp
    src/lvserialbuf.cpp
    src/crlog.cpp
    src/crprops.cpp
    src/rtfimp.cpp
    src/cri18n.cpp
    src/lvmemman.cpp
    src/lvstyles.cpp
    src/crtxtenc.cpp
    src/lvstsheet.cpp
    src/textlang.cpp
    src/lvstream/lvdefstreambuffer.cpp
    src/lvstream/lvfilemappedstream.cpp
    src/lvstream/lvnamedstream.cpp
    src/lvstream/lvfilestream.cpp
    src/lvstream/lvdirectorycontainer.cpp
    src/lvstream/lvcachedstream.cpp
    src/lvstream/lvzipdecodestream.cpp
    src/lvstream/lvziparc.cpp
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
    src/lvxml/lvtextbookmarkparser.cpp
    src/lvxml/lvxmlparser.cpp
    src/lvxml/lvhtmlparser.cpp
    src/lvxml/fb2coverpageparsercallback.cpp
    src/lvxml/lvxmlutils.cpp
    src/crhyphman.cpp
    src/crskin.cpp
    src/lvdocview.cpp
    src/lvpagesplitter.cpp
    src/lvtextfm.cpp
    src/lvrend.cpp
    src/wolutil.cpp
    src/crhist.cpp
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
    src/lvfont/lvgammacorrection.cpp
    src/lvfont/lvgammatbl.c
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
    src/lvtinydom/lvtinydomutils.cpp
    src/lvtinydom/ldomdatastoragemanager.cpp
    src/lvtinydom/ldomtextstoragechunk.cpp
    src/lvtinydom/cachefile.cpp
    src/lvtinydom/cachefileheader.cpp
    src/lvtinydom/lvtinynodecollection.cpp
    src/lvtinydom/ldomblobcache.cpp
    src/lvtinydom/ldomnode.cpp
    src/lvtinydom/lvbase64nodestream.cpp
    src/lvtinydom/renderrectaccessor.cpp
    src/lvtinydom/lxmldocbase.cpp
    src/lvtinydom/ldomxpointer.cpp
    src/lvtinydom/ldomxpointerex.cpp
    src/lvtinydom/writenodeex.cpp
    src/lvtinydom/lvtocitem.cpp
    src/lvtinydom/lvpagemap.cpp
    src/lvtinydom/ldomxrange.cpp
    src/lvtinydom/ldomxrangelist.cpp
    src/lvtinydom/ldommarkedrange.cpp
    src/lvtinydom/ldomdocument.cpp
    src/lvtinydom/lvimportstylesheetparser.cpp
    src/lvtinydom/ldomdoccache.cpp
    src/lvtinydom/ldomdoccacheimpl.cpp
    src/lvtinydom/ldomelementwriter.cpp
    src/lvtinydom/ldomdocumentwriter.cpp
    src/lvtinydom/ldomdocumentwriterfilter.cpp
    src/lvtinydom/ldomdocumentfragmentwriter.cpp
    src/lvtinydom/ldomwordexlist.cpp
    src/lvtinydom/lstridmap.cpp
    src/mdfmt.cpp
)

if(USE_LOCALE_DATA)
    list(APPEND CRENGINE_NG_SOURCES
        src/locale_data/fc-lang-data.c
        src/locale_data/crlocaledata.cpp)
endif()

if(NOT ANDROID)
    list(APPEND CRENGINE_NG_SOURCES
        src/crgui.cpp)
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

if (WIN32)
    set(CMAKE_RC_COMPILER windres)
    # set rc syntax
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> <DEFINES> -O coff -o <OBJECT> -i <SOURCE>")
    set(CMAKE_RC_SOURCE_FILE_EXTENSIONS rc)
    set(CMAKE_RC_FLAGS "-I${CMAKE_BINARY_DIR} -I${CMAKE_CURRENT_BINARY_DIR}")
    # enable resource language for this project
    enable_language(RC)
endif(WIN32)

set(CRENGINE_NG_RC_SOURCES)
if(WIN32)
    if(CRE_BUILD_SHARED)
        configure_file(rc-config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/rc-config.h)
        set(CRENGINE_NG_RC_SOURCES crengine-ng.rc)
        set_property(SOURCE crengine-ng.rc PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/crengine-ng-config.h)
    endif(CRE_BUILD_SHARED)
endif(WIN32)

set(cre_install_libs)
if (CRE_BUILD_SHARED)
    add_library(crengine-ng SHARED ${CRENGINE_NG_SOURCES} ${CRENGINE_NG_RC_SOURCES})
    set_target_properties(crengine-ng PROPERTIES OUTPUT_NAME crengine-ng)
    set_target_properties(crengine-ng PROPERTIES VERSION ${VERSION})
    set_target_properties(crengine-ng PROPERTIES SOVERSION ${LIB_API_VERSION})
    target_compile_definitions(crengine-ng PUBLIC ${PUBLIC_COMPILE_DEFINITIONS} PRIVATE ${PRIVATE_COMPILE_DEFINITIONS})
    target_include_directories(crengine-ng PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES})
    target_link_libraries(crengine-ng PUBLIC ${PUBLIC_LINK_LIBRARIES} PRIVATE ${PRIVATE_LINK_LIBRARIES})
    list(APPEND cre_install_libs crengine-ng)
    if(MACOS)
        set_target_properties(crengine-ng PROPERTIES FRAMEWORK TRUE FRAMEWORK_VERSION A)
    endif()
endif (CRE_BUILD_SHARED)
if (CRE_BUILD_STATIC)
    add_library(crengine-ng_static STATIC ${CRENGINE_NG_SOURCES})
    if(MSVC)
        set_target_properties(crengine-ng_static PROPERTIES OUTPUT_NAME crengine-ng_static)
    else()
        set_target_properties(crengine-ng_static PROPERTIES OUTPUT_NAME crengine-ng)
    endif()
    target_compile_definitions(crengine-ng_static PUBLIC ${PUBLIC_COMPILE_DEFINITIONS} PRIVATE ${PRIVATE_COMPILE_DEFINITIONS})
    target_include_directories(crengine-ng_static PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES})
    target_link_libraries(crengine-ng_static PUBLIC ${PUBLIC_LINK_LIBRARIES} PRIVATE ${PRIVATE_LINK_LIBRARIES})
    list(APPEND cre_install_libs crengine-ng_static)
endif (CRE_BUILD_STATIC)
if (USE_CHM)
    list(APPEND cre_install_libs cre_chmlib)
endif()
if (USE_ANTIWORD)
    list(APPEND cre_install_libs cre_antiword)
endif()
list(APPEND cre_install_libs cre_qimagescale)
if (USE_SHASUM)
    list(APPEND cre_install_libs cre_rfc6234-shas)
endif()
if (USE_CMARK_GFM)
    list(APPEND cre_install_libs cre_cmark-gfm)
    list(APPEND cre_install_libs cre_cmark-gfm-extensions)
endif()

if(BUILD_TOOLS)
    add_subdirectory(Tools)
endif(BUILD_TOOLS)

if(ENABLE_UNITTESTING)
    add_subdirectory(tests)
endif(ENABLE_UNITTESTING)

# Installation
include(GNUInstallDirs)

string(REPLACE ";" ", " PKG_CONFIG_REQUIRED_PRIVATE "${PKG_CONFIG_REQUIRED_PRIVATE}")

set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}/crengine-ng)
set(DATA_INSTALL_DIR ${CMAKE_INSTALL_DATADIR}/crengine-ng)
if(MSVC)
    set(DEF_CMAKE_INSTALL_DIR CMake)
else()
    set(DEF_CMAKE_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/crengine-ng)
endif()
set(CMAKE_INSTALL_DIR ${DEF_CMAKE_INSTALL_DIR} CACHE PATH "Installation directory for CMake files")

set(CRE_NG_VERSION ${VERSION})
set(CRE_NG_DATADIR ${CMAKE_INSTALL_PREFIX}/${DATA_INSTALL_DIR}/)
configure_file(crengine-ng-config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/crengine-ng-config.h)

if(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL)
    install(
        # Note the trailing slash in the argument to `DIRECTORY'!
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION ${INCLUDE_INSTALL_DIR}
        COMPONENT headers)
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/crengine-ng-config.h
        DESTINATION ${INCLUDE_INSTALL_DIR}
        COMPONENT headers)
endif()

if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL)
    if (WIN32)
        install(TARGETS ${cre_install_libs}
            EXPORT crengine-ng-targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION .
            COMPONENT libraries)
    else()
        install(TARGETS ${cre_install_libs}
            EXPORT crengine-ng-targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            FRAMEWORK DESTINATION Library/Frameworks
            COMPONENT libraries)
    endif()

    install(EXPORT crengine-ng-targets
        FILE crengine-ng-targets.cmake
        NAMESPACE crengine-ng::
        DESTINATION ${CMAKE_INSTALL_DIR}
        COMPONENT headers)

    include(CMakePackageConfigHelpers)

    configure_package_config_file(crengine-ng-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/crengine-ng-config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_DIR}
        PATH_VARS INCLUDE_INSTALL_DIR DATA_INSTALL_DIR)
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/crengine-ng-config-version.cmake
        VERSION ${VERSION}
        COMPATIBILITY SameMajorVersion)

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/crengine-ng-config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/crengine-ng-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_DIR}
        COMPONENT headers)

    string(REPLACE ";" " " PC_PUBLIC_COMPILE_DEFINITIONS "${PUBLIC_COMPILE_DEFINITIONS}")
    configure_file(crengine-ng.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/crengine-ng.pc @ONLY)
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/crengine-ng.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
        COMPONENT pkgconfig)
endif()

if(NOT SKIP_INSTALL_DATA AND NOT SKIP_INSTALL_ALL)
    if (WIN32)
        install(
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/css/
            DESTINATION .
            COMPONENT css
            FILES_MATCHING PATTERN "*.css")
        install(
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/hyph/
            DESTINATION hyph
            COMPONENT hyph
            FILES_MATCHING PATTERN "*.pattern")
    else()
        install(
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/css/
            DESTINATION ${DATA_INSTALL_DIR}
            COMPONENT css
            FILES_MATCHING PATTERN "*.css")
        install(
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/hyph/
            DESTINATION ${DATA_INSTALL_DIR}/hyph
            COMPONENT hyph
            FILES_MATCHING PATTERN "*.pattern")
    endif()
endif()
