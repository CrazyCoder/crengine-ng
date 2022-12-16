/***************************************************************************
 *   crengine-ng, unit testing                                             *
 *   Copyright (C) 2022 Aleksey Chernov <valexlin@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

/**
 * \file compare-two-binfiles.h
 * \brief Comparison of two binary files.
 * 
 * One or both files may be a gzip archive.
 * In this case, the contents of the archive are compared.
 */

#ifndef _COMPARE_TWO_BINFILES_H
#define _COMPARE_TWO_BINFILES_H

class LVDrawBuf;

namespace crengine_ng
{

namespace unittesting
{

bool compareTwoBinaryFiles(const char* file1, const char* file2);

} // namespace unittesting

} // namespace crengine_ng

#endif // _COMPARE_TWO_BINFILES_H
