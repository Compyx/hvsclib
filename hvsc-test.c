/* vim: set et ts=4 sw=4 sts=4 fdm=marker syntax=c.doxygen: */

/** \file   hvsc-test.c
 * \brief   Test driver for hvsclib
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

#include "hvsc.h"


/** \brief  Test driver
 *
 * \return  EXIT_SUCCESS or EXIT_FAILURE
 */
int main(int argc, char *argv[])
{
    int num;
    int i;
    long *lengths;

    hvsc_stil_t stil;
    hvsc_bugs_t bugs;
    hvsc_stil_tune_entry_t tune_entry;
    hvsc_psid_t psid;

    int major;
    int minor;
    int revision;

    puts("HVSC LIB test driver\n");

    printf("Library version string = %s\n", hvsc_lib_version_str());
    hvsc_lib_version_num(&major, &minor, &revision);
    printf("Libarry version number = %d, %d, %d\n", major, minor, revision);

    if (argc < 2) {
        printf("Usage: %s <PSID-file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("Initializing .. ");
    if (!hvsc_init("/home/compyx/c64/HVSC")) {
        hvsc_perror(argv[0]);
        return EXIT_FAILURE;
    }
    printf("OK\n");

    printf("Retrieving songlength info .. \n");
    num = hvsc_sldb_get_lengths(argv[1], &lengths);
    if (num < 0) {
        hvsc_perror(argv[0]);
        hvsc_exit();
        return EXIT_SUCCESS;
    }
    printf("OK: ");
    printf("Got %d songs:\n", num);
    for (i = 0; i < num; i++) {
        printf("    %02ld:%02ld\n",
                lengths[i] / 60, lengths[i] % 60);
    }

    free(lengths);


    printf("\nTesting STIL handling\n\n");

    printf("Opening STIL, looking for %s\n", argv[1]);
    if (!hvsc_stil_open(argv[1], &stil)) {
        hvsc_perror(argv[0]);
        if (hvsc_errno != HVSC_ERR_NOT_FOUND) {
            hvsc_exit();
            return EXIT_FAILURE;
        }
        printf("Continuing\n");
    } else {

        printf("Reading STIL entry text\n");
        if (hvsc_stil_read_entry(&stil)) {
            printf("Dumping STIL entry text:\n");
            hvsc_stil_dump_entry(&stil);
        }

        printf("Parsing STIL entry text\n");
        if (!hvsc_stil_parse_entry(&stil)) {
            hvsc_perror("Failed");
            hvsc_stil_close(&stil);
            hvsc_exit();
            return EXIT_FAILURE;
        }

        printf("Dumping parsed data:\n");
        hvsc_stil_dump(&stil);

        /* test hvsc_stil_get_tune_entry() */
        printf("\nTesting hvsc_get_tune_entry(3):\n");
        if (!hvsc_stil_get_tune_entry(&stil, &tune_entry, 3)) {
            hvsc_perror("Failed");
        } else {
            printf("OK! Calling hvsc_stil_dump_tune_entry()\n\n");
            hvsc_stil_dump_tune_entry(&tune_entry);
        }

        printf("Closing STIL\n");
        hvsc_stil_close(&stil);
    }

    printf("\n\nTesting HVSC BUGlist\n\n");

    if (!hvsc_bugs_open(argv[1], &bugs)) {
        hvsc_perror(argv[0]);
        if (hvsc_errno == HVSC_ERR_NOT_FOUND) {
            printf("BUGlist: No entry found, no worries\n");
        }
    } else {
        printf("Found entry:\n");
        printf("{ bug} %s\n", bugs.text);
        printf("{user} %s\n", bugs.user);
        hvsc_bugs_close(&bugs);
    }

    printf("\n\nTesing PSID file handling\n\n");

    printf("Opening %s\n", argv[1]);
    if (hvsc_psid_open(argv[1], &psid)) {
        printf("Dumping header:\n");
        hvsc_psid_dump(&psid);
        hvsc_psid_close(&psid);
    }

    hvsc_exit();
    return EXIT_SUCCESS;
}
