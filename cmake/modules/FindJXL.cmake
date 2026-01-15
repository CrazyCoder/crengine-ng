# - Try to find the libjxl
# Once done this will define
#
#  JXL_FOUND - system has libjxl
#  JXL_INCLUDE_DIR - The include directory to use for the libjxl headers
#  JXL_LIBRARIES - Link these to use libjxl
#  JXL_DEFINITIONS - Compiler switches required for using libjxl

# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

mark_as_advanced(
  JXL_INCLUDE_DIR
  JXL_LIBRARY
)

# Append JXL_ROOT or $ENV{JXL_ROOT} if set (prioritize the direct cmake var)
set(_JXL_ROOT_SEARCH_DIR "")

if(JXL_ROOT)
  list(APPEND _JXL_ROOT_SEARCH_DIR ${JXL_ROOT})
else()
  set(_ENV_JXL_ROOT $ENV{JXL_ROOT})
  if(_ENV_JXL_ROOT)
    list(APPEND _JXL_ROOT_SEARCH_DIR ${_ENV_JXL_ROOT})
  endif()
endif()

# Additionally try and use pkconfig to find libjxl
find_package(PkgConfig QUIET)
pkg_check_modules(PC_LIBJXL QUIET IMPORTED_TARGET libjxl libjxl_cms libjxl_threads)

# ------------------------------------------------------------------------
#  Search for libjxl include DIR
# ------------------------------------------------------------------------

set(_JXL_INCLUDE_SEARCH_DIRS "")
list(APPEND _JXL_INCLUDE_SEARCH_DIRS
  ${JXL_INCLUDEDIR}
  ${_JXL_ROOT_SEARCH_DIR}
  ${PC_LIBJXL_INCLUDE_DIRS}
  ${SYSTEM_LIBRARY_PATHS}
)

# Look for a standard libjxl header file.
find_path(JXL_INCLUDE_DIR jxl/decode.h
  PATHS ${_JXL_INCLUDE_SEARCH_DIRS}
  PATH_SUFFIXES include
)

if(EXISTS "${JXL_INCLUDE_DIR}/jxl/version.h")
  file(STRINGS "${JXL_INCLUDE_DIR}/jxl/version.h" JPEGXL_VERSION_MAJOR_LINE REGEX "^#define[ \t]+JPEGXL_MAJOR_VERSION[ \t]+[0-9]+.*$")
  file(STRINGS "${JXL_INCLUDE_DIR}/jxl/version.h" JPEGXL_VERSION_MINOR_LINE REGEX "^#define[ \t]+JPEGXL_MINOR_VERSION[ \t]+[0-9]+.*$")
  file(STRINGS "${JXL_INCLUDE_DIR}/jxl/version.h" JPEGXL_VERSION_PATCH_LINE REGEX "^#define[ \t]+JPEGXL_PATCH_VERSION[ \t]+[0-9]+.*$")
  string(REGEX REPLACE "^#define[ \t]+JPEGXL_MAJOR_VERSION[ \t]+([0-9]+).*$" "\\1" JPEGXL_VERSION_MAJOR "${JPEGXL_VERSION_MAJOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+JPEGXL_MINOR_VERSION[ \t]+([0-9]+).*$" "\\1" JPEGXL_VERSION_MINOR "${JPEGXL_VERSION_MINOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+JPEGXL_PATCH_VERSION[ \t]+([0-9]+).*$" "\\1" JPEGXL_VERSION_PATCH "${JPEGXL_VERSION_PATCH_LINE}")
  set(JXL_VERSION ${JPEGXL_VERSION_MAJOR}.${JPEGXL_VERSION_MINOR}.${JPEGXL_VERSION_PATCH})
endif()

# ------------------------------------------------------------------------
#  Search for libjxl lib DIR
# ------------------------------------------------------------------------
set(JXL_LIBRARY_DEPS)
set(JXL_CMS_LIBRARY_DEPS)

set(_JXL_LIBRARYDIR_SEARCH_DIRS "")
list(APPEND _JXL_LIBRARYDIR_SEARCH_DIRS
  ${JXL_LIBRARYDIR}
  ${_JXL_ROOT_SEARCH_DIR}
  ${PC_LIBJXL_LIBRARY_DIRS}
  ${SYSTEM_LIBRARY_PATHS}
)

# Static library setup
if(UNIX AND JXL_USE_STATIC_LIBS)
  set(_JXL_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
endif()

set(JXL_PATH_SUFFIXES
  lib
  lib64
)

find_library(JXL_LIBRARY jxl
  PATHS ${_JXL_LIBRARYDIR_SEARCH_DIRS}
  PATH_SUFFIXES ${JXL_PATH_SUFFIXES}
)
find_library(JXL_THREADS_LIBRARY jxl_threads
        PATHS ${_JXL_LIBRARYDIR_SEARCH_DIRS}
        PATH_SUFFIXES ${JXL_PATH_SUFFIXES}
)
find_library(JXL_CMS_LIBRARY jxl_cms
        PATHS ${_JXL_LIBRARYDIR_SEARCH_DIRS}
        PATH_SUFFIXES ${JXL_PATH_SUFFIXES}
)

if (JXL_USE_STATIC_LIBS)
  # Locate static dependency libraries
  if (JXL_CMS_LIBRARY)
    find_library(LCMS2_LIBRARY lcms2
            PATHS ${_JXL_LIBRARYDIR_SEARCH_DIRS}
            PATH_SUFFIXES ${JXL_PATH_SUFFIXES}
    )
    if (LCMS2_LIBRARY)
      list(APPEND JXL_CMS_LIBRARY_DEPS ${LCMS2_LIBRARY})
    endif()
  endif()
  if (JXL_LIBRARY)
    find_library(BROTLICOMMON_LIBRARY brotlicommon
            PATHS ${_JXL_LIBRARYDIR_SEARCH_DIRS}
            PATH_SUFFIXES ${JXL_PATH_SUFFIXES}
    )
    if (BROTLICOMMON_LIBRARY)
      list(APPEND JXL_LIBRARY_DEPS ${BROTLICOMMON_LIBRARY})
    endif()
    find_library(BROTLIENC_LIBRARY brotlienc
            PATHS ${_JXL_LIBRARYDIR_SEARCH_DIRS}
            PATH_SUFFIXES ${JXL_PATH_SUFFIXES}
    )
    if (BROTLIENC_LIBRARY)
      list(APPEND JXL_LIBRARY_DEPS ${BROTLIENC_LIBRARY})
    endif()
    find_library(BROTLIDEC_LIBRARY brotlidec
            PATHS ${_JXL_LIBRARYDIR_SEARCH_DIRS}
            PATH_SUFFIXES ${JXL_PATH_SUFFIXES}
    )
    if (BROTLIDEC_LIBRARY)
      list(APPEND JXL_LIBRARY_DEPS ${BROTLIDEC_LIBRARY})
    endif()
    find_library(HWY_LIBRARY hwy
            PATHS ${_JXL_LIBRARYDIR_SEARCH_DIRS}
            PATH_SUFFIXES ${JXL_PATH_SUFFIXES}
    )
    if (HWY_LIBRARY)
      list(APPEND JXL_LIBRARY_DEPS ${HWY_LIBRARY})
    endif()
  endif()
endif()

if(UNIX AND JXL_USE_STATIC_LIBS)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${_JXL_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
  unset(_UTF8PROC_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES)
endif()

# ------------------------------------------------------------------------
#  Cache and set JXL_FOUND
# ------------------------------------------------------------------------

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JXL
  FOUND_VAR JXL_FOUND
  REQUIRED_VARS
    JXL_LIBRARY
    JXL_INCLUDE_DIR
  VERSION_VAR JXL_VERSION
)

if(JXL_FOUND)
  set(JXL_LIBRARIES)
  list(APPEND JXL_LIBRARIES ${JXL_LIBRARY})
  list(APPEND JXL_LIBRARIES ${JXL_LIBRARY_DEPS})
  list(APPEND JXL_LIBRARIES ${JXL_THREADS_LIBRARY})
  if (${JXL_CMS_LIBRARY})
    # Older versions of libjxl (<0.9.0) do not contain a separate libjxl_cms.so
    list(APPEND JXL_LIBRARIES ${JXL_CMS_LIBRARY})
    list(APPEND JXL_LIBRARIES ${JXL_CMS_LIBRARY_DEPS})
  endif()
  set(JXL_INCLUDE_DIRS ${JXL_INCLUDE_DIR})
  set(JXL_DEFINITIONS ${PC_LIBJXL_CFLAGS_OTHER})

  get_filename_component(JXL_LIBRARY_DIRS ${JXL_LIBRARY} DIRECTORY)

  if(NOT TARGET JXL::jxl_cms)
    add_library(JXL::jxl_cms UNKNOWN IMPORTED)
    set_target_properties(JXL::jxl_cms PROPERTIES
            IMPORTED_LOCATION "${JXL_CMS_LIBRARY}"
            INTERFACE_COMPILE_DEFINITIONS "${JXL_DEFINITIONS}"
            INTERFACE_INCLUDE_DIRECTORIES "${JXL_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${JXL_CMS_LIBRARY_DEPS}"
    )
  endif()
  if(NOT TARGET JXL::jxl)
    add_library(JXL::jxl UNKNOWN IMPORTED)
    set_target_properties(JXL::jxl PROPERTIES
            IMPORTED_LOCATION "${JXL_LIBRARY}"
            INTERFACE_COMPILE_DEFINITIONS "${JXL_DEFINITIONS}"
            INTERFACE_INCLUDE_DIRECTORIES "${JXL_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "JXL::jxl_cms;${JXL_LIBRARY_DEPS}"
    )
  endif()
  if(NOT TARGET JXL::jxl_threads)
    add_library(JXL::jxl_threads UNKNOWN IMPORTED)
    set_target_properties(JXL::jxl_threads PROPERTIES
            IMPORTED_LOCATION "${JXL_THREADS_LIBRARY}"
            INTERFACE_COMPILE_DEFINITIONS "${JXL_DEFINITIONS}"
            INTERFACE_INCLUDE_DIRECTORIES "${JXL_INCLUDE_DIRS}"
    )
  endif()
endif()
