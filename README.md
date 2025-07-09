# texpack
A C++ library for combining textures into Zwoptex spritesheets. (If you don't know what those are, they're used by Cocos2d-x v2 and other game engines.)

## Inclusion
This library can be included using [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake).
```cmake
# Add texpack to your project
CPMAddPackage("gh:hiimjasmine00/texpack@0.4.0")

# Add the library to your target
target_link_libraries(${PROJECT_NAME} texpack)
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
packer.plist("path/to/spritesheet.plist", "spritesheet.png");
```

## License
This library is licensed under the [MIT License](./LICENSE).