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

#ifndef __LVGAMMACORRECTION_H_INCLUDED__
#define __LVGAMMACORRECTION_H_INCLUDED__

class LVGammaCorrection
{
public:
    /**
     * @brief Gamma table index where no gamma correction is required, i.e. gamma = 1.0
     */
    static const int NoCorrectionIndex;

    /**
     * @brief Returns the nearest index in the gamma table
     * @param gamma required gamma value
     * @return index in table
     */
    static int getIndex(float gamma);

    /**
     * @brief Returns the gamma value corresponding to the index in the table
     * @param index index in table
     * @return gamma value
     */
    static float getValue(int index);

    /**
     * @brief Corrects gamma for font glyph image (byte buffer)
     * @param buf font glyph image
     * @param buf_sz size of the glyph image in bytes
     * @param gamma_index gamma index, must be 0..56 (15 means no correction).
     *   0 - gamma value '0.3'; 15 - '1.0'; 30 - '1.9'
     */
    static void gammaCorrection(unsigned char* buf, int buf_sz, int gamma_index);
};

#endif // __LVGAMMACORRECTION_H_INCLUDED__
