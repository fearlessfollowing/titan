linux compile:
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DOPENCV_EXTRA_MODULES_PATH=/home/nvidia/work/opencv/opencv_contrib/modules -DCMAKE_INSTALL_PREFIX=/home/nvidia/work/inspro/external ..
make -j8
make install
