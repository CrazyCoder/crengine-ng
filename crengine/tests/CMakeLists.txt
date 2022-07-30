
# Add googletest sources
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz
  URL_HASH MD5=e8a8df240b6938bb6384155d4c37d937
)
FetchContent_Declare(
  freefont
  URL      http://ftp.gnu.org/gnu/freefont/freefont-otf-20120503.tar.gz
  URL_HASH MD5=e23c149a4cc494ac1f473f13cca16b67
)
FetchContent_Declare(
  robotofont
  URL https://github.com/googlefonts/roboto/releases/download/v2.138/roboto-android.zip
  URL_HASH MD5=3b43a5cb33196ec25e44d5fcb40219e1
)

set(BUILD_GMOCK OFF CACHE BOOL "Builds the googlemock subproject" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)" FORCE)
FetchContent_MakeAvailable(googletest)
# add_subdirectory() for googletest sources
# is called automatically as cmake is also used

FetchContent_Populate(freefont)
FetchContent_Populate(robotofont)

find_program(ZIP_PROG NAMES zip)
if(NOT EXISTS "${ZIP_PROG}")
  message(FATAL_ERROR "zip program NOT found!")
endif()
add_definitions(-DZIP_PROG=\"${ZIP_PROG}\")

set(FONTS_DIR ${CMAKE_CURRENT_BINARY_DIR}/fonts)
file(MAKE_DIRECTORY ${FONTS_DIR})
file(GLOB FREEFONT_OTF_FILES ${freefont_SOURCE_DIR}/*.otf)
file(COPY ${FREEFONT_OTF_FILES} DESTINATION ${FONTS_DIR})
# From Roboto family we use only one file Roboto-Regular.ttf for testing purposes
file(COPY ${robotofont_SOURCE_DIR}/Roboto-Regular.ttf DESTINATION ${FONTS_DIR})

add_definitions(-DTESTS_DATADIR=\"${CMAKE_CURRENT_SOURCE_DIR}/data/\")
add_definitions(-DTESTS_TMPDIR=\"${CMAKE_CURRENT_BINARY_DIR}/\")
add_definitions(-DHYPH_DIR=\"${CMAKE_SOURCE_DIR}/crengine/data/hyph/\")
add_definitions(-DCSS_DIR=\"${CMAKE_SOURCE_DIR}/crengine/data/css/\")
add_definitions(-DFONTS_DIR=\"${FONTS_DIR}/\")
add_definitions(-DRENDER_REFERENCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/render-references/\")
add_definitions(-DDOCPARSE_REFERENCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/docparse-references/\")

set(SRC_LIST
  savetobmp.cpp
  calc-drawbuf-diff.cpp
  compare-two-textfiles.cpp
  compare-two-binfiles.cpp
  make-rand-file.cpp
  crengine-ng-environment.cpp
  blockwritecache_tests.cpp
  tinydom_tests.cpp
  textrender_tests.cpp
  wtf8_tests.cpp
  docparse_tests.cpp
  docview_funcs_tests.cpp
  zip-use_tests.cpp
  localenames_tests.cpp
  hyph_tests.cpp
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
target_link_libraries(unittests ${CRE_NG} gtest_main)
if (CRE_BUILD_STATIC)
  target_include_directories(unittests PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES})
endif()

include(GoogleTest)
gtest_discover_tests(unittests)
