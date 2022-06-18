# Gathering

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

### Third-party libraries (included by git submodules)
* Dear ImGui - https://github.com/ocornut/imgui
* OpenGL Mathematics (GLM) - https://github.com/g-truc/glm
* GLFW - https://github.com/glfw/glfw

## Build
### CMake options
(temporary)

|Option                          |Description|Default|
|--------------------------------|---|---|
|``GATHERING_CREATE_DOCS=ON``  |Create doxygen documentation|``OFF``|
|``GATHERING_BUILD_SAMPLES=ON``|Build examples|``ON``|
|``GATHERING_DEBUGPRINTS=ON``  |Debug prints enabled|``ON``|
|``GATHERING_AUTO_HEADLESS=ON``|Automatically switches to headless simuluation when manually closing a window|``ON``|

## Camera controls
...

## Credits / Attributions
* OpenGL is a trademark of the [Khronos Group Inc.](http://www.khronos.org)
