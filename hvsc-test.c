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
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

#include "hvsc.h"

/** \brief  Test case
 */
typedef struct test_case_s {
    const char *name;               /**< test name */
    const char *desc;               /**< test description */
    bool (*func)(const char *);     /**< test function */
} test_case_t;




/** \brief  Run SLDB test on \a psid
 *
 * \param[in]   path    path to SID file
 *
 * \return  bool
 */
static bool test_sldb(const char *path)
{
    int i;
    int num;
    long *lengths;

    printf("Retrieving song lengths of '%s'\n", path);
    num = hvsc_sldb_get_lengths(path, &lengths);
    if (num < 0) {
        return false;
    }
    printf("OK: ");
    printf("Got %d songs:\n", num);
    for (i = 0; i < num; i++) {
        printf("    %02ld:%02ld\n",
                lengths[i] / 60, lengths[i] % 60);
    }

    free(lengths);
    return true;
}


/** \brief  Run STIL test on \a psid
 *
 * \param[in]   path    path to SID file
 *
 * \return  bool
 */
static bool test_stil(const char *path)
{
    hvsc_stil_t stil;
    hvsc_stil_tune_entry_t tune_entry;

    printf("Testing STIL handling\n\n");

    printf("Opening STIL, looking for %s\n", path);
    if (!hvsc_stil_open(path, &stil)) {
        hvsc_perror("hvsc-test");
        if (hvsc_errno != HVSC_ERR_NOT_FOUND) {
            return false;
        }
        printf("Continuing anyway...\n");
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
            return false;
        }

        printf("Dumping parsed data:\n");
        hvsc_stil_dump(&stil);

        /* test hvsc_stil_get_tune_entry() */
        printf("\nTesting hvsc_get_tune_entry(3):\n");
        if (!hvsc_stil_get_tune_entry(&stil, &tune_entry, 3)) {
            hvsc_perror("Failed");
            hvsc_stil_close(&stil);
            return false;
        } else {
            printf("OK! Calling hvsc_stil_dump_tune_entry()\n\n");
            hvsc_stil_dump_tune_entry(&tune_entry);
        }

        printf("Closing STIL\n");
        hvsc_stil_close(&stil);
    }
    return true;
}


/** \brief  Run BUGlist test on \a psid
 *
 * \param[in]   path    path to SID file
 *
 * \return  bool
 */
static bool test_buglist(const char *path)
{
    hvsc_bugs_t bugs;

    printf("Testing HVSC BUGlist\n\n");

    if (!hvsc_bugs_open(path, &bugs)) {
        hvsc_perror("hvsc-test");
        if (hvsc_errno == HVSC_ERR_NOT_FOUND) {
            printf("BUGlist: No entry found, no worries\n");
            return true;
        }
        return false;
    } else {
        printf("Found entry:\n");
        printf("{ bug} %s\n", bugs.text);
        printf("{user} %s\n", bugs.user);
        hvsc_bugs_close(&bugs);
        return true;
    }
}


/** \brief  Run PSID test on \a psid
 *
 * \param[in]   path    path to SID file
 *
 * \return  bool
 */
static bool test_psid(const char *path)
{
    hvsc_psid_t psid;
    printf("\n\nTesing PSID file handling\n\n");

    printf("Opening %s\n", path);
    if (hvsc_psid_open(path, &psid)) {
        printf("Dumping header:\n");
        hvsc_psid_dump(&psid);

        printf("Writing binary in SID as 'tune.sid'\n");
        if (hvsc_psid_write_bin(&psid, "tune.sid")) {
            printf("OK\n");
        } else {
            printf("Failed\n");
            hvsc_perror("hvsc-test");
        }

        hvsc_psid_close(&psid);
        return true;
    } else {
        hvsc_perror("hvsc-test");
        return false;
    }
}


/** \brief  Test cases
 */
static test_case_t cases[] = {
    { "sldb", "test Songlength.* database support", test_sldb },
    { "stil", "test STIL.txt (SID Tune Information List) support", test_stil },
    { "bugs", "test BUGlist.txt suport", test_buglist },
    { "psid", "test PSID file support", test_psid },
    { NULL, NULL, NULL }
};


/** \brief  Print usage message on stdout
 *
 * \param[in]   prg program name
 */
static void usage(const char *prg)
{
    int i;

    printf("Usage: %s <test-name> <psid-file>\n", prg);
    printf("\n<test-name> can either be 'all' to run all tests, or:\n");
    for (i = 0; cases[i].name != NULL; i++) {
        printf("\t%s\t%s\n", cases[i].name, cases[i].desc);
    }
}



/** \brief  Test driver
 *
 * \return  EXIT_SUCCESS or EXIT_FAILURE
 */
int main(int argc, char *argv[])
{
    int i;

    int major;
    int minor;
    int revision;

    if (argc < 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    puts("HVSC LIB test driver\n");

    printf("Library version string = %s\n", hvsc_lib_version_str());
    hvsc_lib_version_num(&major, &minor, &revision);
    printf("Libarry version number = %d, %d, %d\n", major, minor, revision);

    printf("Initializing .. ");
    if (!hvsc_init("/home/compyx/c64/HVSC")) {
        hvsc_perror(argv[0]);
        return EXIT_FAILURE;
    }
    printf("OK\n");

    printf("Got case '%s'\n", argv[1]);

    if (strcmp(argv[1], "all") == 0) {
        printf("Running all tests\n\n");
        for (i = 0; cases[i].name != NULL; i++) {
            if (cases[i].func(argv[2])) {
                printf("<<OK>>\n");
            } else {
                printf("<<Fail>>\n");
            }
        }
    } else {
        for (i = 0; cases[i].name != NULL; i++) {
            if (strcmp(cases[i].name, argv[1]) == 0) {
                if (cases[i].func(argv[2])) {
                    printf("<<OK>>\n");
                } else {
                    printf("<<Fail>>\n");
                }
                break;
            }
        }
    }

    hvsc_exit();
    return EXIT_SUCCESS;
}
