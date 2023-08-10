
set(FONTS_DIR ${CMAKE_CURRENT_BINARY_DIR}/fonts)
file(MAKE_DIRECTORY ${FONTS_DIR})

# GoogleTest 1.13+ requires at least C++14
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

if(OFFLINE_BUILD_MODE)
    find_package(GTest 1.11.0 REQUIRED)
    # Check fonts availability in ${FONTS_DIR}
    list(APPEND FREEFONT_FILES "FreeMono.otf")
    list(APPEND FREEFONT_FILES "FreeMonoBold.otf")
    list(APPEND FREEFONT_FILES "FreeMonoOblique.otf")
    list(APPEND FREEFONT_FILES "FreeMonoBoldOblique.otf")
    list(APPEND FREEFONT_FILES "FreeSans.otf")
    list(APPEND FREEFONT_FILES "FreeSansBold.otf")
    list(APPEND FREEFONT_FILES "FreeSansOblique.otf")
    list(APPEND FREEFONT_FILES "FreeSansBoldOblique.otf")
    list(APPEND FREEFONT_FILES "FreeSerif.otf")
    list(APPEND FREEFONT_FILES "FreeSerifBold.otf")
    list(APPEND FREEFONT_FILES "FreeSerifItalic.otf")
    list(APPEND FREEFONT_FILES "FreeSerifBoldItalic.otf")
    list(APPEND ALL_FONTS_FOR_TESTS ${FREEFONT_FILES})
    set(HAVE_ALL_FONTS ON)
    foreach(_FONT IN ITEMS ${ALL_FONTS_FOR_TESTS})
        #message("Checking ${_FONT}")
        if (NOT EXISTS "${FONTS_DIR}/${_FONT}")
            message("Font file ${_FONT} NOT found!")
            set(HAVE_ALL_FONTS OFF)
            break()
        endif()
    endforeach()
    if(NOT HAVE_ALL_FONTS)
        message("Not all required fonts were found in the ${FONTS_DIR} directory")
        message("For unit tests to work, please download the fonts and extract them to this directory.")
        message("1. Download FreeFont from http://ftp.gnu.org/gnu/freefont/freefont-otf-20120503.tar.gz")
        message("# wget -c http://ftp.gnu.org/gnu/freefont/freefont-otf-20120503.tar.gz")
        message("2. Extract all *.otf files and place them in the ${FONTS_DIR} directory.")
        message("# tar -xvzf freefont-otf-20120503.tar.gz -C crengine/tests/fonts/ --strip-components=1 --wildcards '*.otf'")
        message("3. Then re-call cmake")
        message(FATAL_ERROR "Emergency abort")
    endif(NOT HAVE_ALL_FONTS)
else(OFFLINE_BUILD_MODE)
    # Add googletest sources
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.tar.gz
        URL_HASH MD5=95b29f0038ec84a611df951d74d99897
    )
    FetchContent_Declare(
        freefont
        URL      http://ftp.gnu.org/gnu/freefont/freefont-otf-20120503.tar.gz
        URL_HASH MD5=e23c149a4cc494ac1f473f13cca16b67
    )

    set(BUILD_GMOCK OFF CACHE BOOL "Builds the googlemock subproject" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)" FORCE)
    FetchContent_MakeAvailable(googletest)
    # add_subdirectory() for googletest sources
    # is called automatically as cmake is also used

    FetchContent_Populate(freefont)

    file(GLOB FREEFONT_OTF_FILES ${freefont_SOURCE_DIR}/*.otf)
    file(COPY ${FREEFONT_OTF_FILES} DESTINATION ${FONTS_DIR})
endif(OFFLINE_BUILD_MODE)

find_program(ZIP_PROG NAMES zip)
if(NOT EXISTS "${ZIP_PROG}")
    message(FATAL_ERROR "zip program NOT found!")
endif()
add_definitions(-DZIP_PROG=\"${ZIP_PROG}\")

add_definitions(-DTESTS_DATADIR=\"${CMAKE_CURRENT_SOURCE_DIR}/data/\")
add_definitions(-DTESTS_TMPDIR=\"${CMAKE_CURRENT_BINARY_DIR}/\")
add_definitions(-DHYPH_DIR=\"${CMAKE_SOURCE_DIR}/crengine/data/hyph/\")
add_definitions(-DCSS_DIR=\"${CMAKE_SOURCE_DIR}/crengine/data/css/\")
add_definitions(-DFONTS_DIR=\"${FONTS_DIR}/\")
add_definitions(-DRENDER_REFERENCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/render-references/\")
add_definitions(-DDOCPARSE_REFERENCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/docparse-references/\")
add_definitions(-DPROPS_REFERENCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/props-references/\")

# Copy from the source directory to the working directory to test the opening of the document by relative path
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/data/hello_fb2.fb2" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(RENAME "${CMAKE_CURRENT_BINARY_DIR}/hello_fb2.fb2" "${CMAKE_CURRENT_BINARY_DIR}/hello_fb2_rel.fb2")

set(SRC_LIST
    savetobmp.cpp
    calc-drawbuf-diff.cpp
    compare-two-textfiles.cpp
    compare-two-binfiles.cpp
    make-rand-file.cpp
    crengine-ng-environment.cpp
    tests_blockwritecache.cpp
    tests_tinydom.cpp
    tests_textrender.cpp
    tests_wtf8.cpp
    tests_docparse.cpp
    tests_docview_funcs.cpp
    tests_zip-use.cpp
    tests_localenames.cpp
    tests_hyph.cpp
    tests_fontman_funcs.cpp
    tests_streamutils.cpp
    tests_bm_title.cpp
    tests_props.cpp
    tests_formatting.cpp
    tests_doc_props.cpp
)

set(CRE_NG)
if (CRE_BUILD_STATIC)
    set(CRE_NG crengine-ng_static)
elseif(CRE_BUILD_SHARED)
    set(CRE_NG crengine-ng)
endif()

if(WIN32)
    add_definitions(-DWIN32 -D_CONSOLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mconsole")
endif(WIN32)

add_executable(unittests ${SRC_LIST})
target_link_libraries(unittests ${CRE_NG} GTest::gtest_main)
target_include_directories(unittests PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES})

include(GoogleTest)
gtest_discover_tests(unittests)
