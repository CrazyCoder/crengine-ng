# - Try to find the WebP (fallback module)
# Once done this will define
#
#  WebPAlt_FOUND - system has webp
#  WebP_FOUND - system has webp
#  WebP_INCLUDE_DIR - The include directory to use for the webp headers
#  WebP_LIBRARIES - Link these to use webp
#  WebP_DEFINITIONS - Compiler switches required for using webp

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
  WebP_INCLUDE_DIR
  WebP_LIBRARY
  WebPDemux_LIBRARY
)

# Append WEBP_ROOT or $ENV{WEBP_ROOT} if set (prioritize the direct cmake var)
set(_WEBP_ROOT_SEARCH_DIR "")

if(WEBP_ROOT)
  list(APPEND _WEBP_ROOT_SEARCH_DIR ${WEBP_ROOT})
else()
  set(_ENV_WEBP_ROOT $ENV{WEBP_ROOT})
  if(_ENV_WEBP_ROOT)
    list(APPEND _WEBP_ROOT_SEARCH_DIR ${_ENV_WEBP_ROOT})
  endif()
endif()

# Additionally try and use pkconfig to find libwebp

find_package(PkgConfig QUIET)
pkg_check_modules(PC_WEBP QUIET libwebp)

# ------------------------------------------------------------------------
#  Search for webp include DIR
# ------------------------------------------------------------------------

set(_WEBP_INCLUDE_SEARCH_DIRS "")
list(APPEND _WEBP_INCLUDE_SEARCH_DIRS
  ${WEBP_INCLUDEDIR}
  ${_WEBP_ROOT_SEARCH_DIR}
  ${PC_WEBP_INCLUDE_DIRS}
  ${SYSTEM_LIBRARY_PATHS}
)

# Look for a standard webp header file.
find_path(WebP_INCLUDE_DIR "webp/decode.h"
  PATHS ${_WEBP_INCLUDE_SEARCH_DIRS}
  PATH_SUFFIXES include
)

# ------------------------------------------------------------------------
#  Search for webp lib DIR
# ------------------------------------------------------------------------

set(_WEBP_LIBRARYDIR_SEARCH_DIRS "")
list(APPEND _WEBP_LIBRARYDIR_SEARCH_DIRS
  ${WEBP_LIBRARYDIR}
  ${_WEBP_ROOT_SEARCH_DIR}
  ${PC_WEBP_LIBRARY_DIRS}
  ${SYSTEM_LIBRARY_PATHS}
)

# Static library setup
if(UNIX AND WebP_USE_STATIC_LIBS)
  set(_WEBP_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
endif()

set(WEBP_PATH_SUFFIXES
  lib
  lib64
)

find_library(WebP_LIBRARY webp
  PATHS ${_WEBP_LIBRARYDIR_SEARCH_DIRS}
  PATH_SUFFIXES ${WEBP_PATH_SUFFIXES}
)

find_library(WebPDemux_LIBRARY webpdemux
  PATHS ${_WEBP_LIBRARYDIR_SEARCH_DIRS}
  PATH_SUFFIXES ${WEBP_PATH_SUFFIXES}
)

if(UNIX AND WebP_USE_STATIC_LIBS)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${_WEBP_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
  unset(_WEBP_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES)
endif()

# ------------------------------------------------------------------------
#  Cache and set WEBP_FOUND
# ------------------------------------------------------------------------

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WebPAlt
  FOUND_VAR WebPAlt_FOUND
  REQUIRED_VARS
    WebP_LIBRARY
    WebPDemux_LIBRARY
    WebP_INCLUDE_DIR
  VERSION_VAR WEBP_VERSION
)

if(WebPAlt_FOUND)
  set(WebP_FOUND TRUE)
  set(WebP_LIBRARIES)
  list(APPEND WebP_LIBRARIES ${WebP_LIBRARY})
  list(APPEND WebP_LIBRARIES ${WebPDemux_LIBRARY})
  set(WebP_INCLUDE_DIRS ${WebP_INCLUDE_DIR})
  set(WebP_DEFINITIONS ${PC_WEBP_CFLAGS_OTHER})

  get_filename_component(WebP_LIBRARY_DIRS ${WebP_LIBRARY} DIRECTORY)

  if(NOT TARGET WebP::webp)
    add_library(WebP::webp UNKNOWN IMPORTED)
    set_target_properties(WebP::webp PROPERTIES
      IMPORTED_LOCATION "${WebP_LIBRARY}"
      INTERFACE_COMPILE_DEFINITIONS "${WebP_DEFINITIONS}"
      INTERFACE_INCLUDE_DIRECTORIES "${WebP_INCLUDE_DIRS}"
    )
  endif()
  if(NOT TARGET WebP::webpdemux)
    add_library(WebP::webpdemux UNKNOWN IMPORTED)
    set_target_properties(WebP::webpdemux PROPERTIES
            IMPORTED_LOCATION "${WebPDemux_LIBRARY}"
            INTERFACE_COMPILE_DEFINITIONS "${WebP_DEFINITIONS}"
            INTERFACE_INCLUDE_DIRECTORIES "${WebP_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "WebP::webp"
    )
  endif()
endif()
