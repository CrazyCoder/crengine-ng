
if(UNIX)
  if(NOT MACOS)
    add_subdirectory(Fb2Linux)
  endif(NOT MACOS)
endif(UNIX)

add_subdirectory(HyphConv)
add_subdirectory(langstat)
add_subdirectory(langstat2)
add_subdirectory(glyphcache_bench)
add_subdirectory(HyphDumper)
add_subdirectory(blend-algo-test)
add_subdirectory(zip-test)
