#!/bin/bash

failure() {
        echo "Oops, $* failed ;(" >&2
        exit 1
}

step_begin() {
	echo "##teamcity[blockOpened name='$1']"
	#echo "##teamcity[progressStart '$1']"
}

step_finish() {
	sleep 1
	#echo "##teamcity[progressEnd '$1']"
	echo "##teamcity[blockClosed name='$1']"
}

step_begin "cleanup"
git clean -x -f -d -e .ccache/ -e tests/testrun/ -e times.log || failure "cleanup"
step_finish "cleanup"
echo "======================================================================="
step_begin "prepare"

BUILD_NUMBER=${1:-$(date '+%y%m%d')}
PREFIX=$(readlink -m ${2:-$(pwd)/install_prefix_as_the_second_parameter}/openldap)

echo "BUILD_NUMBER: $BUILD_NUMBER"
echo "PREFIX: $PREFIX"

git fetch origin --prune --tags || failure "git fetch"
BUILD_ID=$(git describe --abbrev=15 --always --long --tags | sed -e "s/^.\+-\([0-9]\+-g[0-9a-f]\+\)\$/.${BUILD_NUMBER}-\1/" -e "s/^\([0-9a-f]\+\)\$/.${BUILD_NUMBER}-g\1/")$(git show --abbrev=15 --format=-t%t | head -n 1)
echo "BUILD_ID: $BUILD_ID"

step_finish "prepare"
echo "======================================================================="
step_begin "configure"

LDFLAGS="-Wl,--as-needed,-Bsymbolic,--gc-sections,-O,-zignore"
CFLAGS="-Wall -g -fvisibility=hidden -Os -include $(readlink -f $(dirname $0)/ps/glibc-225.h)"
LIBS="-Wl,--no-as-needed,-lrt"

if [ -n "$(which gcc)" ] && gcc -v 2>&1 | grep -q -i lto \
	&& [ -n "$(which gcc-ar)" -a -n "$(which gcc-nm)" -a -n "$(which gcc-ranlib)" ]
then
#	gcc -fpic -O3 -c ps/glibc-225.c -o ps/glibc-225.o
#	CFLAGS+=" -D_LTO_BUG_WORKAROUND -save-temps"
#	LDFLAGS+=",$(readlink -f ps/glibc-225.o)"
	echo "*** Link-Time Optimization (LTO) will be used" >&2
	CFLAGS+=" -free -flto -fno-fat-lto-objects -flto-partition=none"
	export CC=gcc AR=gcc-ar NM=gcc-nm RANLIB=gcc-ranlib
fi

export CFLAGS LDFLAGS LIBS CXXFLAGS="$CFLAGS"

./configure \
	--prefix=${PREFIX} --enable-dynacl --enable-ldap \
	--enable-overlays --disable-bdb --disable-hdb \
	--disable-dynamic --disable-shared --enable-static \
	--enable-debug --with-gnu-ld --without-cyrus-sasl \
	--disable-spasswd --disable-lmpasswd \
	--without-tls --disable-rwm --disable-relay \
	|| failure "configure"

PACKAGE="$(grep VERSION= Makefile | cut -d ' ' -f 2).${BUILD_NUMBER}"
echo "PACKAGE: $PACKAGE"

mkdir -p ${PREFIX}/bin \
	&& (git log --no-merges --dense --date=short --pretty=format:"%ad %s" $(git describe --always --abbrev=0 --tags HEAD^1~1).. \
		| tr -s ' ' ' ' | grep -v ' ITS#[0-9]\{4\}$' | sort -r | uniq -u \
	&& /bin/echo -e "\nPackage version: $PACKAGE\nSource code tag: $(git describe --abbrev=15 --long --always --tags)" ) > ${PREFIX}/changelog.txt \
	|| failure "fix-1"

find ./ -name Makefile -type f | xargs sed -e "s/STRIP = -s/STRIP =/g;s/\(VERSION= .\+\)/\1${BUILD_ID}/g" -i \
	|| failure "fix-2"

find ./ -name Makefile | xargs -r sed -i 's/-Wall -g/-Wall -Werror -g/g' \
	|| failure "fix-3"

step_finish "configure"
echo "======================================================================="
step_begin "build mdb-tools"

(cd libraries/liblmdb && make -k && cp mdb_chk mdb_copy mdb_stat -t ${PREFIX}/bin/) \
	|| failure "build-1"

step_finish "build mdb-tools"
echo "======================================================================="
step_begin "build openldap"

if [ -z "${TEAMCITY_PROCESS_FLOW_ID}" ]; then
	make depend \
		|| failure "make-deps"
fi

make -k \
	|| failure "build-2"

step_finish "build openldap"
echo "======================================================================="
step_begin "install"

make -k install \
	|| failure "install"

step_finish "install"
echo "======================================================================="
step_begin "sweep"

find ${PREFIX} -name '*.a' -o -name '*.la' | xargs -r rm \
	|| failure "sweep-1"
find ${PREFIX} -type d -empty | xargs -r rm -r \
	|| failure "sweep-2"
rm -r ${PREFIX}/var ${PREFIX}/include && ln -s /var ${PREFIX}/ \
	|| failure "sweep-3"

step_finish "sweep"
echo "======================================================================="
step_begin "packaging"

FILE="openldap.$PACKAGE.tar.xz"
tar -caf $FILE --owner=root -C ${PREFIX}/.. openldap \
	&& sleep 1 && cat ${PREFIX}/changelog.txt >&2 && sleep 1 \
	&& ([ -n "$2" ] && echo "##teamcity[publishArtifacts '$FILE']" \
		|| echo "Skip publishing of artifact ($(ls -hs $FILE))" >&2) \
	|| failure "tar"

step_finish "packaging"
