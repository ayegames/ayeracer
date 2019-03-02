./autogen.sh
mkdir -p ayebuild
pushd ayebuild
../configure
make && echo Please execute 'sudo make install' to install && popd && exit
popd && echo ayebuild: Error during build
