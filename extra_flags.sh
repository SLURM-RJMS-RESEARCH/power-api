#!/bin/bash

CFLAGS=""
LDFLAGS=""

function papi {
    if hash papi_version > /dev/null 2>&1 ; then
        version_str=$(papi_version | cut -d ' ' -f 3)
        IFS="." read -a version <<< "$version_str"
        major="${version[0]}"
        minor="${version[1]}"
        if [ "$major" -eq "5" -a "$minor" -ge "3" ] || [ "$major" -gt "5" ]; then
            CFLAGS+=" -DHAS_PAPI"
            LDFLAGS+=" -lpapi"
        fi
    fi
}

papi

for arg in "$@"; do
    if [ "$arg" == "--cflags" ]; then
        echo "$CFLAGS"
    elif [ "$arg" == "--ldflags" ]; then
        echo "$LDFLAGS"
    fi
done
