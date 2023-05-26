
set(SRC_LIST
    main.cpp
    lvfontglyphcache_a.cpp
    lvfontglyphcache_b.cpp
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

add_executable(glyphcache_bench ${SRC_LIST})
target_link_libraries(glyphcache_bench ${CRE_NG})
if (CRE_BUILD_STATIC)
    target_include_directories(glyphcache_bench PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES})
endif()

configure_file(valgrind_check.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/valgrind_check.sh)
