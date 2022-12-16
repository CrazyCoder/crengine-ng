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
 * \file make_rand_file.h
 * \brief Create file with random contents.
 */

#ifndef _MAKE_RAND_FILE_H
#define _MAKE_RAND_FILE_H

#include <stdint.h>

namespace crengine_ng
{

namespace unittesting
{

bool makeRandomFile(const char* fname, bool text_mode, uint64_t size);

} // namespace unittesting

} // namespace crengine_ng

#endif // _MAKE_RAND_FILE_H
