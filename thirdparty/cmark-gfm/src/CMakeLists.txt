
set(CMARK_GFM_SOURCES
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
  footnotes.c
  map.c
  render.c
  man.c
  xml.c
  html.c
  commonmark.c
  plaintext.c
  latex.c
  houdini_href_e.c
  houdini_html_e.c
  houdini_html_u.c
  cmark_ctype.c
  arena.c
  linked_list.c
  syntax_extension.c
  registry.c
  plugin.c
)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/../extensions)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmark-gfm_version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmark-gfm_version.h)

include (GenerateExportHeader)

# -fvisibility=hidden
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

add_library(cre_cmark-gfm OBJECT ${CMARK_GFM_SOURCES})

generate_export_header(cre_cmark-gfm BASE_NAME cmark-gfm)

# Feature tests
include(CheckIncludeFile)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckSymbolExists)
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
  ${CMAKE_CURRENT_BINARY_DIR}/cmark-gfm-config.h)
