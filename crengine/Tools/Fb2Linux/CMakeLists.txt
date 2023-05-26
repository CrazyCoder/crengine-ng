
if(NOT UNIX OR MACOS)
    message(FATAL_ERROR "This program available only for Unix/Linux!")
endif()

find_package(X11 REQUIRED)

include_directories(${X11_INCLUDE_DIR})

set(SRC_LIST
    fb2v.cpp
    xutils.cpp
)

file(COPY "${CMAKE_SOURCE_DIR}/crengine/data/css/fb2.css" DESTINATION ${CMAKE_CURRENT_BINARY_DIR} USE_SOURCE_PERMISSIONS)
file(COPY example.fb2 DESTINATION ${CMAKE_CURRENT_BINARY_DIR} USE_SOURCE_PERMISSIONS)
file(COPY "${CMAKE_SOURCE_DIR}/crengine/data/hyph/hyph-ru-ru,en-us.pattern" DESTINATION ${CMAKE_CURRENT_BINARY_DIR} USE_SOURCE_PERMISSIONS)

set(CRE_NG)
if (CRE_BUILD_STATIC)
    set(CRE_NG crengine-ng_static)
elseif(CRE_BUILD_SHARED)
    set(CRE_NG crengine-ng)
endif()

add_executable(fb2view ${SRC_LIST})
target_link_libraries(fb2view ${CRE_NG} ${X11_LIBRARIES})
if (CRE_BUILD_STATIC)
    target_include_directories(fb2view PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES})
endif()
