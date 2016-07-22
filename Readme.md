# bladeGPS

Very crude experimental implimentation of [gps-sdr-sim](https://github.com/osqzss/gps-sdr-sim) for real-time signal generation.
The code works with bladeRF and has been tested on Windows only.

```
Usage: bladegps [options]
Options:
  -e <gps_nav>     RINEX navigation file for GPS ephemerides (required)
  -u <user_motion> User motion file (dynamic mode)
  -g <nmea_gga>    NMEA GGA stream (dynamic mode)
  -l <location>    Lat,Lon,Hgt (static mode) e.g. 35.274,137.014,100
  -t <date,time>   Scenario start time YYYY/MM/DD,hh:mm:ss
  -d <duration>    Duration [sec] (max: 86400)
  -x <XB_number>   Enable XB board, e.g. '-x 200' for XB200
  -i               Interactive mode: North='w', South='s', East='d', West='a'
```

### Additional include files and libraries

1. libbladeRF.h and bladeRF.lib (build from the [source](https://github.com/Nuand/bladeRF))
2. pthread.h and pthreadVC2.lib (available from [sourceware.org/pthreads-win32](https://sourceware.org/pthreads-win32/))


### Build on Windows - Instructions Courtesy of OSQZSS

You can find the instructions to build the bladeRF library from the source with Visual Studio 2013 Express for Windows Desktop in the Nuand wiki page:
https://github.com/Nuand/bladeRF/wiki/Getting-Started%3A-Windows

Assume you already downloaded pthread and libusb files and successfully built the bladeRF library.

In order to build bladeGPS, you need to add the paths to the following folders in Configuration Properties -> C/C++ -> General -> Additional Include Directoris:

“pthreads-w32-2-9-1-release/Pre-built.2/include” for pthread.h
“bladeRF/include” for libbladeRF.h
You also need to add the paths to the following folders in Configuration Properties -> Linker -> General -> Additional Library Directories:

“pthreads-w32-2-9-1-release/Pre-built.2/lib/x64” for pthreadVC2.lib
“bladeRF/x64” for bladeRF.lib
For the link command, specify the name of the additional libraries in Configuration Properties -> Linker -> Input -> Additional Dependencies:

pthreadVC2.lib
bladeRF.lib
Basically that's it! Now you should be able to build the code. In order to execute bladeGPS, you need the following DLLs in the same folder:

bladeRF.dll
libusb-1.0.dll
pthreadVC2.dll

### Build on Linux (Untested)

1. Retrive the bladeRF source in a directory next to the current directory.

 ```
$ cd ..
$ git clone git@github.com:Nuand/bladeRF.git
```

2. Build the bladeRF host library.

 ```
$ cd bladeRF/host
$ mkdir build
$ cd build
$ cmake ..
$ make
```

3. Build bladeGPS.

 ```
$ cd ../../../bladeGPS
$ make
```

### License

Copyright &copy; 2015 Takuji Ebinuma  
Distributed under the [MIT License](http://www.opensource.org/licenses/mit-license.php).
