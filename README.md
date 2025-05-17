# texpack
A C++ library for combining textures into Cocos-style spritesheets.

## Inclusion
This library can be included using [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake).
```cmake
# It is highly recommended to use a specific commit hash instead of main
CPMAddPackage("gh:hiimjasmine00/texpack#main")

# Add the library to your target
target_link_libraries(${PROJECT_NAME} texpack)
```

If you are using this library with [Geode SDK](https://github.com/geode-sdk/geode), you have to add the following line to your `CMakeLists.txt` before `add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)`, preferably at the top:
```cmake
# Turn off code generation for pugixml
set(GEODE_CODEGEN_EXTRA_ARGS "--skip-pugixml")
```

## Usage
Full documentation is available in [texpack.hpp](./include/texpack.hpp).
```cpp
// Include the library
#include <texpack.hpp>

// Create a new packer, with a maximum size of 10000x10000
texpack::Packer packer(10000);

// Add a texture to the packer
packer.frame("example", "path/to/texture.png");

// Pack the textures into a spritesheet
packer.pack();

// Save the spritesheet to a PNG file
packer.png("path/to/spritesheet.png");

// Save the spritesheet to a property list file
packer.plist("path/to/spritesheet.plist");
```

## License
This library is licensed under the [MIT License](./LICENSE).