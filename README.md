# hvsclib

## A library to work with HVSC (High Voltage SID Collection) files

### Introduction

This library is mostly meant for SID players to retrieve data from the `Songlengths.md5`, `STIL.txt` and `BUGlist.txt` files in `/DOCUMENTS/` directory of the HVSC. The song length data can be used by a player to skip to the next sub tune in a SID, rather than looping forever, and the STIL and BUGlist data contain extra information on SIDs, such as commments by the original composers and information on if a SID (sub)tune covers another SID (sub)tune or music from another medium.

This is currently very much a work in progress. Which means the API isn't stable yet and a lot can change.

#### Dependencies

<s>Currently, hvsclib has a single library-dependency: libgrypt20. The `Makefile` just assumes its there, so if you get compile errors, install libgcrypt20-dev. This libary is used to calculate the MD5 digest of a given SID file so the songlength data can be retrieved from `Songlengths.md5`. I may implement my own MD5 algorithm at some point to remove this dependency.</s>

The libgrycpt20 dependency has been removed, it didn't make sense when using SIDs from the HVSC. (It can still be enabled, see the `Makefile`, if you really wish to complicate things)

To build the library, a proper C99-compliant compiler is required, as is GNU Make. The `Makefile` is written with GCC in mind, but it should be easily adjustable to Clang and perhaps others.

To build the documentation, Doxygen is required, with graphviz to generate the call/caller graphs.


### Basic usage

The library has a single header file, `hvsc.h`. Just #include that and you should be good to go.
A good example of how the library works is the file `hvsc-test.c`, which is a small test suite that uses all the functionality of the libary.


#### Initialization and cleanup

The library is initialized with a call to `hvsc_init()`:

The `hvsc_init()` call is needed to initialize the library, setting the proper paths to the HVSC files.

```C
#include <hvsc.h>

if (!hvsclib_init("/home/compyx/HVSC")) {
    hvsc_perror("crap");
    exit(1);
}
```

After use, the library should be cleaned up with a call to `hvsc_exit()`, this will free any resource the library used for its paths. Any other resources obtained from the library using other calls should still be free'd using the proper functions.
```C
#include <hvsc.h>

hvsc_exit();
```

#### Getting song lengths for a SID

Probably the most important function of the library is to retrieve song lengths, the duration of each (sub)tune. This information will allow a SID player to skip to the next song.

The following function will get the song lengths for SID file *path*. *path* is expected to be a relative path inside the HVSC. For example: "MUSICIANS/H/Hubbard_Rob/Commando.sid".

```C
#include <hvsc.h>

bool display_song_lengths(const char *path)
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
}
```

In the example above `*lengths'` is used to store a pointer to a list of `long int`s, each one a song length in seconds, while the return value of `hvsc_sldb_get_lengths()` is the number of elements in the list. When the function returns < 0, no song length info on the SID file was found (most likely the SID filename was incorrect, but theoretically, a call to malloc(3) could have failed, check `hvsc_errno` to be sure).
