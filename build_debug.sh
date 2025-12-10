set -e
mkdir -p build && cd build
psp-cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_PRX=OFF -DENC_PRX=OFF
make

CMD_PYTHON=$(command -v python3 || command -v python)

if [ -n "${CMD_PYTHON}" ]; then
${CMD_PYTHON} ../paktool/kpaktool.py -p ../res -o kleleatoms/resources.pak
else
echo "Python not found, can't pack assets!"
exit 1
fi
