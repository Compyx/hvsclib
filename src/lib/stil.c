/* vim: set et ts=4 sw=4 sts=4 fdm=marker syntax=c.doxygen: */

/** \file   src/lib/stil.c
 * \brief   Sid Tune Information List handling
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
#include <string.h>
#include <ctype.h>

#include "base.h"

#include "stil.h"


/*
 * Forward declarations
 */

static void                 stil_field_init(hvsc_stil_field_t *field);
static hvsc_stil_field_t *  stil_field_new(int type,
                                           const char *text,
                                           long ts_from, long ts_to);
static void                 stil_field_free(hvsc_stil_field_t *field);
static hvsc_stil_field_t *  stil_field_dup(const hvsc_stil_field_t *field);

static void                 stil_block_init(hvsc_stil_block_t *block);
static hvsc_stil_block_t *  stil_block_new(void);
static void                 stil_block_free(hvsc_stil_block_t *block);
static bool                 stil_block_add_field(hvsc_stil_block_t *block,
                                                 hvsc_stil_field_t *field);
static hvsc_stil_block_t *  stil_block_dup(const hvsc_stil_block_t *block);

static bool                 stil_parse_timestamp(char *s,
                                                 hvsc_stil_timestamp_t *ts,
                                                 char **endptr);


/** \brief  List of field indentifiers
 *
 * \see hvsc_stil_field_type_t
 */
static const char *field_identifiers[] = {
    " ARTIST:",
    " AUTHOR:",
    "    BUG:",     /* XXX: only used in BUGlist.txt */
    "COMMENT:",
    "   NAME:",
    "  TITLE:",
    NULL
};


/** \brief  List of field identifier display string for dumping
 *
 * This makes it more clear to distinguish parser errors (ie NAME: showing up
 * in a field text)
 */
static const char *field_displays[] = {
    "{ artist}",
    "{ author}",
    "{    bug}",     /* XXX: only used in BUGlist.txt */
    "{comment}",
    "{   name}",
    "{  title}",
    NULL
};


/** \brief  Determine is \a s hold a field identifier
 *
 * Checks against a list of know field identifiers.
 *
 * \param[in]   s   string to parse
 *
 * \return  field type or -1 (HVSC_FIELD_INVALID) when not found
 *
 * \note    returning -1 does not indicate failure, just that \a s doesn't
 *          contain a field indentifier (ie normal text for a comment or so)
 */
static int stil_get_field_type(const char *s)
{
    int i = 0;

    while (field_identifiers[i] != NULL) {
        int result = strncmp(s, field_identifiers[i], 8);
        if (result == 0) {
            return i;   /* got it */
        }
        i++;
    }
    return HVSC_FIELD_INVALID;
}


/** \brief  Parse tune number from string \a s
 *
 * The string \a s is expected to be in the form "(#N)" where N is a decimal
 * string.
 *
 * \param[in]   s   string
 *
 * \return  tune number or -1 when not found
 */
static int stil_parse_tune_number(const char *s)
{
    while (*s != '\0' && isspace((int)*s)) {
        s++;
    }
    if (*s == '(' && *(s + 1) == '#') {
        char *endptr;
        long result;

        result = strtol(s + 2, &endptr, 10);
        if (*endptr == ')') {
            return (int)result;
        }
    }
    return -1;
}


/** \brief  Parse a STIL timestamp
 *
 * A STIL timestamp is either '([H]H:MM)' or '([H]H:MM-[H]H:MM)'. In the first
 * case, the 'to' member of \a ts is set to -1 to indicate only a single
 * timestamp was found, not a range.
 *
 * \param[in]   s       string to parse
 * \param[out]  ts      timestamp object to store result
 * \param[out]  endptr  object to store pointer to first non-parsed character
 *
 * \return  bool
 */
static bool stil_parse_timestamp(char *s, hvsc_stil_timestamp_t *ts,
                                 char **endptr)
{
    char *p;
    long result;

    /* get first/only entry */
    result = hvsc_parse_simple_timestamp(s, &p);
    if (result < 0) {
        *endptr = p;
        return false;
    }
    ts->from = result;

    /* do we have a range? */
    if (*p != '-') {
        /* nope, single entry */
        ts->to = -1;
        *endptr = p;
        return true;
    }

    /* get second entry */
    result = hvsc_parse_simple_timestamp(p + 1, endptr);
    if (result < 0) {
        return false;
    }
    ts->to = result;
    return true;
}



/*
 * STIL field functions
 */

/** \brief  Initialize \a field for use
 *
 * \param[in,out]   field   STIL field object
 */
static void stil_field_init(hvsc_stil_field_t *field)
{
    field->type = HVSC_FIELD_INVALID;
    field->text = NULL;
    field->timestamp.from = -1;
    field->timestamp.to = -1;
}


/** \brief  Allocate a new STIL field object
 *
 * \param[in]   type    field type
 * \param[in]   text    field text
 * \param[in]   ts_from timestamp 'from' member
 * \param[in]   ts_to   timestamp 'to' member
 *
 * \return  new STIL field object or `NULL` on failure
 */
static hvsc_stil_field_t *stil_field_new(int type, const char *text,
                                         long ts_from, long ts_to)
{
    hvsc_stil_field_t *field = malloc(sizeof *field);

    if (field == NULL) {
        hvsc_errno = HVSC_ERR_OOM;
    } else {
        stil_field_init(field);
        field->type = type;
        field->timestamp.from = ts_from;
        field->timestamp.to = ts_to;
        field->text = hvsc_strdup(text);
        if (field->text == NULL) {
            stil_field_free(field);
            field = NULL;
        }
    }
    return field;
}


/** \brief  Free memory used by member of \a field and \a field itself
 *
 * \param[in,out]   field   STIL field object
 */
static void stil_field_free(hvsc_stil_field_t *field)
{
    if (field->text != NULL) {
        free(field->text);
    }
    free(field);
}


/*
 * STIL block functions
 */

/** \brief  Initialize STIL block
 *
 * Initializes all members to 0/NULL.
 *
 * \param[in,out]   block   STIL block
 */
static void stil_block_init(hvsc_stil_block_t *block)
{
    block->tune = 0;
    block->fields = NULL;
    block->fields_max = 0;
    block->fields_used = 0;
}


/** \brief  Allocate and intialize a new STIL block
 *
 * \return  new STIL block or `NULL` on failure
 */
static hvsc_stil_block_t *stil_block_new(void)
{
    hvsc_stil_block_t *block;
    size_t i;

    block = malloc(sizeof *block);
    if (block == NULL) {
        hvsc_errno = HVSC_ERR_OOM;
        return NULL;
    }
    stil_block_init(block);

    block->fields = malloc(HVSC_STIL_BLOCK_FIELDS_INIT * sizeof *(block->fields));
    if (block->fields == NULL) {
        hvsc_errno = HVSC_ERR_OOM;
        stil_block_free(block);
        return NULL;
    }
    block->fields_max = HVSC_STIL_BLOCK_FIELDS_INIT;
    for (i = 0; i < HVSC_STIL_BLOCK_FIELDS_INIT; i++) {
        block->fields[i] = NULL;
    }
    return block;
}


/** \brief  Make a deep copy of \a field
 *
 * \param[in]   field   STIL field
 *
 * \return  copy of \a field or `NULL` on error
 */
static hvsc_stil_field_t *stil_field_dup(const hvsc_stil_field_t *field)
{
    return stil_field_new(field->type, field->text, field->timestamp.from,
            field->timestamp.to);
}


/** \brief  Make a deep copy of \a block
 *
 * \param[in]   block   STIL block
 *
 * \return  copy of \a block or `NULL` or error
 */
static hvsc_stil_block_t *stil_block_dup(const hvsc_stil_block_t *block)
{
    hvsc_stil_block_t *copy;
    size_t i;

    copy = malloc(sizeof *copy);
    if (copy == NULL) {
        return false;
    }
    stil_block_init(copy);

    copy->tune = block->tune;
    copy->fields_max = block->fields_max;
    copy->fields_used = block->fields_used;
    copy->fields = malloc(block->fields_max * sizeof *(copy->fields));
    for (i = 0; i < copy->fields_used; i++) {
        copy->fields[i] = stil_field_dup(block->fields[i]);
        if (copy->fields[i] == NULL) {
            stil_block_free(copy);
            return false;
        }
    }
    return copy;
}


/** \brief  Free STIL block and its members
 *
 * \param[in,out]   block   STIL block
 */
static void stil_block_free(hvsc_stil_block_t *block)
{
    size_t i;

    for (i = 0; i < block->fields_used; i++) {
        stil_field_free(block->fields[i]);
    }
    free(block->fields);
    free(block);
}


/** \brief  Add STIL \a field to STIL \a block
 *
 * \param[in,out]   block   STIL block
 * \param[in]       field   STIL field
 *
 * \return  bool
 */
static bool stil_block_add_field(hvsc_stil_block_t *block,
                                 hvsc_stil_field_t *field)
{
    hvsc_dbg("max = %zu, used = %zu\n", block->fields_max, block->fields_used);
    /* do we need to resize the array? */
    if (block->fields_max == block->fields_used) {
        /* yep */
        hvsc_stil_field_t **tmp;

        tmp = realloc(block->fields,
                block->fields_max * 2 * sizeof *(block->fields));
        if (tmp == NULL) {
            hvsc_errno = HVSC_ERR_OOM;
            return false;
        }
        block->fields = tmp;
        block->fields_max *= 2;
    }
    block->fields[block->fields_used++] = field;
    return true;
}




/** \brief  Initialize STIL \a handle for use
 *
 * \param[in,out]   handle  STIL handle
 */
static void stil_init_handle(hvsc_stil_t *handle)
{
    hvsc_text_file_init_handle(&(handle->stil));
    handle->psid_path = NULL;
    handle->entry_buffer = NULL;
    handle->entry_bufmax = 0;
    handle->entry_bufused = 0;
    handle->sid_comment = NULL;
    handle->blocks = NULL;
    handle->blocks_max = 0;
    handle->blocks_used = 0;
}


/** \brief  Allocate initial 'blocks' array
 *
 * All block pointers are initialized to `NULL`
 *
 * \param[in,out]   handle  STIL handle
 *
 * \return  bool
 */
static bool stil_handle_init_blocks(hvsc_stil_t *handle)
{
    size_t i;

    handle->blocks = malloc(HVSC_HANDLE_BLOCKS_INIT * sizeof *(handle->blocks));
    if (handle->blocks == NULL) {
        hvsc_errno = HVSC_ERR_OOM;
        return false;
    }
    for (i = 0; i < HVSC_HANDLE_BLOCKS_INIT; i++) {
        handle->blocks[i] = NULL;
    }
    handle->blocks_used = 0;
    handle->blocks_max = HVSC_HANDLE_BLOCKS_INIT;
    return true;
}


/** \brief  Free STIL blocks (entries + array)
 *
 * \param[in,out]   handle  STIL handle
 */
static void stil_handle_free_blocks(hvsc_stil_t *handle)
{
    if (handle->blocks != NULL) {
        size_t i;

        for (i = 0; i < handle->blocks_used; i++) {
            stil_block_free(handle->blocks[i]);
        }
        free(handle->blocks);
        handle->blocks = NULL;
    }
}


/** \brief  Add STIL \a block to STIL \a handle
 *
 * \param[in,out]   handle  STIL handle
 * \param[in]       block   STIL block
 *
 * \return  bool
 */
static bool stil_handle_add_block(hvsc_stil_t *handle, hvsc_stil_block_t *block)
{
    hvsc_stil_block_t *copy;

    /* do we need to resize the array? */
    if (handle->blocks_max == handle->blocks_used) {
        /* yep */
        hvsc_stil_block_t **tmp;

        tmp = realloc(handle->blocks,
                handle->blocks_max * 2 * sizeof *(handle->blocks));
        if (tmp == NULL) {
            hvsc_errno = HVSC_ERR_OOM;
            return false;
        }
        handle->blocks = tmp;
        handle->blocks_max *= 2;
    }

    /* make a copy */
    copy = stil_block_dup(block);
    if (copy == NULL) {
        return false;
    }
    handle->blocks[handle->blocks_used++] = copy;
    return true;
}



/** \brief  Open STIL and look for PSID file \a psid
 *
 * \param[in]       psid    path to PSID file
 * \param[in,out]   handle  STIL handle
 *
 * \return  bool
 */
bool hvsc_stil_open(const char *psid, hvsc_stil_t *handle)
{
    const char *line;

    stil_init_handle(handle);

    handle->entry_buffer = malloc(HVSC_STIL_BUFFER_INIT *
            sizeof *(handle->entry_buffer));
    if (handle->entry_buffer == NULL) {
        hvsc_errno = HVSC_ERR_OOM;
        return false;
    }
    handle->entry_bufmax = HVSC_STIL_BUFFER_INIT;
    handle->entry_bufused = 0;

    if (!hvsc_text_file_open(hvsc_stil_path, &(handle->stil))) {
        return false;
        hvsc_stil_close(handle);
    }

    /* make copy of psid, ripping off the HVSC root directory */
    handle->psid_path = hvsc_path_strip_root(psid);
    hvsc_dbg("stripped path is '%s'\n", handle->psid_path);
    if (handle->psid_path == NULL) {
        hvsc_stil_close(handle);
        return false;
    }

    /* find the entry */
    while (true) {
        line = hvsc_text_file_read(&(handle->stil));
        if (line == NULL) {
            if (feof(handle->stil.fp)) {
                /* EOF, so simply not found */
                hvsc_errno = HVSC_ERR_NOT_FOUND;
            }
            hvsc_stil_close(handle);
            /* I/O error is already set */
            return false;
        }

        if (strcmp(line, handle->psid_path) == 0) {
            hvsc_dbg("Found '%s' at line %ld\n", line, handle->stil.lineno);
            return true;
        }
    }

    /* not found */
    hvsc_errno = HVSC_ERR_NOT_FOUND;
    hvsc_stil_close(handle);
    return true;
}


/** \brief  Clean up memory and close file handle(s) used by \a handle
 *
 * Doesn't free \a handle itself.
 *
 * \param[in,out]   handle  STIL handle
 */
void hvsc_stil_close(hvsc_stil_t *handle)
{
    hvsc_text_file_close(&(handle->stil));
    free(handle->psid_path);

    if (handle->entry_buffer != NULL) {
        size_t i;
        for (i = 0; i < handle->entry_bufused; i++){
            free(handle->entry_buffer[i]);
        }
        free(handle->entry_buffer);
    }

    if (handle->sid_comment != NULL) {
        free(handle->sid_comment);
    }
    if (handle->blocks != NULL) {
        stil_handle_free_blocks(handle);
    }
}


/** \brief  Add a \a line of STIL entry text to \a handle
 *
 * Add a line from the STIL to \a handle, for proper parsing later.
 *
 * \param[in,out]   handle  STIL handle
 * \param[in]       line    line of text
 *
 * \return  bool
 */
static bool hvsc_stil_entry_add_line(hvsc_stil_t *handle, const char *line)
{
    char **buffer;
    char *tmp;

    if (handle->entry_bufmax == handle->entry_bufused) {
        hvsc_dbg("resizing line buffer to %zu entries\n",
                handle->entry_bufmax * 2);
        buffer = realloc(handle->entry_buffer,
                (handle->entry_bufmax * 2) * sizeof *(handle->entry_buffer));
        if (buffer == NULL) {
            hvsc_errno = HVSC_ERR_OOM;
            return false;
        }
        handle->entry_buffer = buffer;
        handle->entry_bufmax *= 2;
    }

    tmp = hvsc_strdup(line);
    if (tmp == NULL) {
        return false;
    }

    handle->entry_buffer[handle->entry_bufused++] = tmp;
    return true;
}


/** \brief  Read current STIL entry
 *
 * Reads all text lines in of the current STIL entry.
 *
 * \param[in,out]   handle  STIL handle
 *
 * \return  bool
 */
bool hvsc_stil_read_entry(hvsc_stil_t *handle)
{
    const char *line;


    while (true) {
        line = hvsc_text_file_read(&(handle->stil));
        if (line == NULL) {
            /* EOF ? */
            if (feof(handle->stil.fp)) {
                /* EOF, so end of entry */
                return true;
            }
            /* I/O error is already set */
            return false;
        }

        /* check for end of entry */
        if (hvsc_string_is_empty(line)) {
            hvsc_dbg("got empty line -> end-of-entry\n");
            return true;
        }

        hvsc_dbg("line %ld: '%s'\n", handle->stil.lineno, line);
        if (!hvsc_stil_entry_add_line(handle, line)) {
            return false;
        }
    }
}


/** \brief  Helper function: dump the lines of the current STIL entry on stdout
 *
 * \param[in]   handle  STIL handle
 */
void hvsc_stil_dump_entry(hvsc_stil_t *handle)
{
    size_t i;

    for (i = 0; i < handle->entry_bufused; i++) {
        printf("%s\n", handle->entry_buffer[i]);
    }
}



/*
 * Functions to parse the STIL entry text into a structured representation
 */


/** \brief  Parse the STIL entry text for a comment
 *
 * Parses a comment from the lines of text in the parser's stil entry. The
 * comment is expected to start with 'COMMENT:' on the first line and each
 * subsequent line is expected to start with 9 spaces, per STIL.faq.
 *
 * \param[in]   state   parser state
 *
 * \return  comment, or `NULL` on failure
 */
static char *stil_parse_comment(hvsc_stil_parser_state_t *state)
{
    char *comment;
    char *tmp;
    size_t len;     /* len per line, excluding '\0' */
    size_t total;   /* total line of comment, excluding '\0' */
    const char *line = state->handle->entry_buffer[state->lineno];

    /* first line is 'COMMENT: <text>' */
    comment = hvsc_strdup(line + 9);
    if (comment == NULL) {
        /* error */
        return NULL;
    }
    total = strlen(line) - 9;
    state->lineno++;

    while (state->lineno < state->handle->entry_bufused) {
        line = state->handle->entry_buffer[state->lineno];
        len = strlen(line);
        /* check for nine spaces */
        if (strncmp("         ", line, 9) != 0) {
            return comment;
        }
        /* realloc to add new line */
        tmp = realloc(comment, total + len - 8 + 1);
        if (tmp == NULL) {
            hvsc_errno = HVSC_ERR_OOM;
            free(comment);
            return NULL;
        }
        comment = tmp;
        /* add line to comment, adding a space from the nine spaces indent to
         * get a proper separating space in the final comment text */
        memcpy(comment + total, line + 8, len - 8 + 1);
        total += (len - 8);

        state->lineno++;
    }

    return comment;
}


/** \brief  Initialize parser
 *
 * Initializes parser state, stores a pointer to handle in the object to easier
 * pass around data.
 *
 * \param[in,out]   parser  STIL parser state
 * \param[in]       handle  STIL handle
 *
 * \return  bool
 */
static bool stil_parser_init(hvsc_stil_parser_state_t *parser,
                             hvsc_stil_t *handle)
{
    parser->handle = handle;
    parser->tune = 0;
    parser->lineno = 0;
    parser->field = NULL;

    /* add block for tune #1 */
    parser->block = stil_block_new();
    if (parser->block == NULL) {
        return false;
    }
    return true;
}


/** \brief  Free memory used by the parser's members
 *
 * Frees memory used by the members of \a parser, but not parser itself. The
 * STIL handle stored in \a parser also isn't freed, that is done by
 * hvsc_stil_close()
 *
 * \param[in,out]   parser  STIL parser state
 */
static void stil_parser_free(hvsc_stil_parser_state_t *parser)
{
    if (parser->block != NULL) {
        stil_block_free(parser->block);
    }
}




/** \brief  Parse textual content of \a handle into a structured representation
 *
 * \param[in,out]   handle  STIL entry handle
 *
 * \return  bool
 */
bool hvsc_stil_parse_entry(hvsc_stil_t *handle)
{
    hvsc_stil_parser_state_t state;

    /* init parser state */
    if (!stil_parser_init(&state, handle)) {
        return false;
    }

    /* allocate array for STIL blocks */
    if (!stil_handle_init_blocks(handle)) {
        return false;
    }

    while (state.lineno < state.handle->entry_bufused) {
        char *line = handle->entry_buffer[state.lineno];
        size_t len;
        char *comment;
        int type;
        int num;
        char *t;
        hvsc_stil_timestamp_t ts;

        ts.from = -1;
        ts.to = -1;

        /* to avoid unitialized warning later on (it isn't uinitialized) */
        comment = NULL;

        hvsc_dbg("parsing:\n%s\n", line);

        /* tune number? */
        num = stil_parse_tune_number(line);
        if (num > 0) {
            hvsc_dbg("Got tune mumber %d\n", num);
            state.tune = num;

            /*
             * store block and alloc new one (if tune > 1, otherwise we already
             * have a block)
             */
            if (state.tune > 1) {
                if (!stil_handle_add_block(state.handle, state.block)) {
                    return false;
                }
                stil_block_free(state.block);
                state.block = stil_block_new();
                if (state.block == NULL) {
                    return false;
                }
                state.block->tune = num;
            }

        } else {
            /* must be a field */
            type = stil_get_field_type(line);
            hvsc_dbg("Got field type %d\n", type);

            switch (type) {
                /* COMMENT: field */
                case HVSC_FIELD_COMMENT:
                    comment = stil_parse_comment(&state);
                    if (comment == NULL) {
                        return false;
                    }
                    if (state.tune == 0) {
                        /* SID-wide comment */
                        state.handle->sid_comment = comment;
                    } else {
                        /* normal per-tune comment */
                        line = comment;
                    }
                    /* comment parsing 'ate' the first non-comment line, so
                     * adjust parser state */
                    state.lineno--;
                    break;

                /* TITLE: field */
                case HVSC_FIELD_TITLE:
                    /* check for timestamp */
                    len = strlen(line);
                    /* find closing ')' at end of line */
                    if (len > 6 && line[len - 1] == ')') {
                        hvsc_dbg("possible TIMESTAMP\n");
                        line += 9;

                        /* find opening '(' */
                        t = line + len - 1;
                        while (t >= line && *t != '(') {
                            t--;
                        }
                        if (t == line) {
                            /* nope */
                            hvsc_dbg("no closing '(') found, ignoring\n");
                        } else {
                            char *endptr;

                            if (!stil_parse_timestamp(t + 1, &ts, &endptr)) {
                                /*
                                 * Some lines contain strings like "(lyrics)"
                                 * or "(music)", so don't trigger a parser
                                 * error, just ignore
                                 */
                                hvsc_dbg("invalid TIMESTAMP, ignoring\n");
                            } else {
                                hvsc_dbg("got TIMESTAMP: %ld-%ld\n",
                                        ts.from, ts.to);
                                /* TODO: adjust line: strip timestamp text */
                            }
                        }
                    }

                    /* TODO: check for 'Album' field: [from ...] */
                    break;

                /* Other fields without special meaning/sub fields */
                default:
                    /* don't copy the first nine chars (field ident + space) */
                    line += 9;
                    break;
            }

            /*
             * Add line to block
             * TODO: parse out sub fields and timestamps
             */
            if (state.tune > 0) {
                hvsc_dbg("Adding '%s'\n", line);
                state.field = stil_field_new(type, line, ts.from, ts.to);
                if (state.field == NULL) {
                    hvsc_dbg("failed to allocate field object\n");
                    return false;
                }
                if (!stil_block_add_field(state.block, state.field)) {
                    hvsc_dbg("failed to add field to block\n");
                    return false;
                }

                /* if the line was a comment, free the comment */
                if (type == HVSC_FIELD_COMMENT) {
                    free(comment);
                    comment = NULL;
                }
            } else {
                /* got all the SID-wide stuff, now add the rest to per-tune
                 * STIL blocks */
                state.tune = 1;
                state.block->tune = 1;
            }
        }
        state.lineno++;
    }

    /* add last block */
    if (!stil_handle_add_block(state.handle, state.block)) {
        return false;
    }

    stil_parser_free(&state);
    return true;
}



/** \brief  Temp: dump parsed entries
 *
 * \param[in]   handle  STIL handle
 */
void hvsc_stil_dump(hvsc_stil_t *handle)
{
    size_t t;   /* tune index, not its number */
    size_t f;   /* field index, not its type */

    printf("\n\n{File: %s}\n", handle->psid_path);
    if (handle->sid_comment != NULL) {
        printf("\n{SID-wide comment}\n%s\n", handle->sid_comment);
    }

    printf("\n{Per-tune info}\n\n");
    for (t = 0; t < handle->blocks_used; t++) {
        hvsc_stil_block_t *block = handle->blocks[t];
        printf("  {#%d}\n", block->tune);
        for (f = 0; f < block->fields_used; f++) {
            printf("    %s %s\n",
                    field_displays[block->fields[f]->type],
                    block->fields[f]->text);
            /* do we have a valid timestamp ? */
            if (block->fields[f]->timestamp.from >= 0) {
                long from = block->fields[f]->timestamp.from;
                long to = block->fields[f]->timestamp.to;

                if (to < 0) {
                    printf("      {timestamp} %ld:%02ld\n",
                            from / 60, from % 60);
                } else {
                    printf("      {timestamp} %ld:%02ld-%ld:%02ld\n",
                            from / 60, from % 60, to / 60, to % 60);
                }
            }
        }
        putchar('\n');
    }
}
