#!/bin/sh
glibtoolize --copy --force &&
autoreconf -if
./configure --with-jack CFLAGS="-I/opt/local/include" LDFLAGS="-L/opt/local/lib -L/opt/local/lib/jack"
make
sudo make install
echo '/opt/local/bin/jackd -dcoreaudio -p1024 -r44100 -P' >> ~/.jackdrc
