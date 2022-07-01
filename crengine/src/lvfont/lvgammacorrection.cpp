/***************************************************************************
 *   coolreader-ng, gamma correction of font glyph images                  *
 *   Copyright © 2022 by coolreader-ng authors                             *
 *   Copyright © Vadim Lopatin, 2011                                       *
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

#include "lvgammacorrection.h"
#include "lvgammatbl.h"

const int LVGammaCorrection::NoCorrectionIndex = GAMMA_NO_CORRECTION_INDEX;

static inline float my_fabs(float value) {
    if (value < 0.0f)
        return -value;
    return value;
}

int LVGammaCorrection::getIndex(float gamma) {
    int index = 0;
    float min = my_fabs(lvgammatbl_data[0].gamma - gamma);
    for (int i = 1; i < GAMMA_LEVELS; i++) {
        float diff = my_fabs(lvgammatbl_data[i].gamma - gamma);
        if (diff < min) {
            min = diff;
            index = i;
        }
    }
    return index;
}

float LVGammaCorrection::getValue(int index) {
    if (index < 0)
        return lvgammatbl_data[0].gamma;
    if (index >= GAMMA_LEVELS)
        return lvgammatbl_data[GAMMA_LEVELS - 1].gamma;
    return lvgammatbl_data[index].gamma;
}

void LVGammaCorrection::gammaCorrection(unsigned char* buf, int buf_sz, int gamma_index) {
    if (gamma_index < 0 || gamma_index >= GAMMA_LEVELS)
        return;
    const unsigned char* table = lvgammatbl_data[gamma_index].table;
    for (int i = 0; i < buf_sz; i++)
        buf[i] = table[buf[i]];
}
