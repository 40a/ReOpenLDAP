#!/bin/bash

#autoscan && libtoolize --force --automake --copy && aclocal -I build && autoheader && autoconf && automake --add-missing --copy

failure() {
        echo "Oops, $* failed ;(" >&2
        exit 1
}

git clean -x -f -d -e ./ps -e .ccache/ -e tests/testrun/ -e times.log || failure "cleanup"

CC=clang CFLAGS="-Wall -g -Oz" CPPFLAGS="$CFLAGS" \
	./configure \
        --prefix=/opt/openldap.devel --enable-dynacl --enable-ldap \
        --enable-overlays --disable-bdb --disable-hdb \
        --disable-dynamic --disable-shared --enable-static \
	|| failure "configure"

make depend || failure "depend"
echo "=======================================================================" && sleep 1
make -j4 && make -j4 -C libraries/liblmdb || failure "build"
echo "DONE"
