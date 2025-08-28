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

    std::unique_ptr<ReadArchiveWrapper> create_read_archive(uintptr_t data_ptr, size_t length) {
        auto reader = create_reader_from_buffer(data_ptr, length);
        auto archive = zenkit::ReadArchive::from(reader.get());
        return std::make_unique<ReadArchiveWrapper>(std::move(archive));
    }

} // namespace zenkit::wasm
