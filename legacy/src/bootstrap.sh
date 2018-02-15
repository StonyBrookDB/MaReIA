sudo apt-get -y install build-essential cmake pkg-config

sudo apt-get -y install libjpeg62-dev 
sudo apt-get -y install libtiff4-dev libjasper-dev

sudo apt-get -y install  libgtk2.0-dev

sudo apt-get -y install libavcodec-dev libavformat-dev libswscale-dev libv4l-dev

sudo apt-get -y install libdc1394-22-dev

sudo apt-get -y install libxine-dev libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev 

sudo apt-get -y install python-dev python-numpy
 
sudo apt-get -y install libtbb-dev

sudo apt-get -y install libqt4-dev

wget http://sourceforge.net/projects/opencvlibrary/files/opencv-unix/2.4.5/opencv-2.4.5.tar.gz

# tar -xvf opencv-2.4.5.tar.gz

cd opencv-2.4.5
mkdir build
cd build
#cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local -D WITH_TBB=ON -D #BUILD_NEW_PYTHON_SUPPORT=ON -D WITH_V4L=ON -D INSTALL_C_EXAMPLES=ON -D INSTALL_PYTHON_EXAMPLES=ON 
#    -D BUILD_EXAMPLES=ON -D WITH_QT=ON -D WITH_OPENGL=ON ..
cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install

wget http://sourceforge.net/projects/boost/files/boost/1.54.0/boost_1_54_0.tar.bz2
