#!/bin/bash

#autoscan && libtoolize --force --automake --copy && aclocal -I build && autoheader && autoconf && automake --add-missing --copy

failure() {
        echo "Oops, $* failed ;(" >&2
        exit 1
}

git clean -x -f -d -e ./ps -e .ccache/ -e tests/testrun/ -e times.log || failure "cleanup"

NBD="--disable-ndb"
MYSQL_CLUSTER="$(find -L /opt /usr/local -maxdepth 2 -name 'mysql-cluster*' -type d | sort -r | head -1)"
if [ -n "${MYSQL_CLUSTER}" -a -x ${MYSQL_CLUSTER}/bin/mysql_config ]; then
	echo "MYSQL_CLUSTER	= ${MYSQL_CLUSTER}"
	PATH=${MYSQL_CLUSTER}/bin:$PATH
fi
MYSQL_CONFIG="$(which mysql_config)"
if [ -n "${MYSQL_CONFIG}" ]; then
	echo "MYSQL_CONFIG	= ${MYSQL_CONFIG}"
	MYSQL_NDB_API="$(${MYSQL_CONFIG} --include | sed 's|-I/|/|g')/storage/ndb/ndbapi/NdbApi.hpp"
	if [ -s "${MYSQL_NDB_API}" ]; then
		echo "MYSQL_NDB_API	= ${MYSQL_NDB_API}"
		NBD="--enable-ndb"
	fi
fi

IODBC=$([ -d /usr/include/iodbc ] && echo "-I/usr/include/iodbc")
CFLAGS="-Wall -g -Os -DLDAP_MEMORY_DEBUG -DUSE_VALGRIND $IODBC" CPPFLAGS="$CFLAGS" \
	./configure \
		--enable-backends --enable-overlays $NBD \
		--enable-rewrite --enable-dynacl --enable-aci --enable-slapi \
		--disable-dependency-tracking \
	|| failure "configure"

make depend && make -j4 && make -j4 -C libraries/liblmdb || failure "build"

echo "DONE"
