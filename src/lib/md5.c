/* vim: set et ts=4 sw=4 sts=4 fdm=marker syntax=c.doxygen: */

/** \file   src/lib/md5.c
 * \brief   MD5 implementation
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 *  HVSClib - a library to work with High Voltage SID Collection files
 *  Copyright (C) 2018  Bas Wassink <b.wassink@ziggo.nl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.*
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


/** \brief  Flag indicating the precalculated tables for MD5 are initialized
 */
static bool md5_initialized = false;


static uint32_t md5_shift_table[64] = {
    /* $00-$0f */
    7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
    /* $10-$1f */
    5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
    /* $20-$2f */
    4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
    /* $30-$3f */
    6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 16, 21,  6, 10, 16, 21
};


static uint32_t md5_table[64];


/** \brief  Initialize MD5 helper tables
 */
static void init_md5(void)
{
    int i;

    for (i = 0; i < 64; i++) {
        md5_table[i] = 0;
    }
}


/** \brief  Calculate MD5 hash of \a len bytes of \a message
 *
 * \param[in]   message data to calculate MD5 hash of
 * \param[in]   len     number of bytes in \a message
 * \param[in]   digest  target of the MD5 hash (the user must allocate at
 *                      least 16 bytes for this data)
 *
 * Example:
 * \code{.c}
 *
 *  uint8_t *data;  // let's assume this points to valid data
 *  uint8_t result[16];
 *
 *  hvsc_md5(data, 32, &result);
 * \endcode
 */
void hvsc_md5(const uint8_t *message, size_t len, uint8_t *digest)
{
    if (!md5_initialized) {
        init_md5();
        md5_initialized = true;
    }
}
