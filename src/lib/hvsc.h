/* vim: set et ts=4 sw=4 sts=4 fdm=marker syntax=c.doxygen: */

/** \file   src/lib/hvsc.h
 * \brief   Library header
 *
 * This file only contains prototypes, types and defines which are supposed to
 * be public.
 *
 * XXX: currently exposes to much internal stuff, perhaps use some opaque
 *      types (ie void*)?
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

#ifndef HVSC_HVSC_H
#define HVSC_HVSC_H

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
    HVSC_ERR_INVALID,           /**< invalid data or operation detected */

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
    HVSC_FIELD_TITLE,           /**< title of the cover */

    HVSC_FIELD_TYPE_COUNT       /**< number of valid field types */
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


/** \brief  Handle for the BUGlist functions
 */
typedef struct hvsc_bugs_s {
    hvsc_text_file_t        bugs;       /**< handle for the BUGlist.txt file */
    char *                  psid_path;  /**< path to PSID file */
    char *                  text;       /**< text about the bug */
    char *                  user;       /**< person reporting the bug */
} hvsc_bugs_t;


/** \brief  STIL tune entry object
 */
typedef struct hvsc_stil_tune_entry_s {
    int                 tune;           /**< tune number in the SID (1-256) */
    hvsc_stil_field_t **fields;         /**< STIL fields array */
    size_t              field_count;    /**< number of fields in \a fields */
} hvsc_stil_tune_entry_t;

/*
 * Types
 */



/*
 * main.c stuff
 */

bool        hvsc_init(const char *path);
void        hvsc_exit(void);

const char *hvsc_lib_version_str(void);
void        hvsc_lib_version_num(int *major, int *minor, int *revision);

/*
 * base.c stuff
 */


extern int hvsc_errno;

const char *hvsc_strerror(int n);
void        hvsc_perror(const char *prefix);


/*
 * sldb.c stuff
 */

#ifdef HVSC_USE_MD5
char *      hvsc_sldb_get_entry_md5(const char *psid);
#endif
char *      hvsc_sldb_get_entry_txt(const char *psid);
int         hvsc_sldb_get_lengths(const char *psid, long **lengths);


/*
 * stil.c stuff
 */

bool        hvsc_stil_open(const char *psid, hvsc_stil_t *handle);
void        hvsc_stil_close(hvsc_stil_t *handle);
bool        hvsc_stil_read_entry(hvsc_stil_t *handle);
void        hvsc_stil_dump_entry(hvsc_stil_t *handle);
bool        hvsc_stil_parse_entry(hvsc_stil_t *handle);
void        hvsc_stil_dump(hvsc_stil_t *handle);
bool        hvsc_stil_get_tune_entry(const hvsc_stil_t *handle,
                                     hvsc_stil_tune_entry_t *entry,
                                     int tune);
void        hvsc_stil_dump_tune_entry(const hvsc_stil_tune_entry_t *entry);

/*
 * bugs.c stuff
 */

bool        hvsc_bugs_open(const char *psid, hvsc_bugs_t *handle);
void        hvsc_bugs_close(hvsc_bugs_t *handle);

#endif
