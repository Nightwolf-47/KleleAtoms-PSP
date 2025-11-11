set -e
mkdir -p build && cd build
psp-cmake ..
make clean
rm -v kleleatoms/*
