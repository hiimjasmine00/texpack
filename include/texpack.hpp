#ifndef TEXPACK_HPP
#define TEXPACK_HPP

#include <filesystem>
#include <Geode/Result.hpp>
#include <vector>

namespace texpack {
    struct Size;

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

        std::string string() const {
            return "{" + std::to_string(x) + "," + std::to_string(y) + "}";
        }
    };

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

        std::string string() const {
            return "{" + std::to_string(width) + "," + std::to_string(height) + "}";
        }
    };

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

        std::string string() const {
            return "{{" + std::to_string(origin.x) + "," + std::to_string(origin.y)
                + "},{" + std::to_string(size.width) + "," + std::to_string(size.height) + "}}";
        }
    };

    struct Frame {
        std::string name;
        std::vector<uint8_t> data;
        Point offset;
        Size size;
        Rect rect;
        bool rotated;
    };

    class Packer {
    protected:
        std::vector<Frame> m_frames;
        Size m_size;
        uint32_t m_capacity;
    public:
        Packer(uint32_t capacity = 10000) : m_size(), m_capacity(capacity) {}
        Packer(Packer&& other) : m_frames(std::move(other.m_frames)), m_size(other.m_size), m_capacity(other.m_capacity) {}
        ~Packer() = default;

        void frame(std::string_view name, const uint8_t* data, uint32_t width, uint32_t height);
        geode::Result<Frame&> frame(std::string_view name);
        geode::Result<const Frame&> frame(std::string_view name) const;
        std::vector<Frame>& frames() { return m_frames; }
        const std::vector<Frame>& frames() const { return m_frames; }
        geode::Result<> pack();
        std::string plist(const std::string& name, std::string_view indent = "\t");
        geode::Result<> plist(const std::filesystem::path& path, const std::string& name, std::string_view indent = "\t");
        geode::Result<std::vector<uint8_t>> png();
        geode::Result<> png(const std::filesystem::path& path);
        Size& size() { return m_size; }
        const Size& size() const { return m_size; }
    };
}

#endif
