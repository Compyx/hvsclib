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

After use, the library should be cleaned up with a call to `hvsc_exit()`, this will free any resource the libary used.
```
#include <hvsc.h>

hvsc_exit();
```

