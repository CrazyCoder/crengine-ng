
set(CMARK_SOURCES
  cmark.c
  node.c
  iterator.c
  blocks.c
  inlines.c
  scanners.c
  scanners.re
  utf8.c
  buffer.c
  references.c
  render.c
  man.c
  xml.c
  html.c
  commonmark.c
  latex.c
  houdini_href_e.c
  houdini_html_e.c
  houdini_html_u.c
  cmark_ctype.c
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmark_version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmark_version.h)

include (GenerateExportHeader)

# -fvisibility=hidden
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

add_library(cre_cmark OBJECT ${CMARK_SOURCES})

generate_export_header(cre_cmark BASE_NAME cmark)

if (MSVC)
  set_property(TARGET ${PROGRAM}
    APPEND PROPERTY LINK_FLAGS /INCREMENTAL:NO)
endif(MSVC)

# Feature tests
include(CheckIncludeFile)
include(CheckCSourceCompiles)
CHECK_INCLUDE_FILE(stdbool.h HAVE_STDBOOL_H)
CHECK_C_SOURCE_COMPILES(
  "int main() { __builtin_expect(0,0); return 0; }"
  HAVE___BUILTIN_EXPECT)
CHECK_C_SOURCE_COMPILES("
  int f(void) __attribute__ (());
  int main() { return 0; }
" HAVE___ATTRIBUTE__)

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmark-config.h)
