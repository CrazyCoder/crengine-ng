# Copyright 1999-2022 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=7
inherit cmake

DESCRIPTION="Cross-platform library designed to implement e-book readers. Supports many e-book formats"
HOMEPAGE="https://gitlab.com/coolreader-ng/crengine-ng"
SRC_URI="https://gitlab.com/coolreader-ng/${PN}/-/archive/${PV}/${P}.tar.bz2"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~amd64 ~x86"

IUSE="+svg chm"

CDEPEND="sys-libs/zlib
	media-libs/libpng:0
	virtual/jpeg:0
	>=media-libs/freetype-2.10.0
	media-libs/harfbuzz:=
	dev-libs/libunibreak:=
	dev-libs/fribidi
	app-arch/zstd:=
	dev-libs/libutf8proc:=
	media-libs/fontconfig"

RDEPEND="${CDEPEND}"
DEPEND="${RDEPEND}"
BDEPEND="virtual/pkgconfig
	${CDEPEND}"

src_configure() {
	CMAKE_USE_DIR="${S}"
	CMAKE_BUILD_TYPE="Release"
	local mycmakeargs=(
		-DCRE_BUILD_SHARED=ON
		-DCRE_BUILD_STATIC=OFF
		-DUSE_COLOR_BACKBUFFER=ON
		-DWITH_LIBPNG=ON
		-DWITH_LIBJPEG=ON
		-DWITH_FREETYPE=ON
		-DWITH_HARFBUZZ=ON
		-DWITH_LIBUNIBREAK=ON
		-DWITH_FRIBIDI=ON
		-DWITH_ZSTD=ON
		-DWITH_UTF8PROC=ON
		-DUSE_NANOSVG=$(usex svg)
		-DUSE_CHM=$(usex chm)
		-DUSE_FONTCONFIG=ON
		-DUSE_SHASUM=OFF
		-DUSE_CMARK=OFF
		-DUSE_CMARK_GFM=ON
	)
	cmake_src_configure
}
