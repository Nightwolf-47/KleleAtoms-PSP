mkdir -p build && cd build
psp-cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_PRX=ON -DENC_PRX=ON
make
rm -rf kleleatoms/res
cp -r ../res kleleatoms
