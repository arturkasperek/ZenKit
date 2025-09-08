#include <zenkit/Vfs.hh>
#include <zenkit/Logger.hh>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class VdfExtractor {
public:
    VdfExtractor(const std::string& game_path, const std::string& output_path)
        : game_path_(game_path), output_path_(output_path) {
        
        // Setup ZenKit logging
        zenkit::Logger::set_default(zenkit::LogLevel::INFO);
    }

    bool run() {
        std::cout << "VDF Extractor - Gothic Game Asset Bootstrapper\n";
        std::cout << "==============================================\n";
        std::cout << "Game location: " << game_path_ << "\n";
        std::cout << "Output location: " << output_path_ << "\n\n";

        // Check if game path exists
        if (!fs::exists(game_path_)) {
            std::cerr << "Error: Game path does not exist: " << game_path_ << std::endl;
            return false;
        }

        // Check for Data folder
        fs::path data_path = fs::path(game_path_) / "Data";
        if (!fs::exists(data_path)) {
            std::cerr << "Error: Data folder not found in game path: " << data_path << std::endl;
            return false;
        }

        // Create output directory
        try {
            fs::create_directories(output_path_);
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error: Failed to create output directory: " << e.what() << std::endl;
            return false;
        }

        // Find all VDF files
        std::vector<fs::path> vdf_files = find_vdf_files(data_path);
        
        if (vdf_files.empty()) {
            std::cout << "No VDF files found in Data directory." << std::endl;
            return true;
        }

        std::cout << "Found " << vdf_files.size() << " VDF files:\n";
        for (const auto& vdf : vdf_files) {
            std::cout << "  - " << vdf.filename().string() << std::endl;
        }
        std::cout << std::endl;

        // Extract each VDF
        size_t success_count = 0;
        for (const auto& vdf_path : vdf_files) {
            if (extract_vdf(vdf_path)) {
                success_count++;
            }
        }

        std::cout << "\nExtraction complete!\n";
        std::cout << "Successfully extracted " << success_count << " out of " << vdf_files.size() << " VDF files.\n";
        
        return success_count > 0;
    }

private:
    std::string game_path_;
    std::string output_path_;

    std::vector<fs::path> find_vdf_files(const fs::path& data_path) {
        std::vector<fs::path> vdf_files;
        
        try {
            // Recursively search for .vdf files
            for (const auto& entry : fs::recursive_directory_iterator(data_path)) {
                if (entry.is_regular_file()) {
                    const auto& path = entry.path();
                    std::string extension = path.extension().string();
                    
                    // Convert to lowercase for case-insensitive comparison
                    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                    
                    if (extension == ".vdf") {
                        vdf_files.push_back(path);
                    }
                }
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Warning: Error reading directory " << data_path << ": " << e.what() << std::endl;
        }

        // Sort files for consistent processing order
        std::sort(vdf_files.begin(), vdf_files.end());
        
        return vdf_files;
    }

    fs::path flatten_path(const fs::path& relative_path) {
        // Convert path to string and normalize
        std::string path_str = relative_path.string();
        
        // Remove common Gothic prefixes to flatten structure
        std::vector<std::string> prefixes_to_remove = {
            "_WORK/DATA/",
            "_WORK\\DATA\\",  // Windows path separator
            "_work/data/",    // lowercase variants
            "_work\\data\\"
        };
        
        for (const auto& prefix : prefixes_to_remove) {
            if (path_str.find(prefix) == 0) {
                path_str = path_str.substr(prefix.length());
                break;
            }
        }
        
        return fs::path(path_str);
    }

    void extract_tree(const zenkit::VfsNode& node, const fs::path& output_root, const fs::path& relative_path = fs::path()) {
        if (node.type() == zenkit::VfsNodeType::DIRECTORY) {
            // Skip directories that are just organizational (_WORK, DATA)
            std::string dir_name = node.name();
            std::transform(dir_name.begin(), dir_name.end(), dir_name.begin(), ::tolower);
            
            if (dir_name == "_work" || dir_name == "data") {
                // Skip this directory level, just process children with current path
                for (auto const& child : node.children()) {
                    extract_tree(child, output_root, relative_path);
                }
                return;
            }
            
            auto new_relative_path = relative_path / node.name();
            auto flattened_path = flatten_path(new_relative_path);
            auto dir_path = output_root / flattened_path;
            
            std::error_code ec;
            fs::create_directories(dir_path, ec);
            
            for (auto const& child : node.children()) {
                extract_tree(child, output_root, new_relative_path);
            }
            return;
        }

        // For files, use flattened path
        auto flattened_relative = flatten_path(relative_path);
        auto file_path = output_root / flattened_relative / node.name();
        
        std::error_code ec;
        fs::create_directories(file_path.parent_path(), ec);

        auto reader = node.open_read();
        std::ofstream out(file_path, std::ios::binary | std::ios::trunc);
        if (!out) {
            std::cerr << "\n  Warning: Failed to open output file: " << file_path << std::endl;
            return;
        }

        std::vector<std::byte> buffer(1 << 16);
        for (;;) {
            size_t read_bytes = reader->read(buffer.data(), buffer.size());
            if (read_bytes == 0) break;
            out.write(reinterpret_cast<char const*>(buffer.data()), static_cast<std::streamsize>(read_bytes));
        }
    }

    bool extract_vdf(const fs::path& vdf_path) {
        std::cout << "Extracting: " << vdf_path.filename().string() << "...";
        
        try {
            // Extract directly to the root output directory (no VDF subdirectory)
            fs::path vdf_output_dir = fs::path(output_path_);
            
            std::error_code ec;
            fs::create_directories(vdf_output_dir, ec);
            if (ec) {
                std::cout << " FAILED (could not create output directory: " << ec.message() << ")\n";
                return false;
            }

            // Mount VDF and extract all files
            zenkit::Vfs vfs;
            vfs.mount_disk(vdf_path);
            
            size_t file_count = 0;
            for (auto const& child : vfs.root().children()) {
                extract_tree(child, vdf_output_dir);
                file_count++;
            }

            std::cout << " OK (" << file_count << " entries)\n";
            return file_count > 0;

        } catch (const std::exception& e) {
            std::cout << " FAILED (" << e.what() << ")\n";
            return false;
        }
    }
};

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <game_location> <assets_destination>\n\n";
    std::cout << "Arguments:\n";
    std::cout << "  game_location      Path to Gothic game installation (e.g., /Users/artur/dev/gothic/Gothic2)\n";
    std::cout << "  assets_destination Path where extracted assets will be stored (e.g., /Users/artur/dev/gothic/ZenKit/public/game-assets)\n\n";
    std::cout << "Description:\n";
    std::cout << "  This tool extracts VDF (Virtual File System) archives from a Gothic game installation\n";
    std::cout << "  to bootstrap assets for editor development. It recursively searches the Data folder\n";
    std::cout << "  for all .vdf files and extracts their contents to the specified destination.\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " /usr/games/Gothic2 ./public/game-assets\n";
    std::cout << "  " << program_name << " \"C:\\Games\\Gothic II\" \"D:\\Projects\\assets\"\n";
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    std::string game_location = argv[1];
    std::string assets_destination = argv[2];

    VdfExtractor extractor(game_location, assets_destination);
    
    if (extractor.run()) {
        std::cout << "\nVDF extraction completed successfully!\n";
        return 0;
    } else {
        std::cout << "\nVDF extraction failed!\n";
        return 1;
    }
}
