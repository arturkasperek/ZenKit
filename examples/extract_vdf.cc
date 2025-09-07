// Copyright © 2024 GothicKit Contributors.
// SPDX-License-Identifier: MIT

#include <zenkit/Vfs.hh>
#include <zenkit/Stream.hh>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

static void extract_tree(zenkit::VfsNode const& node,
                         fs::path const& outputRoot,
                         fs::path const& relativePath = fs::path()) {
    if (node.type() == zenkit::VfsNodeType::DIRECTORY) {
        auto dirPath = outputRoot / relativePath / node.name();
        std::error_code ec;
        fs::create_directories(dirPath, ec);
        for (auto const& child : node.children()) {
            extract_tree(child, outputRoot, relativePath / node.name());
        }
        return;
    }

    auto filePath = outputRoot / relativePath / node.name();
    std::error_code ec;
    fs::create_directories(filePath.parent_path(), ec);

    auto reader = node.open_read();
    std::ofstream out(filePath, std::ios::binary | std::ios::trunc);
    if (!out) {
        std::cerr << "Failed to open output file: " << filePath << "\n";
        return;
    }

    std::vector<std::byte> buffer(1 << 16);
    for (;;) {
        size_t readBytes = reader->read(buffer.data(), buffer.size());
        if (readBytes == 0) break;
        out.write(reinterpret_cast<char const*>(buffer.data()), static_cast<std::streamsize>(readBytes));
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: extract_vdf <path/to/archive.vdf>\n";
        return 1;
    }

    fs::path vdfPath(argv[1]);
    if (!fs::exists(vdfPath)) {
        std::cerr << "VDF not found: " << vdfPath << "\n";
        return 1;
    }

    fs::path outRoot = vdfPath.parent_path() / vdfPath.stem();
    std::error_code ec;
    fs::create_directories(outRoot, ec);

    try {
        zenkit::Vfs vfs;
        vfs.mount_disk(vdfPath);
        for (auto const& child : vfs.root().children()) {
            extract_tree(child, outRoot);
        }
        std::cout << "Extracted to: " << outRoot << "\n";
        return 0;
    } catch (std::exception const& ex) {
        std::cerr << "Extraction failed: " << ex.what() << "\n";
        return 2;
    }
}


