project(blend-algo-test C)

include_directories(${CMAKE_BINARY_DIR})

check_symbol_exists(srandom "stdlib.h" HAVE_SRANDOM)
check_symbol_exists(random "stdlib.h" HAVE_RANDOM)

if(HAVE_SRANDOM)
    add_definitions(-DHAVE_SRANDOM)
endif (HAVE_SRANDOM)
if(HAVE_RANDOM)
    add_definitions(-DHAVE_RANDOM)
endif (HAVE_RANDOM)

set(BLEND_GRAY_SRC_LIST
    blend_over_gray_test.c
)

set(BLEND_RGB_SRC_LIST
    blend_over_rgb_test.c
)

add_executable(blend_over_gray_test ${BLEND_GRAY_SRC_LIST})
add_executable(blend_over_rgb_test  ${BLEND_RGB_SRC_LIST})

set(LDADD_LIBS -lm)

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mconsole")
endif(WIN32)

target_link_libraries(blend_over_gray_test ${LDADD_LIBS})
target_link_libraries(blend_over_rgb_test  ${LDADD_LIBS})
