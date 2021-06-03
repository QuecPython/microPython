#!/bin/sh

py_ver=$(python --version 2> /dev/null | sed -n '/Python 3/p')
py3_ver=$(python3 --version 2> /dev/null | sed -n '/Python 3/p')

if [ ! -n "$py_ver" ]; then
    if [ ! -n "$py3_ver" ]; then
        echo "! No Python3.x found in system environment."
    else
        py_cmd=python3
    fi
else
    py_cmd=python
fi

if [ "$1" = "clean" ]; then
    make -C ports/quectel/core --no-print-directory -f Makefile clean PYTHON=$py_cmd
else
    make -C ports/quectel/core --no-print-directory -f Makefile all V=1 PYTHON=$py_cmd
fi
