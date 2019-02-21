1. linux compile
export CFLAGS="" //不先执行这个，make的时候会报错
./configure --build aarch64-unknown-linux-gnu --disable-video --without-imagemagick --without-gtk --without-qt --without-jpeg --without-python --without-x --prefix=/usr/local
make -j8
sudo make install

2.源码：
官网：http://zbar.sourceforge.net/download.html
github：
github的没有configure，只有config.ac，但是autoconf会报错，所以暂时没有编译过



