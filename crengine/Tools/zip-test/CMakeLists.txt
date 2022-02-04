
set(SRC_LIST
    main.cpp
)

set(crengine_part_SRC_LIST
    ../../src/cp_stats.cpp
    ../../src/crtxtenc.cpp
    ../../src/lvmemman.cpp
    ../../src/lvstream/lvdefstreambuffer.cpp
    ../../src/lvstream/lvfilemappedstream.cpp
    ../../src/lvstream/lvnamedstream.cpp
    ../../src/lvstream/lvfilestream.cpp
    ../../src/lvstream/lvstream.cpp
    ../../src/lvstream/lvziparc.cpp
    ../../src/lvstream/lvzipdecodestream.cpp
    ../../src/lvstream/lvmemorystream.cpp
    ../../src/lvstream/lvcachedstream.cpp
    ../../src/lvstream/lvtcrstream.cpp
    ../../src/lvstream/lvdirectorycontainer.cpp
    ../../src/lvstream/lvblockwritestream.cpp
    ../../src/lvstream/lvstreamutils.cpp
    ../../src/lvstring.cpp
    ../../src/lvstring32collection.cpp
    ../../src/crlog.cpp
    ../../src/lvserialbuf.cpp
)

add_definitions(-D_LARGEFILE64_SOURCE=1 -D_FILE_OFFSET_BITS=64)
add_definitions(-DLVLONG_FILE_SUPPORT=1)

if(UNIX)
    add_definitions(-DLINUX -D_LINUX)
endif(UNIX)

if(WIN32)
    add_definitions(-DWIN32 -D_CONSOLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mconsole")
endif(WIN32)

add_executable(zip-test ${SRC_LIST} ${crengine_part_SRC_LIST})
target_include_directories(zip-test PRIVATE ${UTF8PROC_INCLUDE_DIRS})
target_link_libraries(zip-test ${ZLIB_LIBRARIES} ${UTF8PROC_LIBRARIES})
