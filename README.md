# Gathering

![example](https://raw.githubusercontent.com/wiki/mc-thulu/gathering/web/sides.png)

## Download
This repository contains submodules for external dependencies. When doing a fresh clone, make sure you clone recursively:
```
git clone --recursive https://github.com/mc-thulu/gathering.git
```
Updating an already cloned repository:
```
git submodule init
git submodule update
```

### Requirements
* OpenGl 4.6
* Python 3.6+ (optional)
* Doxygen (optional)

### Third-party libraries (included by git submodules)
* Dear ImGui - https://github.com/ocornut/imgui
* OpenGL Mathematics (GLM) - https://github.com/g-truc/glm
* GLFW - https://github.com/glfw/glfw
* PyBind11 - https://github.com/pybind/pybind11
* Eigen - https://eigen.tuxfamily.org/

## Build
### CMake options
(temporary)

|Option                          |Description|Default|
|--------------------------------|---|---|
|``GATHERING_CREATE_DOCS=ON``  |Create doxygen documentation|``OFF``|
|``GATHERING_BUILD_SAMPLES=ON``|Build examples|``ON``|
|``GATHERING_DEBUGPRINTS=ON``  |Debug prints enabled|``ON``|
|``GATHERING_AUTO_HEADLESS=ON``|Automatically switches to headless simuluation when manually closing a window|``ON``|
|``GATHERING_PYBIND=ON``|Build a MODULE library for python using _PyBind11_|``ON``|

## Credits / Attributions
* OpenGL is a trademark of the [Khronos Group Inc.](http://www.khronos.org)
