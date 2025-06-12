#include <cmath>
#include <fstream>
#ifdef TEXPACK_PUGIXML
#define pugi _pugi
#include <pugixml.hpp>
#undef pugi
namespace pugi = _pugi;
#else
#include <pugixml.hpp>
#endif
#include <rectpack2D/finders_interface.h>
#include <spng.h>
#include <texpack.hpp>

using namespace texpack;
using namespace geode;
using namespace rectpack2D;

void Packer::frame(std::string_view name, std::span<const uint8_t> data, uint32_t width, uint32_t height) {
    auto it = std::ranges::find_if(m_frames, [&name](const Frame& frame) { return frame.name == name; });
    if (it != m_frames.end()) m_frames.erase(it, m_frames.end());

    Frame frame;

    frame.name = name;
    frame.size.width = width;
    frame.size.height = height;

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
    right = std::min(right, (int)width);

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
    bottom = std::min(bottom, (int)height);

    if (top >= bottom) bottom = top + 1;

    auto w = right - left;
    auto h = bottom - top;
    frame.offset.x = left - (int)round((width - w) / 2.0);
    frame.offset.y = (int)round((height - h) / 2.0) - top;
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

void Packer::frame(std::string_view name, const Image& image) {
    frame(name, image.data, image.width, image.height);
}

Result<> Packer::frame(std::string_view name, std::span<const uint8_t> data) {
    GEODE_UNWRAP_INTO(auto image, fromPNG(data));
    frame(name, image.data, image.width, image.height);
    return Ok();
}

Result<> Packer::frame(std::string_view name, const std::filesystem::path& path) {
    GEODE_UNWRAP_INTO(auto image, fromPNG(path));
    frame(name, image.data, image.width, image.height);
    return Ok();
}

Result<Frame&> Packer::frame(std::string_view name) {
    auto it = std::ranges::find_if(m_frames, [&name](const Frame& frame) { return frame.name == name; });
    if (it == m_frames.end()) return Err("Frame not found");

    return Ok(*it);
}

Result<const Frame&> Packer::frame(std::string_view name) const {
    auto it = std::ranges::find_if(m_frames, [&name](const Frame& frame) { return frame.name == name; });
    if (it == m_frames.end()) return Err("Frame not found");

    return Ok(*it);
}

Result<> Packer::pack() {
    if (m_frames.empty()) return Ok();

    std::ranges::sort(m_frames, [](const Frame& a, const Frame& b) {
        return a.name < b.name;
    });

    std::vector<rect_xywhf> rects;
    rects.reserve(m_frames.size());
    for (auto& frame : m_frames) {
        rects.emplace_back(0, 0, frame.rect.size.width, frame.rect.size.height, false);
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

    if (!success) return Err("Packing failed on " + m_frames[index].name);
    else if (result.w <= 0 || result.h <= 0) return Err("Packing failed");

    for (int i = 0; i < rects.size(); i++) {
        auto& frame = m_frames[i];
        auto& rect = rects[i];
        frame.rect.origin.x = rect.x;
        frame.rect.origin.y = rect.y;
        frame.rotated = rect.flipped;

        if (frame.rotated) {
            std::vector<uint8_t> original(frame.data);
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

std::string Packer::plist(const std::string& name, std::string_view indent) {
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
    metadata.append_child("string").text() = "{" + std::to_string(m_image.width) + "," + std::to_string(m_image.height) + "}";
    metadata.append_child("key").text() = "textureFileName";
    metadata.append_child("string").text() = name;

    std::ostringstream writer;
    doc.save(writer, indent.data());
    return writer.str();
}

Result<> Packer::plist(const std::filesystem::path& path, const std::string& name, std::string_view indent) {
    std::ofstream file(path);
    if (!file.is_open()) return Err("Failed to open file");
    file << plist(name, indent);
    file.close();
    return Ok();
}

Result<std::vector<uint8_t>> Packer::png() {
    GEODE_UNWRAP_INTO(auto pngData, toPNG(m_image));
    return Ok(pngData);
}

Result<> Packer::png(const std::filesystem::path& path) {
    GEODE_UNWRAP(toPNG(path, m_image));
    return Ok();
}

Result<Image> texpack::fromPNG(std::span<const uint8_t> data) {
    auto ctx = spng_ctx_new(0);
    if (!ctx) return Err("Failed to create PNG context");

    if (auto result = spng_set_png_buffer(ctx, data.data(), data.size())) {
        spng_ctx_free(ctx);
        return Err(std::string("Failed to set PNG buffer: ") + spng_strerror(result));
    }

    spng_ihdr ihdr;
    if (auto result = spng_get_ihdr(ctx, &ihdr)) {
        spng_ctx_free(ctx);
        return Err(std::string("Failed to get image header: ") + spng_strerror(result));
    }

    size_t imageSize = 0;
    if (auto result = spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &imageSize)) {
        spng_ctx_free(ctx);
        return Err(std::string("Failed to get image size: ") + spng_strerror(result));
    }

    std::vector<uint8_t> image(imageSize);
    if (auto result = spng_decode_image(ctx, image.data(), imageSize, SPNG_FMT_RGBA8, SPNG_DECODE_TRNS)) {
        spng_ctx_free(ctx);
        return Err(std::string("Failed to decode image: ") + spng_strerror(result));
    }

    spng_ctx_free(ctx);

    for (size_t i = 0; i < image.size(); i += 4) {
        if (image[i + 3] == 0) {
            image[i] = 0;
            image[i + 1] = 0;
            image[i + 2] = 0;
        }
    }

    return Ok<Image>({ image, ihdr.width, ihdr.height });
}

Result<Image> texpack::fromPNG(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return Err("Failed to open file");
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    GEODE_UNWRAP_INTO(auto image, fromPNG(data));
    return Ok(image);
}

Result<std::vector<uint8_t>> texpack::toPNG(std::span<const uint8_t> data, uint32_t width, uint32_t height) {
    auto ctx = spng_ctx_new(SPNG_CTX_ENCODER);
    if (!ctx) return Err("Failed to create PNG context");

    if (auto result = spng_set_option(ctx, SPNG_ENCODE_TO_BUFFER, 1)) {
        spng_ctx_free(ctx);
        return Err(std::string("Failed to set encoding options: ") + spng_strerror(result));
    }

    spng_ihdr ihdr = { width, height, 8, SPNG_COLOR_TYPE_TRUECOLOR_ALPHA, 0, SPNG_FILTER_NONE, SPNG_INTERLACE_NONE };
    if (auto result = spng_set_ihdr(ctx, &ihdr)) {
        spng_ctx_free(ctx);
        return Err(std::string("Failed to set image header: ") + spng_strerror(result));
    }

    if (auto result = spng_encode_image(ctx, data.data(), width * height * 4, SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE)) {
        spng_ctx_free(ctx);
        return Err(std::string("Failed to encode image: ") + spng_strerror(result));
    }

    size_t pngSize = 0;
    auto result = 0;
    auto buffer = reinterpret_cast<uint8_t*>(spng_get_png_buffer(ctx, &pngSize, &result));
    if (result != 0) {
        spng_ctx_free(ctx);
        return Err(std::string("Failed to get PNG buffer: ") + spng_strerror(result));
    }
    else if (!buffer) {
        spng_ctx_free(ctx);
        return Err("Failed to get PNG buffer");
    }

    std::vector<uint8_t> pngData(buffer, buffer + pngSize);
    spng_ctx_free(ctx);
    free(buffer);
    return Ok(pngData);
}

Result<std::vector<uint8_t>> texpack::toPNG(const Image& image) {
    GEODE_UNWRAP_INTO(auto pngData, toPNG(image.data, image.width, image.height));
    return Ok(pngData);
}

Result<> texpack::toPNG(const std::filesystem::path& path, std::span<const uint8_t> data, uint32_t width, uint32_t height) {
    GEODE_UNWRAP_INTO(auto pngData, toPNG(data, width, height));
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return Err("Failed to open file");
    file.write(reinterpret_cast<const char*>(pngData.data()), pngData.size());
    file.close();
    return Ok();
}

Result<> texpack::toPNG(const std::filesystem::path& path, const Image& image) {
    GEODE_UNWRAP(toPNG(path, image.data, image.width, image.height));
    return Ok();
}
