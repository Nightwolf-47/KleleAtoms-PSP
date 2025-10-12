mkdir -p build && cd build
psp-cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_PRX=OFF -DENC_PRX=OFF
make
rm -rf kleleatoms/res
cp -r ../res kleleatoms
