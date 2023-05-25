
set(SRC_LIST
    langstat.cpp
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

add_executable(langstat2 ${SRC_LIST})
target_link_libraries(langstat2 ${CRE_NG} ${PRIVATE_LINK_LIBRARIES})
if (CRE_BUILD_STATIC)
    target_include_directories(langstat2 PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES})
endif()
