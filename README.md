edgex-device-v4l2camera

About
The EdgeX Device v4lcamera Service is developed to communicate to v4l camera connected to the I2C bus in an EdgeX deployment

Dependencies:

Acquire the video 4 linux 2 development libraries (v4l2)

 $ sudo apt-get install libv4l-dev
 $ sudo apt-get install v4l-utils
 $ sudo apt-get install libjpeg

See https://github.com/bellbind/node-v4l2camera/wiki/V4l2-programming for general workflow with the v4l2 libraries.

Build Instruction:

Build/install device-c-sdk library and headers by using the following command

sh> ./scripts/build_deps.sh
sh> ./scripts/build.sh

By default, the configuration and profile file used by the service are available in 'res' folder.

Configuration:

Port number /dev/video0 specified in the configuration.toml
Configure Auto Event in the configuration.toml

Description

The code will initiate a H264 stream from the camera device located at /dev/video0.
It will capture snapshot at regular intervals based on auto event configuration.

Note:
Tested with Logitech c930e web camera
