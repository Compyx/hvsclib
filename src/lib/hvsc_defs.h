/* vim: set et ts=4 sw=4 sts=4 fdm=marker syntax=c.doxygen: */

/** \file   src/lib/hvsc_defs.h
 * \brief   Globally used constants and types
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

#ifndef HVSC_HVSC_DEFS_H
#define HVSC_HVSC_DEFS_H

/** \brief  Enable debugging messages via hvsc_dbg()
 */
#if 0
#define HVSC_DEBUG
#endif

/** \brief  Error code for the library
 */
extern int hvsc_err;


/** \brief  Path to the Songlengths file, relative to the HVSC root
 */
#define HVSC_SLDB_FILE  "DOCUMENTS/Songlengths.md5"


/** \brief  Path to the STIL file, relative to the HVSC root
 */
#define HVSC_STIL_FILE  "DOCUMENTS/STIL.txt"


/** \brief  Path to the BUGlist file, relative to the HVSC root
 */
#define HVSC_BUGS_FILE  "DOCUMENTS/BUGlist.txt"


/** \brief  MD5 digest size in bytes
 */
#define HVSC_DIGEST_SIZE    16


/** \brief  Error codes
 */
typedef enum hvsc_err_e {
    HVSC_ERR_OK = 0,            /**< no error */
    HVSC_ERR_OOM,               /**< out of memory error */
    HVSC_ERR_IO,                /**< I/O error */
    HVSC_ERR_FILE_TOO_LARGE,    /**< file too large (> 2GB) */
    HVSC_ERR_GCRYPT,            /**< error in gcrypt library */
    HVSC_ERR_TIMESTAMP,         /**< error parsing a timestamp */
    HVSC_ERR_NOT_FOUND,         /**< entry/tune not found */

    HVSC_ERR_CODE_COUNT         /**< number of error messages */

} hvsc_err_t;


/** \brief  STIL entry field type enumeration
 */
typedef enum hvsc_stil_field_type_e {
    HVSC_FIELD_INVALID = -1,    /**< unknow field type */
    HVSC_FIELD_ARTIST,          /**< Artist (in case of cover) */
    HVSC_FIELD_AUTHOR,          /**< Author (of SID/subtune) */
    HVSC_FIELD_BUG,             /**< Bug note (only in BUGlist.txt) */
    HVSC_FIELD_COMMENT,         /**< Comment */
    HVSC_FIELD_NAME,            /**< (sub)tune name */
    HVSC_FIELD_TITLE            /**< title of the cover */
} hvsc_stil_field_type_t;


/** \brief  Handle for the text file reader functions
 */
typedef struct hvsc_text_file_s {
    FILE *  fp;     /**< file pointer */
    char *  path;   /**< copy of the path of the file (for error messages) */
    long    lineno; /**< line number in file */
    size_t  linelen;    /**< line length */
    char *  buffer; /**< buffer for line data, grows when required */
    size_t  buflen; /**< size of buffer, grows when needed */
} hvsc_text_file_t;


/** \brief  Number of initial entries in the STIL buffer
 */
#define HVSC_STIL_BUFFER_INIT    32


/** \brief  STIL timestamp object
 *
 * Set `to` to -1 to signal only `from` should be used. Set `from` to -1 to
 * signal the entire timestamp is unused. Both entries are in seconds.
 *
 * Examples: no timestamp would result in { -1, -1 }
 *           "(0:30)" would result in { 30, -1 }
 *           "(0:30-2:15)" would result in { 30, 135 }
 */
typedef struct hvsc_stil_timestamp_s {
    long from;  /**< 'from' timestamp, or only timestamp */
    long to;    /**< 'to' timestamp, optional */
} hvsc_stil_timestamp_t;


/** \brief  STIL field object
 */
typedef struct hvsc_stil_field_s {
    hvsc_stil_field_type_t      type;       /**< field type */
    char *                      text;       /**< field content */
    hvsc_stil_timestamp_t       timestamp;  /**< timestamp (optional) */
    char *                      album;      /**< cover info (optional) */
} hvsc_stil_field_t;


/** \brief  Initial size of the fields array in a hvsc_stil_block_t
 */
#define HVSC_STIL_BLOCK_FIELDS_INIT    32

/** \brief  STIL block object
 */
typedef struct hvsc_stil_block_s {
    int                 tune;           /**< tune number
                                             (0 = global/only tune) */
    hvsc_stil_field_t **fields;         /**< list of STIL fields, in order of
                                             the text in the STIL.txt file */
    size_t              fields_max;     /**< size of the fields array */
    size_t              fields_used;    /**< used entries in the fields array */
} hvsc_stil_block_t;


/** \brief  Initial size of blocks array in a handle
 */
#define HVSC_HANDLE_BLOCKS_INIT    32


/** \brief  Handle for the STIL functions
 */
typedef struct hvsc_stil_s {
    hvsc_text_file_t    stil;           /**< handle for the STIL.txt file */
    char *              psid_path;      /**< path to PSID file */
    char **             entry_buffer;   /**< content of the STIL entry */
    size_t              entry_bufmax;   /**< number of available entries in
                                             the entry_buffer */
    size_t              entry_bufused;  /**< number of used entries in the
                                             entry_buffer */
    char *              sid_comment;    /**< global comment (optional) */
    hvsc_stil_block_t **blocks;         /**< STIL blocks */
    size_t              blocks_max;     /**< number of available blocks */
    size_t              blocks_used;    /**< number of used blocks */
} hvsc_stil_t;


/** \brief  STIL parser state
 */
typedef struct hvsc_stil_parser_state_s {
    hvsc_stil_t *           handle;     /**< STIL handle */
    hvsc_stil_field_t *     field;      /**< STIL field object */
    int                     tune;       /**< current tune number */
    size_t                  lineno;     /**< line number in STIL text buffer */
    hvsc_stil_block_t *     block;      /**< temporary STIL block */

    hvsc_stil_timestamp_t   ts;         /**< temporary timestamp object */
    size_t                  linelen;    /**< remaining length of the current
                                             line after parsing out the
                                             optional sub fields
                                             (timestamp, album) */

    char *                  album;      /**< album/cover string */
    size_t                  album_len;  /**< length of album string */
} hvsc_stil_parser_state_t;


/** \brief  BUGlist entry
 */
typedef struct hvsc_buglist_entry_s {
    char *desc;     /**< descriptions of the bug */
    char *user;     /**< person reporting the bug */
} hvsc_buglist_entry_t;

#endif
