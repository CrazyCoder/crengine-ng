prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=${prefix}
libdir=@CMAKE_INSTALL_PREFIX@/@CMAKE_INSTALL_LIBDIR@
includedir=${prefix}/@CMAKE_INSTALL_INCLUDEDIR@

Name: crengine-ng
Description: Lightweight library designed to implement text viewers and e-book readers.
Version: @VERSION@
Requires:
Requires.private: @PKG_CONFIG_REQUIRED_PRIVATE@
Libs: -L${libdir} -lcrengine-ng
Libs.private: @PKG_CONFIG_LIBS_PRIVATE@
Cflags: -I${includedir}/crengine-ng @PC_PUBLIC_COMPILE_DEFINITIONS@
