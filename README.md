# SDF plugin for CoppeliaSim

Can import [SDF 1.6](http://sdformat.org/spec?ver=1.6) files. Some feature are not implemented yet.

### Compiling

1. Install required packages for [libPlugin](https://github.com/CoppeliaRobotics/libPlugin): see libPlugin's [README](external/libPlugin/README.md)
2. Download and install Qt (same version as CoppeliaSim, i.e. 5.5.0)
3. Checkout and compile
```
$ git clone --recursive https://github.com/CoppeliaRobotics/simExtSDF.git
$ cmake .
$ cmake --build .
```
