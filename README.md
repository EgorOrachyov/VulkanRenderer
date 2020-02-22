![Project logo](https://github.com/EgorOrachyov/Ignimbrite/blob/master/docs/pictures/Logo1020x240.png)

# Ignimbrite

![MacOS build](https://github.com/EgorOrachyov/VulkanRenderer/workflows/MacOS/badge.svg)
![Linux build](https://github.com/EgorOrachyov/VulkanRenderer/workflows/Linux/badge.svg)
![Windows build](https://github.com/EgorOrachyov/VulkanRenderer/workflows/Windows/badge.svg)

**C++11 real-time rendering library** for dynamic 3D scenes visualisation with 
basic drawing interface, configurable graphics and post-process pipelines. 
Provides high-level abstractions, such as material and mesh systems, as well 
as low-level interface to access graphics back-end. Uses Vulkan API as primary 
back-end. OpenGL 4 supports is planed in far future.

**Purpose of the project**: implement rendering library with low abstractions set 
and low overhead, provide user with a toolset for visualization technical 
or another data in real-time with high level of customization. 

**Note**: project under heavy development. It is not ready for usage.

## Features

### Rendering
* Back-end graphical interface
* Material system
* Mesh system
* Post-process effects manager
* Graphical pipeline
* Shader compilation and reflection

### Back-ends support
* Vulkan 1.0 on macOS, Window and Linux

## Platforms
* Windows
* Linux
* MacOS

## Toolchain
* C++11 with standard library
* CMake 3.11 or higher

## Third-party projects
* [Vulkan SDK](https://vulkan.lunarg.com)
* [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
* [GLFW](https://github.com/glfw/glfw) 
* [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross)
* [GLM](https://github.com/g-truc/glm)
* [Tiny .obj loader](https://github.com/syoyo/tinyobjloader)
* [STB Image](https://github.com/nothings/stb)

## Directory structure

* assets - various geometry and image data for testing
* code - actual library sources with depenencies
  * devices - backends for various graphics API
  * engine - library core 
  * modules - optional library modules
  * thirdparty - project code dependencies 
* docs - documents, text files and data for development/info
* shaders - shaders source files and compiled SPIR-V shaders
* test - contains test applications for various lib code testing
* CMakeLists.txt - root cmake file, add it as sub directory to your project for this project usage

## Get and build 

This project uses git as primary VCS and CMake 3.11 as build configuration tool.
Project language is C++11 with std library 11. The following commands allow to get 
repository:

```
$ git clone https://github.com/EgorOrachyov/Ignimbrite.git
$ cd Ignimbrite
```

Create build folder, configure and run build:

```
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_MODE=Debug
$ cmake --build .
```

Build with all enabled options (see cmake options below):

```
$ cmake .. -DCMAKE_BUILD_MODE=Debug -DIGNIMBRITE_WITH_GLFW=ON -DIGNIMBRITE_WITH_VULKAN=ON -DIGNIMBRITE_WITH_TESTS=ON 
$ cmake --build .
```

### CMake optional flags

We export some optional flags to configure cmake build and final
library image. You can set the following options to get desired functionality:

* IGNIMBRITE_WITH_GLFW=(YES/NO) - build glfw library to be able to create glfw based window applications.
* IGNIMBRITE_WITH_VULKAN=(YES/NO) - build vulkan device to be able to use vulkan graphics backend.
* IGNIMBRITE_WITH_TESTS=(YES/NO) - build tests executables with samples and test examples.

By default all the options are set in 'NO', including Vulkan and GLFW, because
they may require additional dependency setup process on you machine. 

### Dependencies setup

We try to keep as much dependencies managed inside project as we can. However, if you want to 
use vulkan as graphics backed, you will have to manually install Vulkan SDK from 
the official LunarG website.

Vulkan SDK [page](https://vulkan.lunarg.com/sdk/home) for Windows, Linux and MacOS. 
Follow the instructions, listed in the 'Getting started' manual page for each platform.

> Note: remember to export env variable VULKAN_SDK=/path/to/installed/vulkan/sdk, 
> because we use this variable to locate vulkan include .h files and binary libs on your
> machine in case, when cmake failed to find these files in standard system /include and /lib directories.

## License

This project is licensed under MIT license. 
Full text could be found in [license file](https://github.com/EgorOrachyov/Ignimbrite/blob/master/LICENSE.md).

## Contributors

* Egor Orachyov - [GitHub account](https://github.com/EgorOrachyov)
* Sultim Tsyrendashiev - [GitHub account](https://github.com/SultimTsyrendashiev)
