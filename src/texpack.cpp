#include <fmt/format.h>
#include <pugixml.hpp>
#include <rectpack2D/finders_interface.h>
#include <spng.h>
#include <texpack.hpp>

using namespace texpack;
using namespace geode;
using namespace rectpack2D;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__) || defined(WIN64) || defined(_WIN64) || defined(__WIN64) && !defined(__CYGWIN__)
#define GEODE_IS_WINDOWS
#endif

#ifdef GEODE_IS_WINDOWS
#define NOMINMAX
#include <Windows.h>

std::string formatError() {
    DWORD error = GetLastError();

    LPSTR buffer = nullptr;
    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        (LPSTR)&buffer,
        0,
        nullptr
    );

    if (size == 0 || !buffer) {
        return fmt::format("Win error {}", error);
    }

    std::string message(buffer, size);
    LocalFree(buffer);

    while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
        message.pop_back();
    }

    return message;
}

Result<> readFileInto(const std::filesystem::path& path, std::vector<uint8_t>& out) {
    HANDLE file = CreateFileW(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (file == INVALID_HANDLE_VALUE) {
        return Err(fmt::format("Unable to open file: {}", formatError()));
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(file, &fileSize)) {
        CloseHandle(file);
        return Err(fmt::format("Unable to get file size: {}", formatError()));
    }

    out.resize(fileSize.QuadPart);
    DWORD read = 0;
    if (!ReadFile(file, out.data(), static_cast<DWORD>(out.size()), &read, nullptr)) {
        CloseHandle(file);
        return Err(fmt::format("Unable to read file: {}", formatError()));
    }

    CloseHandle(file);

    if (read < out.size()) {
        return Err(fmt::format("Unable to read entire file: only read {} of {}", read, out.size()));
    }

    return Ok();
}

Result<> writeFileFrom(const std::filesystem::path& path, void* data, size_t size) {
    HANDLE file = CreateFileW(
        path.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (file == INVALID_HANDLE_VALUE) {
        return Err(fmt::format("Unable to open file: {}", formatError()));
    }

    DWORD written = 0;
    if (!WriteFile(file, data, static_cast<DWORD>(size), &written, nullptr)) {
        CloseHandle(file);
        return Err(fmt::format("Unable to write file: {}", formatError()));
    }

    if (written < size) {
        CloseHandle(file);
        return Err(fmt::format("Unable to write entire file: only wrote {} of {}", written, size));
    }

    CloseHandle(file);
    
    return Ok();
}
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

std::string formatError() {
    return strerror(errno);
}

Result<> readFileInto(const std::filesystem::path& path, std::vector<uint8_t>& out) {
    int file = open(path.c_str(), O_RDONLY);

    if (file == -1) {
        return Err(fmt::format("Unable to open file: {}", formatError()));
    }

    struct stat fst;
    if (fstat(file, &fst) == -1) {
        close(file);
        return Err(fmt::format("Unable to get file size: {}", formatError()));
    }

    out.resize(fst.st_size);
    ssize_t bread = read(file, out.data(), out.size());
    close(file);

    if (bread < 0) {
        return Err(fmt::format("Unable to read file: {}", formatError()));
    }

    if (bread < out.size()) {
        return Err(fmt::format("Unable to read entire file: only read {} of {}", bread, out.size()));
    }

    return Ok();
}

Result<> writeFileFrom(const std::filesystem::path& path, void* data, size_t size) {
    int file = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    if (file < 0) {
        return Err(fmt::format("Unable to open file: {}", formatError()));
    }

    size_t written = 0;
    while (written < size) {
        ssize_t bwrite = write(file, (uint8_t*)data + written, size - written);
        if (bwrite < 0) {
            if (errno == EINTR) continue;
            close(file);
            return Err(fmt::format("Unable to write file: {}", formatError()));
        }
        written += bwrite;
    }

    close(file);
    
    return Ok();
}
#endif

std::string Point::string() const {
    return fmt::format("{{{},{}}}", x, y);
}

std::string Size::string() const {
    return fmt::format("{{{},{}}}", width, height);
}

std::string Rect::string() const {
    return fmt::format("{{{{{},{}}},{{{},{}}}}}", origin.x, origin.y, size.width, size.height);
}

Image::Image() = default;
Image::Image(std::span<const uint8_t> data, uint32_t width, uint32_t height) : data(data.begin(), data.end()), width(width), height(height) {}
Image::Image(std::vector<uint8_t>&& data, uint32_t width, uint32_t height) : data(std::move(data)), width(width), height(height) {}

Image::Image(const Image&) = default;
Image::Image(Image&&) = default;
Image& Image::operator=(const Image&) = default;
Image& Image::operator=(Image&&) = default;

Packer::Packer(int capacity) : m_frames(), m_image(), m_capacity(capacity) {}

Packer::Packer(const Packer&) = default;
Packer::Packer(Packer&&) = default;
Packer& Packer::operator=(const Packer&) = default;
Packer& Packer::operator=(Packer&&) = default;

void Packer::frame(std::string name, std::span<const uint8_t> data, uint32_t width, uint32_t height) {
    auto it = std::ranges::find_if(m_frames, [&name](const Frame& frame) { return frame.name == name; });
    if (it != m_frames.end()) m_frames.erase(it, m_frames.end());

    Frame frame;

    frame.name = std::move(name);
    frame.size.width = width;
    frame.size.height = height;

    if (width == 0 || height == 0) {
        frame.offset.x = 0;
        frame.offset.y = 0;
        frame.rect.size.width = 0;
        frame.rect.size.height = 0;
        frame.rotated = false;
        m_frames.push_back(std::move(frame));
        return;
    }

    auto left = -1;
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            if (data[(y * width + x) * 4 + 3] != 0) {
                left = x;
                break;
            }
        }
        if (left != -1) break;
    }
    left = std::max(left, 0);

    auto right = 0;
    for (int x = width - 1; x >= 0; x--) {
        for (int y = 0; y < height; y++) {
            if (data[(y * width + x) * 4 + 3] != 0) {
                right = x + 1;
                break;
            }
        }
        if (right != 0) break;
    }
    right = std::min<int>(right, width);

    if (left >= right) right = left + 1;

    auto top = -1;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (data[(y * width + x) * 4 + 3] != 0) {
                top = y;
                break;
            }
        }
        if (top != -1) break;
    }
    top = std::max(top, 0);

    auto bottom = 0;
    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            if (data[(y * width + x) * 4 + 3] != 0) {
                bottom = y + 1;
                break;
            }
        }
        if (bottom != 0) break;
    }
    bottom = std::min<int>(bottom, height);

    if (top >= bottom) bottom = top + 1;

    auto w = right - left;
    auto h = bottom - top;
    frame.offset.x = left - (width - w) / 2 - (width % 2 != w % 2);
    frame.offset.y = (height - h) / 2 + (height % 2 != h % 2) - top;
    frame.data.resize(w * h * 4);

    for (int x = 0; x < w * 4; x++) {
        for (int y = 0; y < h; y++) {
            frame.data[y * w * 4 + x] = data[(y + top) * width * 4 + left * 4 + x];
        }
    }

    frame.rect.size.width = w;
    frame.rect.size.height = h;
    frame.rotated = false;

    m_frames.push_back(std::move(frame));
}

Result<> Packer::frame(std::string name, std::istream& stream, bool premultiplyAlpha) {
    GEODE_UNWRAP_INTO(auto image, fromPNG(stream, premultiplyAlpha));
    frame(std::move(name), image);
    return Ok();
}

Result<> Packer::frame(std::string name, std::span<const uint8_t> data, bool premultiplyAlpha) {
    GEODE_UNWRAP_INTO(auto image, fromPNG(data, premultiplyAlpha));
    frame(std::move(name), image);
    return Ok();
}

Result<> Packer::frame(std::string name, const std::filesystem::path& path, bool premultiplyAlpha) {
    GEODE_UNWRAP_INTO(auto image, fromPNG(path, premultiplyAlpha));
    frame(std::move(name), image);
    return Ok();
}

Result<Frame&> Packer::frame(std::string_view name) {
    auto it = std::ranges::find_if(m_frames, [name](const Frame& frame) { return frame.name == name; });
    if (it == m_frames.end()) return Err("Frame not found");

    return Ok(*it);
}

Result<const Frame&> Packer::frame(std::string_view name) const {
    auto it = std::ranges::find_if(m_frames, [name](const Frame& frame) { return frame.name == name; });
    if (it == m_frames.end()) return Err("Frame not found");

    return Ok(*it);
}

Result<> Packer::pack(int padding) {
    if (m_frames.empty()) return Ok();

    std::ranges::sort(m_frames, [](const Frame& a, const Frame& b) {
        return a.name < b.name;
    });

    std::vector<rect_xywhf> rects;
    rects.reserve(m_frames.size());
    auto doublePadding = padding * 2;
    for (auto& frame : m_frames) {
        rects.emplace_back(0, 0, frame.rect.size.width + doublePadding, frame.rect.size.height + doublePadding, false);
    }

    auto index = 0;
    auto success = true;
    auto result = find_best_packing<empty_spaces<true>>(rects, make_finder_input(
        m_capacity, 1,
        [&index](auto&) {
            index++;
            return callback_result::CONTINUE_PACKING;
        },
        [&success](auto&) {
            success = false;
            return callback_result::ABORT_PACKING;
        },
        flipping_option::ENABLED
    ));

    if (!success) return Err(fmt::format("Packing failed on {}", m_frames[index].name));
    else if (result.w <= 0 || result.h <= 0) return Err("Packing failed");

    for (int i = 0; i < rects.size(); i++) {
        auto& frame = m_frames[i];
        auto& rect = rects[i];
        frame.rect.origin.x = rect.x + padding;
        frame.rect.origin.y = rect.y + padding;
        frame.rotated = rect.flipped;

        if (frame.rotated) {
            std::vector<uint8_t> original(frame.data.size());
            frame.data.swap(original);
            auto [w, h] = frame.rect.size;
            for (int x = 0; x < w * 4; x++) {
                for (int y = h - 1; y >= 0; y--) {
                    frame.data[(x / 4 * h + h - 1 - y) * 4 + x % 4] = original[y * w * 4 + x];
                }
            }
        }
    }

    auto& data = m_image.data;
    data.clear();
    data.resize(result.w * result.h * 4);
    for (auto& frame : m_frames) {
        auto [l, t] = frame.rect.origin;
        auto w = frame.rotated ? frame.rect.size.height : frame.rect.size.width;
        auto h = frame.rotated ? frame.rect.size.width : frame.rect.size.height;
        auto& frameData = frame.data;
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w * 4; x++) {
                data[(t + y) * result.w * 4 + l * 4 + x] = frameData[y * w * 4 + x];
            }
        }
    }

    m_image.width = result.w;
    m_image.height = result.h;

    return Ok();
}

void Packer::plist(std::ostream& stream, std::string_view name, std::string_view indent) {
    auto plistData = plist(name, indent);
    stream.write(plistData.data(), plistData.size());
}

struct xml_string_writer : pugi::xml_writer {
    fmt::memory_buffer result;

    void write(const void* data, size_t size) override {
        fmt::format_to(std::back_inserter(result), "{}", std::string_view(static_cast<const char*>(data), size));
    }
};

std::string Packer::plist(std::string_view name, std::string_view indent) {
    pugi::xml_document doc;
    auto root = doc.append_child("plist");
    root.append_attribute("version") = "1.0";
    auto dict = root.append_child("dict");

    dict.append_child("key").text() = "frames";
    auto frames = dict.append_child("dict");
    for (auto& frame : m_frames) {
        frames.append_child("key").text() = frame.name;
        auto frameNode = frames.append_child("dict");
        frameNode.append_child("key").text() = "spriteOffset";
        frameNode.append_child("string").text() = frame.offset.string();
        frameNode.append_child("key").text() = "spriteSize";
        frameNode.append_child("string").text() = frame.rect.size.string();
        frameNode.append_child("key").text() = "spriteSourceSize";
        frameNode.append_child("string").text() = frame.size.string();
        frameNode.append_child("key").text() = "textureRect";
        frameNode.append_child("string").text() = frame.rect.string();
        frameNode.append_child("key").text() = "textureRotated";
        frameNode.append_child(frame.rotated ? "true" : "false");
    }

    dict.append_child("key").text() = "metadata";
    auto metadata = dict.append_child("dict");
    metadata.append_child("key").text() = "format";
    metadata.append_child("integer").text() = 3;
    metadata.append_child("key").text() = "realTextureFileName";
    metadata.append_child("string").text() = name;
    metadata.append_child("key").text() = "size";
    metadata.append_child("string").text() = fmt::format("{{{}, {}}}", m_image.width, m_image.height);
    metadata.append_child("key").text() = "textureFileName";
    metadata.append_child("string").text() = name;

    xml_string_writer writer;
    doc.save(writer, indent.data());
    return fmt::to_string(writer.result);
}

Result<> Packer::plist(const std::filesystem::path& path, std::string_view name, std::string_view indent) {
    auto plistData = plist(name, indent);
    return writeFileFrom(path, plistData.data(), plistData.size());
}

Result<> Packer::png(std::ostream& stream) {
    return toPNG(stream, m_image);
}

Result<std::vector<uint8_t>> Packer::png() {
    return toPNG(m_image);
}

Result<> Packer::png(const std::filesystem::path& path) {
    return toPNG(path, m_image);
}

Result<Image> texpack::fromPNG(std::istream& stream, bool premultiplyAlpha) {
    return fromPNG(std::vector<uint8_t>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()), premultiplyAlpha);
}

Result<Image> texpack::fromPNG(std::span<const uint8_t> data, bool premultiplyAlpha) {
    if (
        data.size() < 8 ||
        data[0] != 137 || data[1] != 80 || data[2] != 78 || data[3] != 71 ||
        data[4] != 13 || data[5] != 10 || data[6] != 26 || data[7] != 10
    ) return Err("Invalid PNG data");

    auto ctx = spng_ctx_new(0);
    if (!ctx) return Err("Failed to create PNG context");

    if (auto result = spng_set_png_buffer(ctx, data.data(), data.size())) {
        spng_ctx_free(ctx);
        return Err(fmt::format("Failed to set PNG buffer: {}", spng_strerror(result)));
    }

    spng_ihdr ihdr;
    if (auto result = spng_get_ihdr(ctx, &ihdr)) {
        spng_ctx_free(ctx);
        return Err(fmt::format("Failed to get image header: {}", spng_strerror(result)));
    }

    size_t imageSize = 0;
    if (auto result = spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &imageSize)) {
        spng_ctx_free(ctx);
        return Err(fmt::format("Failed to get image size: {}", spng_strerror(result)));
    }

    std::vector<uint8_t> image(imageSize);
    if (auto result = spng_decode_image(ctx, image.data(), imageSize, SPNG_FMT_RGBA8, SPNG_DECODE_TRNS)) {
        spng_ctx_free(ctx);
        return Err(fmt::format("Failed to decode image: {}", spng_strerror(result)));
    }

    spng_ctx_free(ctx);

    if (premultiplyAlpha) {
        for (size_t i = 0; i < image.size(); i += 4) {
            auto alpha = image[i + 3] / 255.0;
            image[i] *= alpha;
            image[i + 1] *= alpha;
            image[i + 2] *= alpha;
        }
    }

    return Ok(Image(std::move(image), ihdr.width, ihdr.height));
}

Result<Image> texpack::fromPNG(const std::filesystem::path& path, bool premultiplyAlpha) {
    std::vector<uint8_t> data;
    GEODE_UNWRAP(readFileInto(path, data));
    return fromPNG(data, premultiplyAlpha);
}

Result<> texpack::toPNG(std::ostream& stream, std::span<const uint8_t> data, uint32_t width, uint32_t height) {
    GEODE_UNWRAP_INTO(auto pngData, toPNG(data, width, height));
    stream.write(reinterpret_cast<const char*>(pngData.data()), pngData.size());
    return Ok();
}

Result<std::vector<uint8_t>> texpack::toPNG(std::span<const uint8_t> data, uint32_t width, uint32_t height) {
    auto ctx = spng_ctx_new(SPNG_CTX_ENCODER);
    if (!ctx) return Err("Failed to create PNG context");

    std::vector<uint8_t> pngData;
    if (auto result = spng_set_png_stream(ctx, [](spng_ctx* ctx, void* user, void* data, size_t size) {
        auto pngData = reinterpret_cast<std::vector<uint8_t>*>(user);
        auto pngSize = pngData->size();
        pngData->resize(pngSize + size);
        #ifdef GEODE_IS_WINDOWS
        memcpy_s(pngData->data() + pngSize, pngSize + size, data, size);
        #else
        memcpy(pngData->data() + pngSize, data, size);
        #endif
        return 0;
    }, &pngData)) {
        spng_ctx_free(ctx);
        return Err(fmt::format("Failed to set PNG stream: {}", spng_strerror(result)));
    }

    spng_ihdr ihdr = { width, height, 8, SPNG_COLOR_TYPE_TRUECOLOR_ALPHA, 0, SPNG_FILTER_NONE, SPNG_INTERLACE_NONE };
    if (auto result = spng_set_ihdr(ctx, &ihdr)) {
        spng_ctx_free(ctx);
        return Err(fmt::format("Failed to set image header: {}", spng_strerror(result)));
    }

    if (auto result = spng_encode_image(ctx, data.data(), width * height * 4, SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE)) {
        spng_ctx_free(ctx);
        return Err(fmt::format("Failed to encode image: {}", spng_strerror(result)));
    }

    spng_ctx_free(ctx);
    return Ok(std::move(pngData));
}

Result<> texpack::toPNG(const std::filesystem::path& path, std::span<const uint8_t> data, uint32_t width, uint32_t height) {
    GEODE_UNWRAP_INTO(auto pngData, toPNG(data, width, height));
    return writeFileFrom(path, pngData.data(), pngData.size());
}
