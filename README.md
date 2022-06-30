# NetFramework
[![C++](https://img.shields.io/badge/Language-c%2B%2B20-blue?style=plastic&logo=cplusplus)](https://en.cppreference.com/w/cpp/20)
<br>
[![SMB](https://img.shields.io/badge/Protocol-SMB-yellowgreen?style=plastic)](https://en.wikipedia.org/wiki/Server_Message_Block)
[![DLNA](https://img.shields.io/badge/Protocol-DLNA-yellowgreen?style=plastic)](https://en.wikipedia.org/wiki/Digital_Living_Network_Alliance)
<br>
[![Windows](https://img.shields.io/badge/Platform-Windows-9cf?style=plastic)](https://en.wikipedia.org/wiki/Microsoft_Windows)
[![Linux](https://img.shields.io/badge/Platform-Linux-9cf?style=plastic)](https://en.wikipedia.org/wiki/Linux)
[![Android](https://img.shields.io/badge/Platform-Android-9cf?style=plastic)](https://en.wikipedia.org/wiki/Android_(operating_system))
<br>
[![x86-64](https://img.shields.io/badge/Arch-x86--64-brightgreen?style=plastic)](https://en.wikipedia.org/wiki/X86-64)
[![Arm32](https://img.shields.io/badge/Arch-Arm32-brightgreen?style=plastic)](https://en.wikipedia.org/wiki/ARM_architecture_family)


## Feature
* ***Discover*** - search devices
* ***Browser*** - list files and directories
* ***Manager*** - download files
* ***Customized Protocol*** - add any protocol like ```file://``` or ```http://``` to ```source/protocol/$Any_protocol.cpp```, and register in ```protocol.cpp```

## Clone
```
git clone --recursive https://github.com/Arlen-LT/NetFramework.git
```

## Build
On Windows, `MSVC cl.exe v143` or higher is required. The best way is to download Visual Studio 2022 and then:
```
devenv ./NetFramework
```
On Linux or WSL2, `clang 14.0.5` and `gcc 10.0` have passed the test. Recommend to open the project by VS Code:
```
code ./NetFramework
```
Then use `cmake >= 3.16` to build:
```
cd ./NetFramework
cmake -S . -B build
```

## TODO
* 


