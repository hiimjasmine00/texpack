#include <fstream>
#include <iostream>
#include <texpack.hpp>
#include <spng.h>

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: " << argv[0] << " <folder_path>" << std::endl;
            return 1;
        }

        std::string folderString;
        for (int i = 1; i < argc; i++) {
            if (i > 1) folderString += ' ';
            folderString += argv[i];
        }
        std::filesystem::path folderPath(folderString);

        texpack::Packer packer(10000);

        for (auto& entry : std::filesystem::directory_iterator(folderString)) {
            auto& path = entry.path();
            if (!entry.is_regular_file() || path.extension() != ".png") continue;

            std::ifstream file(path, std::ios::binary);
            if (!file.is_open()) {
                std::cerr << "Failed to open " << path << std::endl;
                return 1;
            }

            std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            auto ctx = spng_ctx_new(0);
            if (!ctx) {
                std::cerr << "Failed to create spng context for " << path << std::endl;
                return 1;
            }

            if (auto result = spng_set_png_buffer(ctx, data.data(), data.size())) {
                std::cerr << "Failed to set PNG buffer for " << path << ": " << spng_strerror(result) << std::endl;
                spng_ctx_free(ctx);
                return 1;
            }

            spng_ihdr ihdr;
            if (auto result = spng_get_ihdr(ctx, &ihdr)) {
                std::cerr << "Failed to get IHDR for " << path << ": " << spng_strerror(result) << std::endl;
                spng_ctx_free(ctx);
                return 1;
            }

            size_t size = 0;
            if (auto result = spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &size)) {
                std::cerr << "Failed to get image size for " << path << ": " << spng_strerror(result) << std::endl;
                spng_ctx_free(ctx);
                return 1;
            }

            std::vector<uint8_t> image(size);
            if (auto result = spng_decode_image(ctx, image.data(), size, SPNG_FMT_RGBA8, 0)) {
                std::cerr << "Failed to decode image for " << path << ": " << spng_strerror(result) << std::endl;
                spng_ctx_free(ctx);
                return 1;
            }

            spng_ctx_free(ctx);

            packer.frame(path.filename().string(), image.data(), ihdr.width, ihdr.height);
        }

        auto packResult = packer.pack();
        if (packResult.isErr()) {
            std::cerr << "Failed to pack frames: " << packResult.unwrapErr() << std::endl;
            return 1;
        }

        auto folderFile = folderPath.filename().string();
        auto outputPath = folderPath.parent_path().parent_path() / "output";

        if (!std::filesystem::exists(outputPath)) std::filesystem::create_directories(outputPath);

        auto pngResult = packer.png(outputPath / (folderFile + ".png"));
        if (pngResult.isErr()) {
            std::cerr << "Failed to create PNG: " << pngResult.unwrapErr() << std::endl;
            return 1;
        }

        auto plistResult = packer.plist(outputPath / (folderFile + ".plist"), folderFile + ".png", "    ");
        if (plistResult.isErr()) {
            std::cerr << "Failed to create PLIST: " << plistResult.unwrapErr() << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
