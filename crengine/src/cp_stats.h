/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007 Vadim Lopatin <coolreader.org@gmail.com>           *
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

#ifndef CP_STAT_H
#define CP_STAT_H

typedef struct
{
    unsigned char ch1;
    unsigned char ch2;
    short count;
} dbl_char_stat_t;

typedef struct
{
    unsigned char ch1;
    unsigned char ch2;
    int count;
} dbl_char_stat_long_t;

typedef struct
{
    const short* ch_stat;
    const dbl_char_stat_t* dbl_ch_stat;
    const char* cp_name;
    const char* lang_name;
} cp_stat_t;

#endif // CP_STAT_H
