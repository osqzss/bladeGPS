# bladeGPS

Very crude experimental implementation of [gps-sdr-sim](https://github.com/osqzss/gps-sdr-sim) for real-time signal generation.
The code works with bladeRF and has been tested on Windows 10 and Ubuntu 20.04.

```
Usage: bladegps [options]
Options:
  -e <gps_nav>     RINEX navigation file for GPS ephemerides (required)
  -y <yuma_alm>    YUMA almanac file for GPS almanacs
  -u <user_motion> User motion file (dynamic mode)
  -g <nmea_gga>    NMEA GGA stream (dynamic mode)
  -l <location>    Lat,Lon,Hgt (static mode) e.g. 35.274,137.014,100
  -t <date,time>   Scenario start time YYYY/MM/DD,hh:mm:ss
  -T <date,time>   Overwrite TOC and TOE to scenario start time
  -d <duration>    Duration [sec] (max: 86400)
  -x <XB_number>   Enable XB board, e.g. '-x 200' for XB200
  -i               Interactive mode: North='w', South='s', East='d', West='a'
  -I               Disable ionospheric delay for spacecraft scenario
  -p               Disable path loss and hold power level constant
```

### Build on Windows with Visual Studio

Follow the instructions at [Nuand wiki page](https://github.com/Nuand/bladeRF/wiki/Getting-Started%3A-Windows) and build the bladeRF library from the source with Visual Studio 2013 Express for Windows Desktop. Assume you already downloaded pthread and libusb files and successfully built the bladeRF library for your Windows environment.

1. Start Visual Studio
2. Create an empty project for a console application
3. On the _Solution Explorer_ at right, add the following files to the project:
    * __bladegps.c__ and __bladegps.h__
    * __gpssim.c__ and __gpssim.h__
    * __getopt.c__ and __getopt.h__
4. Add the paths to the following folders in `Configuration Properties -> C/C++ -> General -> Additional Include Directories`:
    * __pthreads-w32-2-9-1-release/Pre-built.2/include__ for pthread.h
    * __bladeRF/include__ for libbladeRF.h
5. Add the paths to the following folders in `Configuration Properties -> Linker -> General -> Additional Library Directories`:
    * __pthreads-w32-2-9-1-release/Pre-built.2/lib/x64__ for pthreadVC2.lib
    * __bladeRF/x64__ for bladeRF.lib
6. Specify the name of the additional libraries in `Configuration Properties -> Linker -> Input -> Additional Dependencies`:
    * __pthreadVC2.lib__
    * __bladeRF.lib__
7. Select __Release__ in the _Solution Configurations_ drop-down list
8. Select __X64__ in the _Sofution Platforms_ drop-down list
9. Run `Build -> Build Solution`

After a successful build, you can find the executable in the __Release__ folder. You should put the copies of the following DLLs in the same folder to run the code:
* __bladeRF.dll__
* __libusb-1.0.dll__
* __pthreadVC2.dll__

### Build on Linux (Ubuntu 20.04)

1. Retrieve the bladeRF source in a directory next to the current directory

        $ cd ..
        $ git clone git@github.com:Nuand/bladeRF.git

2. Build the bladeRF host library

        $ cd bladeRF/host
        $ mkdir build
        $ cd build
        $ cmake ..
        $ make

3. Build bladeGPS

        $ cd ../../../bladeGPS
        $ make

### Build on Mac OS X (Catalina)

1. Install [Xcode](https://apps.apple.com/us/app/xcode/id497799835?mt=12)

2. Install Xcode Command Line Tools

        $ xcode-select --install

3. Install [MacPorts](https://www.macports.org/install.php)

4. Create a symlink to port

        $ sudo ln -s /opt/local/bin/port /usr/local/bin/port

5. Install bladeRF

        $ sudo port install bladeRF +tecla

6. Install cmake

        $ sudo port install cmake

7. Retrieve the bladeRF source in a directory next to the current directory

        $ cd ..
        $ git clone git@github.com:Nuand/bladeRF.git

8. Build the bladeRF host library

        $ cd bladeRF/host
        $ mkdir build
        $ cd build
        $ cmake ..
        $ make && sudo make install

9. Install libomp and wget

        $ sudo port install libomp
        $ sudo port install wget

10. Compile for \_MACOSX with OpenMP path and build bladeGPS

        $ cd ../../../bladeGPS
        $ make CFLAGS=”-I/opt/local/include/libomp _MACOSX”

### Docker 

A `Dockerfile` was created from Ubuntu 20.04 image that allows you to spin up 
a [docker container](https://docs.docker.com/get-started/overview/) in seconds. 

1. Build docker image

        $ cd docker
        $ docker build -t bladegps .

2. Run bladegps command directly

        $ docker run -it --rm --privileged --device /dev/ttyUSB0 bladegps \
                /bin/bash -c "bladegps -e /opt/bladeGPS/brdc3300.18n -l 35.274,137.014,100 -d 86400"

3. Run bladegps command from docker cli

        $ docker run -it --rm --privileged --device /dev/ttyUSB0 bladegps /bin/bash
        bladegps@cb744220dea3:~$ bladegps -e /opt/bladeGPS/brdc3300.18n -l 35.274,137.014,100 -d 86400

### Docker Compose

A `docker-compose.yml` example is provided, if you want to run bladegps as a 
service, restarting when it hits end of duration. Edit command and the device 
name before starting up service using [docker-compose](https://docs.docker.com/compose/).

1. Run docker-compose service in background

        $ cd docker
        $ docker-compose up -d

2. Check docker-compose process status

        $ docker-compose ps

3. Connect to container running service, if desired

        $ docker-compose exec bladegps /bin/bash

4. Down the service when done

        $ docker-compose down

### License

Copyright &copy; 2015-2022 Takuji Ebinuma  
Distributed under the [MIT License](http://www.opensource.org/licenses/mit-license.php).
