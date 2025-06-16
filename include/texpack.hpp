#ifndef TEXPACK_HPP
#define TEXPACK_HPP

#include <filesystem>
#include <Geode/Result.hpp>
#include <span>
#include <vector>

namespace texpack {
    /// A 2D point structure, representing a coordinate in a 2D space.
    struct Point {
        int x;
        int y;

        Point() : x(0), y(0) {}
        Point(int x, int y) : x(x), y(y) {}
        Point(uint32_t x, uint32_t y) : x(x), y(y) {}
        Point(const Point& other) : x(other.x), y(other.y) {}

        Point& operator=(const Point& other) {
            x = other.x;
            y = other.y;
            return *this;
        }

        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }

        /// Converts the point to a string representation.
        /// @returns A string in the format "{x,y}".
        std::string string() const {
            return "{" + std::to_string(x) + "," + std::to_string(y) + "}";
        }
    };

    /// A 2D size structure, representing the width and height of a rectangle.
    struct Size {
        int width;
        int height;

        Size() : width(0), height(0) {}
        Size(int width, int height) : width(width), height(height) {}
        Size(uint32_t width, uint32_t height) : width(width), height(height) {}
        Size(const Size& other) : width(other.width), height(other.height) {}

        Size& operator=(const Size& other) {
            width = other.width;
            height = other.height;
            return *this;
        }

        bool operator==(const Size& other) const {
            return width == other.width && height == other.height;
        }

        /// Converts the size to a string representation.
        /// @returns A string in the format "{width,height}".
        std::string string() const {
            return "{" + std::to_string(width) + "," + std::to_string(height) + "}";
        }
    };

    /// A 2D rectangle structure, defined by its origin point and size.
    struct Rect {
        Point origin;
        Size size;

        Rect() : origin(), size() {}
        Rect(int x, int y, int width, int height) : origin(x, y), size(width, height) {}
        Rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) : origin(x, y), size(width, height) {}
        Rect(const Point& origin, const Size& size) : origin(origin), size(size) {}
        Rect(const Rect& other) : origin(other.origin), size(other.size) {}

        Rect& operator=(const Rect& other) {
            origin = other.origin;
            size = other.size;
            return *this;
        }

        bool operator==(const Rect& other) const {
            return origin == other.origin && size == other.size;
        }

        /// Converts the rectangle to a string representation.
        /// @returns A string in the format "{{x,y},{width,height}}".
        std::string string() const {
            return "{{" + std::to_string(origin.x) + "," + std::to_string(origin.y)
                + "},{" + std::to_string(size.width) + "," + std::to_string(size.height) + "}}";
        }
    };

    /// A structure representing a frame in the texture atlas.
    struct Frame {
        std::string name;
        std::vector<uint8_t> data;
        Point offset;
        Size size;
        Rect rect;
        bool rotated;
    };

    /// A structure representing an image in RGBA8888 format.
    struct Image {
        std::vector<uint8_t> data;
        uint32_t width;
        uint32_t height;

        Image() : data(), width(0), height(0) {}
        Image(std::span<const uint8_t> data, uint32_t width, uint32_t height) : data(data.begin(), data.end()), width(width), height(height) {}
        Image(const Image& other) : data(other.data), width(other.width), height(other.height) {}
        Image(Image&& other) : data(std::move(other.data)), width(other.width), height(other.height) {
            other.width = 0;
            other.height = 0;
        }

        Image& operator=(const Image& other) {
            data = other.data;
            width = other.width;
            height = other.height;
            return *this;
        }
        Image& operator=(Image&& other) {
            data = std::move(other.data);
            width = other.width;
            height = other.height;
            other.width = 0;
            other.height = 0;
            return *this;
        }
    };

    /// A class for packing frames into a texture atlas, with maximum dimensions.
    class Packer {
    protected:
        std::vector<Frame> m_frames;
        Image m_image;
        int m_capacity;
    public:
        Packer(int capacity = 10000) : m_capacity(capacity) {}
        Packer(const Packer& other) : m_frames(other.m_frames), m_image(other.m_image), m_capacity(other.m_capacity) {}
        Packer(Packer&& other) : m_frames(std::move(other.m_frames)), m_image(std::move(other.m_image)), m_capacity(other.m_capacity) {
            other.m_capacity = 0;
        }

        Packer& operator=(const Packer& other) {
            m_frames = other.m_frames;
            m_image = other.m_image;
            m_capacity = other.m_capacity;
            return *this;
        }
        Packer& operator=(Packer&& other) {
            m_frames = std::move(other.m_frames);
            m_image = std::move(other.m_image);
            m_capacity = other.m_capacity;
            other.m_capacity = 0;
            return *this;
        }

        /// Adds a frame to the packer.
        /// @param name The name of the frame.
        /// @param data The pixel data of the frame, in RGBA8888 format.
        /// @param width The width of the frame.
        /// @param height The height of the frame.
        void frame(std::string_view name, std::span<const uint8_t> data, uint32_t width, uint32_t height);

        /// Adds a frame to the packer from an RGBA8888 image.
        /// @param name The name of the frame.
        /// @param image An RGBA8888 image.
        void frame(std::string_view name, const Image& image) {
            frame(name, image.data, image.width, image.height);
        }

        /// Adds a frame to the packer from PNG data.
        /// @param name The name of the frame.
        /// @param data The PNG data of the frame.
        /// @param premultiplyAlpha Whether to premultiply the alpha channel. (Default: false)
        /// @returns An error if the PNG data cannot be decoded.
        geode::Result<> frame(std::string_view name, std::span<const uint8_t> data, bool premultiplyAlpha = false);

        /// Adds a frame to the packer from a PNG file.
        /// @param name The name of the frame.
        /// @param path The path to the PNG file.
        /// @param premultiplyAlpha Whether to premultiply the alpha channel. (Default: false)
        /// @returns An error if the file cannot be opened or the PNG data cannot be decoded.
        geode::Result<> frame(std::string_view name, const std::filesystem::path& path, bool premultiplyAlpha = false);

        /// Gets a frame from the packer by its name.
        /// @param name The name of the frame.
        /// @returns A reference to the frame, or an error if the frame is not found.
        geode::Result<Frame&> frame(std::string_view name);

        /// Gets a frame from the packer by its name.
        /// @param name The name of the frame.
        /// @returns A constant reference to the frame, or an error if the frame is not found.
        geode::Result<const Frame&> frame(std::string_view name) const;

        /// Gets the frames managed by the packer.
        /// @returns A reference to the vector of frames.
        std::vector<Frame>& frames() { return m_frames; }

        /// Gets the frames managed by the packer.
        /// @returns A constant reference to the vector of frames.
        const std::vector<Frame>& frames() const { return m_frames; }

        /// Gets the packed image.
        /// @returns A reference to the packed image.
        Image& image() { return m_image; }

        /// Gets the packed image.
        /// @returns A constant reference to the packed image.
        const Image& image() const { return m_image; }

        /// Finalizes the packing process, arranging the frames into a texture atlas.
        /// @returns An error if the packing process fails.
        geode::Result<> pack();

        /// Generates a property list representation of the frames.
        /// @param name The name of the texture atlas.
        /// @param indent The string used for indentation in the property list. (Default: "\t")
        /// @returns A string containing the property list representation of the frames.
        std::string plist(const std::string& name, std::string_view indent = "\t");

        /// Saves the property list representation of the frames to a file.
        /// @param path The path to the file where the property list will be saved.
        /// @param name The name of the texture atlas.
        /// @param indent The string used for indentation in the property list. (Default: "\t")
        /// @returns An error if the file cannot be opened or written to.
        geode::Result<> plist(const std::filesystem::path& path, const std::string& name, std::string_view indent = "\t");

        /// Generates a PNG representation of the packed frames.
        /// @returns A vector of bytes containing the PNG data, or an error if the encoding fails.
        geode::Result<std::vector<uint8_t>> png();

        /// Saves the PNG representation of the packed frames to a file.
        /// @param path The path to the file where the PNG will be saved.
        /// @returns An error if the encoding fails or the file cannot be opened.
        geode::Result<> png(const std::filesystem::path& path);
    };

    /// Creates an RGBA8888 image from PNG data.
    /// @param data The PNG data.
    /// @param premultiplyAlpha Whether to premultiply the alpha channel. (Default: false)
    /// @returns The decoded image, or an error if the decoding fails.
    geode::Result<Image> fromPNG(std::span<const uint8_t> data, bool premultiplyAlpha = false);

    /// Creates an RGBA8888 image from a PNG file.
    /// @param path The path to the PNG file.
    /// @param premultiplyAlpha Whether to premultiply the alpha channel. (Default: false)
    /// @returns The decoded image, or an error if the file cannot be opened or the decoding fails.
    geode::Result<Image> fromPNG(const std::filesystem::path& path, bool premultiplyAlpha = false);

    /// Creates a PNG representation of the given pixel data.
    /// @param data The pixel data in RGBA8888 format.
    /// @param width The width of the image.
    /// @param height The height of the image.
    /// @returns A vector of bytes containing the PNG data, or an error if the encoding fails.
    geode::Result<std::vector<uint8_t>> toPNG(std::span<const uint8_t> data, uint32_t width, uint32_t height);

    /// Creates a PNG representation of the given imagw.
    /// @param image An RGBA8888 image.
    /// @returns A vector of bytes containing the PNG data, or an error if the encoding fails.
    inline geode::Result<std::vector<uint8_t>> toPNG(const Image& image) {
        return toPNG(image.data, image.width, image.height);
    }

    /// Saves the PNG representation of the given pixel data to a file.
    /// @param path The path to the file where the PNG will be saved.
    /// @param data The pixel data in RGBA8888 format.
    /// @param width The width of the image.
    /// @param height The height of the image.
    /// @returns An error if the encoding fails or the file cannot be opened.
    geode::Result<> toPNG(const std::filesystem::path& path, std::span<const uint8_t> data, uint32_t width, uint32_t height);

    /// Saves the PNG representation of the given image to a file.
    /// @param path The path to the file where the PNG will be saved.
    /// @param image An RGBA8888 image.
    /// @returns An error if the encoding fails or the file cannot be opened.
    inline geode::Result<> toPNG(const std::filesystem::path& path, const Image& image) {
        return toPNG(path, image.data, image.width, image.height);
    }
}

#endif
