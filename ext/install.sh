#!/bin/bash

basedir=$(cd $(dirname $0); pwd)

check_executable () {
  for executable; do
    command -v $executable >/dev/null 2>&1 || {
      echo >&2 "Unable to find '$executable', make sure it is installed and is in the PATH.  Aborting."
      exit 1
    }
  done
}

check_executable cmake grep sed

set -ex

rm -Rf /tmp/php-driver-installation
mkdir /tmp/php-driver-installation
pushd /tmp/php-driver-installation

mkdir build
builddir=$(cd build; pwd)

echo "Compiling cpp-driver..."
mkdir cpp-driver
pushd cpp-driver
cmake -DCMAKE_CXX_FLAGS="-fPIC" -DCMAKE_INSTALL_PREFIX:PATH=$builddir -DCASS_BUILD_STATIC=ON \
  -DCASS_BUILD_SHARED=OFF -DCMAKE_BUILD_TYPE=RELEASE -DCASS_USE_ZLIB=ON \
  -DCMAKE_INSTALL_LIBDIR:PATH=lib $basedir/../lib/cpp-driver/
make
sudo make install
popd
sudo chown -R $(whoami) $builddir
rm -Rf cpp-driver
rm -f build/lib/libcassandra.{dylib,so}
mv build/lib/libcassandra_static.a build/lib/libcassandra.a

popd

echo "Compiling and installing the extension..."
pushd $basedir
phpize
echo ./configure --with-cassandra=$builddir --with-libdir=lib
LIBS="-lssl -lcrypto -lz -luv -lm -lstdc++" LDFLAGS="-L$builddir/lib" ./configure --with-cassandra=$builddir --with-libdir=lib
make
sudo make install
popd

rm -Rf /tmp/php-driver-installation
