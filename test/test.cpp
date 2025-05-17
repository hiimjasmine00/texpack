#include <iostream>
#include <texpack.hpp>

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            std::cout << "Usage: " << argv[0] << " <folder_path>" << std::endl;
            return 0;
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

            auto frameName = path.filename().string();
            auto frameResult = packer.frame(frameName, path);
            if (frameResult.isErr()) {
                std::cerr << "Failed to add frame " << frameName << ": " << frameResult.unwrapErr() << std::endl;
                return 1;
            }
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
