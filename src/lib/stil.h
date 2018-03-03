/* vim: set et ts=4 sw=4 sts=4 fdm=marker syntax=c.doxygen: */

/** \file   src/lib/stil.h
 * \brief   Sid Tune Information List handling - header
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

#ifndef HVSC_STIL_H
#define HVSC_STIL_H

#include <stdbool.h>

#include "hvsc_defs.h"


bool hvsc_stil_open(const char *psid, hvsc_stil_t *handle);
void hvsc_stil_close(hvsc_stil_t *handle);
bool hvsc_stil_read_entry(hvsc_stil_t *handle);
void hvsc_stil_dump_entry(hvsc_stil_t *handle);

bool hvsc_stil_parse_entry(hvsc_stil_t *handle);

void hvsc_stil_dump(hvsc_stil_t *handle);

bool hvsc_stil_get_tune_entry(const hvsc_stil_t *handle,
                              hvsc_stil_tune_entry_t *entry,
                              int tune);

void hvsc_stil_dump_tune_entry(const hvsc_stil_tune_entry_t *entry);
#endif
