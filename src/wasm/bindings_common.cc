// Copyright © 2024 GothicKit Contributors.
// SPDX-License-Identifier: MIT
#include "bindings_common.hh"
#include "zenkit/Stream.hh"

namespace zenkit::wasm {

    std::unique_ptr<zenkit::Read> create_reader_from_buffer(uintptr_t data_ptr, size_t length) {
        auto bytes = reinterpret_cast<const std::byte*>(data_ptr);
        return zenkit::Read::from(bytes, length);
    }

    std::unique_ptr<zenkit::Read> create_reader_from_string(const std::string& buffer) {
        // Create a vector to avoid string encoding issues
        std::vector<std::byte> data(buffer.size());
        std::memcpy(data.data(), buffer.data(), buffer.size());
        return zenkit::Read::from(std::move(data));
    }

    // New: Create reader from JavaScript Uint8Array (handles memory automatically)
    std::unique_ptr<zenkit::Read> create_reader_from_js_array(const emscripten::val& uint8_array) {
        // Get the length and data pointer from JavaScript Uint8Array
        auto length = uint8_array["length"].as<size_t>();
        
        // Copy data from JavaScript to C++ to avoid memory management issues
        std::vector<std::byte> data(length);
        
        // Copy data from JavaScript array to our C++ vector
        emscripten::val memory = emscripten::val::module_property("HEAPU8");
        emscripten::val memoryBuffer = uint8_array["buffer"];
        emscripten::val byteOffset = uint8_array["byteOffset"];
        
        // Use JavaScript's subarray to get a view of the data
        emscripten::val dataView = uint8_array.call<emscripten::val>("subarray", 0, length);
        
        // Copy byte by byte (safer for WASM)
        for (size_t i = 0; i < length; ++i) {
            auto byte_val = uint8_array[i].as<uint8_t>();
            data[i] = static_cast<std::byte>(byte_val);
        }
        
        return zenkit::Read::from(std::move(data));
    }

    std::unique_ptr<ReadArchiveWrapper> create_read_archive(uintptr_t data_ptr, size_t length) {
        auto reader = create_reader_from_buffer(data_ptr, length);
        auto archive = zenkit::ReadArchive::from(reader.get());
        return std::make_unique<ReadArchiveWrapper>(std::move(archive));
    }

    // New: Create archive from JavaScript Uint8Array
    std::unique_ptr<ReadArchiveWrapper> create_read_archive_from_js_array(const emscripten::val& uint8_array) {
        auto reader = create_reader_from_js_array(uint8_array);
        auto archive = zenkit::ReadArchive::from(reader.get());
        return std::make_unique<ReadArchiveWrapper>(std::move(archive));
    }

} // namespace zenkit::wasm
