# Tested on ubuntu 12.04 and 14.04, you might be able to use something similar
# to build it on windows using mingw... 
# Putting the instructions here in case someone might find it useful
sudo apt-get install git automake autoconf make mingw-w64-x86-64-dev mingw-w64-tools mingw-w64
wget http://curl.haxx.se/download/curl-7.40.0.tar.gz
wget ftp://sourceware.org/pub/pthreads-win32/pthreads-w32-2-9-1-release.tar.gz
wget https://gmplib.org/download/gmp/gmp-6.2.1.tar.xz
git clone https://github.com/TR07SKY/xPI-cpuminer
tar zxf curl-7.40.0.tar.gz && tar zxf pthreads-w32-2-9-1-release.tar.gz && tar xf gmp-6.2.1.tar.xz

mkdir win64_deps

DEPS="${PWD}/win64_deps"

# build and install curl
cd curl-7.40.0
./configure --with-winssl --enable-static --prefix=$DEPS --host=x86_64-w64-mingw32 --disable-shared
make install

#build and install gmp
cd gmp-6.2.1
./configure --enable-static --prefix=$DEPS --host=x86_64-w64-mingw32 --disable-shared
make install

# build and install win32 pthreads
# note: if running 14.04 it might link against its internal winpthreads, this
# usually causes cpuminer threads to immediately exit. So make sure it links your 
# pthread build
cd ../pthreads-w32-2-9-1-release/
cp config.h pthreads_win32_config.h
make -f GNUmakefile CROSS="x86_64-w64-mingw32-" clean GC-static
cp libpthreadGC2.a ${DEPS}/lib/libpthread.a
cp pthread.h semaphore.h sched.h ${DEPS}/include

# build cpuminer
cd ../cpuminer
autoreconf -fi -I${DEPS}/share/aclocal
./configure --host=x86_64-w64-mingw32 \
CFLAGS="-DWIN32 -DCURL_STATICLIB -O3 -std=c99 -I${DEPS}/include -DPTW32_STATIC_LIB" \
--with-libcurl=${DEPS} LDFLAGS="-static -L${DEPS}/lib"
make

# Tada! Now just copy minerd.exe to your windows pc
