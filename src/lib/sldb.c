/* vim: set et ts=4 sw=4 sts=4 fdm=marker syntax=c.doxygen: */

/** \file   src/lib/sldb.c
 * \brief   Songlength database handling
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
#include <inttypes.h>
#include <stdbool.h>
#include <ctype.h>

#include <gcrypt.h>

#include "hvsc.h"

#include "hvsc_defs.h"
#include "base.h"

#include "sldb.h"


/** \brief  Calculate MD5 hash of file \a psid
 *
 * \param[in]   psid    PSID file
 * \param[out]  digest  memory to store MD5 digest, needs to be 16+ bytes
 *
 * \return  bool
 */
static bool create_md5_hash(const char *psid, unsigned char *digest)
{
    unsigned char *data;
    long size;
    gcry_md_hd_t handle;
    gcry_error_t err;
    unsigned char *d;

    /* attempt to open file */
    hvsc_dbg("reading '%s\n", psid);
    size = hvsc_read_file(&data, psid);
    if (size < 0) {
        fprintf(stderr, "failed!\n");
        return false;
    }
    hvsc_dbg("got %ld bytes\n", size);

    /*
     * calculate MD5 hash
     */
    err = gcry_md_open(&handle, GCRY_MD_MD5, 0);
    if (err != 0) {
        hvsc_errno = HVSC_ERR_GCRYPT;
        free(data);
        return -1;
    }

    gcry_md_write(handle, data, (size_t)size);
    d = gcry_md_read(handle, GCRY_MD_MD5);
    memcpy(digest, d, HVSC_DIGEST_SIZE);

    gcry_md_close(handle);

    free(data);

    return true;
}

/** \brief  Find SLDB entry by \a digest
 *
 * The \a digest has to be in the same string form as the SLDB. So 32 bytes
 * representing a 16-byte hex data, in lower case.
 *
 * \param[in]   digest  string representation of the MD5 digest (32 bytes)
 *
 * \return  line of text from SLDB or `NULL` when not found
 */
static char *find_sldb_entry(const char *digest)
{
    hvsc_text_file_t handle;
    const char *line;

    if (!hvsc_text_file_open(hvsc_sldb_path, &handle)) {
        return NULL;
    }

    while (true) {
        line = hvsc_text_file_read(&handle);
        if (line == NULL) {
            hvsc_text_file_close(&handle);
            return NULL;
        }
#if 0
        printf("%s\n", line);
#endif
        if (memcmp(digest, line, HVSC_DIGEST_SIZE * 2) == 0) {
            /* copy the current line before closing the file */
            char *s = hvsc_strdup(handle.buffer);
            hvsc_text_file_close(&handle);
            if (s == NULL) {
                return NULL;
            }
            return s;
        }
    }

    hvsc_text_file_close(&handle);
    hvsc_errno = HVSC_ERR_NOT_FOUND;
    return NULL;
}






/** \brief  Parse SLDB entry
 *
 * The song lengths array is heap-allocated and should freed after use.
 *
 * \param[in]   line    SLDB entry (including hash + '=')
 * \param[out]  lengths object to store pointer to array of song lengths
 *
 * \return  number of songs or -1 on error
 */
static int parse_sldb_entry(char *line, long **lengths)
{
    char *p;
    char *endptr;
    long *entries;
    int i = 0;
    long secs;

    entries = malloc(256 * sizeof *entries);
    if (entries == NULL) {
        return -1;
    }

    p = line + (HVSC_DIGEST_SIZE * 2 + 1);  /* skip MD5HASH and '=' */

    while (*p != '\0') {
        /* skip whitespace */
        while (*p != '\0' && isspace((int)(*p))) {
            p++;
        }
        if (*p == '\0') {
            *lengths = entries;
            return i;
        }

        secs = hvsc_parse_simple_timestamp(p, &endptr);
        if (secs < 0) {
            free(entries);
            return -1;
        }
        entries[i++] = secs;
        p = endptr;
    }

    *lengths = entries;
    return i;
}


/** \brief  Get the SLDB entry for PSID file \a psid
 *
 * \param[in]   psid    path to PSID file
 *
 * \return  heap-allocated entry or `NULL` on failure
 */
char *hvsc_sldb_get_entry(const char *psid)
{
    unsigned char hash[HVSC_DIGEST_SIZE];
    char hash_text[HVSC_DIGEST_SIZE * 2 + 1];
    bool result;
    int i;
    char *entry;

    result = create_md5_hash(psid, hash);
    if (!result) {
        return NULL;
    }

    /* generate text version of hash */
    hvsc_dbg("HASH = ");
    for (i = 0; i < HVSC_DIGEST_SIZE; i++) {
#ifdef HVSC_DEBUG
        printf("%02x", hash[i]);
#endif
        snprintf(hash_text + i * 2, 3, "%02x", hash[i]);
    }
#ifdef HVSC_DEBUG
    putchar('\n');
#endif

    /* parse SLDB */
    entry = find_sldb_entry(hash_text);
    if (entry == NULL) {
        return NULL;
    }
    hvsc_dbg("Got it: %s\n", entry);
    return entry;
}


/** \brief  Get a list of song lengths for PSID file \a psid
 *
 * \param[in]   psid    path to PSID file
 * \param[out]  lengths object to store pointer to array of song lengths
 *
 * \return  number of songs or -1 on error
 */
int hvsc_sldb_get_lengths(const char *psid, long **lengths)
{
    char *entry;
    int result;

    *lengths = NULL;

    entry = hvsc_sldb_get_entry(psid);
    if (entry == NULL) {
        return -1;
    }

    result = parse_sldb_entry(entry, lengths);
    if (result < 0) {
        free(*lengths);
        return false;
    }
    free(entry);
    return result;
}
