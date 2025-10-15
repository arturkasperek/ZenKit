// Copyright © 2024 GothicKit Contributors.
// SPDX-License-Identifier: MIT
/// \file zenkit_wasm.cc
/// \brief Main WebAssembly entry point and coordinator for ZenKit bindings
///
/// This file serves as the main coordinator for all ZenKit WebAssembly bindings.
/// Individual class bindings are organized in separate files for maintainability.

#include "bindings_common.hh"

#include <emscripten/bind.h>
#include <iostream>

namespace zenkit::wasm {

    /// \brief Get ZenKit library version
    std::string getZenKitVersion() {
        return "1.3.0";
    }

    /// \brief Library information structure
    struct LibraryInfo {
        std::string version;
        std::string build_type;
        bool has_mmap;
        bool debug_build;
    };

    /// \brief Get comprehensive library information
    LibraryInfo getLibraryInfo() {
        LibraryInfo info;
        info.version = "1.3.0";
        #ifdef NDEBUG
            info.debug_build = false;
            info.build_type = "Release";
        #else
            info.debug_build = true;
            info.build_type = "Debug";
        #endif
        
        #ifdef _ZK_WITH_MMAP
            info.has_mmap = true;
        #else
            info.has_mmap = false;
        #endif
        
        return info;
    }

} // namespace zenkit::wasm

EMSCRIPTEN_BINDINGS(zenkit_main) {
    using namespace zenkit::wasm;
    using namespace emscripten;

    // Library information
    class_<LibraryInfo>("LibraryInfo")
        .property("version", &LibraryInfo::version)
        .property("buildType", &LibraryInfo::build_type)
        .property("hasMmap", &LibraryInfo::has_mmap)
        .property("debugBuild", &LibraryInfo::debug_build);

    // Core library functions
    function("getZenKitVersion", &getZenKitVersion);
    function("getLibraryInfo", &getLibraryInfo);

    // Texture bindings
    class_<TextureWrapper>("Texture")
        .constructor<>()
        .function("loadFromArray", &TextureWrapper::loadFromArray)
        .property("width", &TextureWrapper::width)
        .property("height", &TextureWrapper::height)
        .property("mipmaps", &TextureWrapper::mipmaps)
        .function("asRgba8", &TextureWrapper::asRgba8);
}

// Archive reading bindings
EMSCRIPTEN_BINDINGS(zenkit_archive) {
    using namespace zenkit::wasm;
    using namespace emscripten;

    // Color data structure
    value_object<ColorData>("ColorData")
        .field("r", &ColorData::r)
        .field("g", &ColorData::g)
        .field("b", &ColorData::b)
        .field("a", &ColorData::a);

    // Archive object data structure
    value_object<ArchiveObjectData>("ArchiveObjectData")
        .field("objectName", &ArchiveObjectData::object_name)
        .field("className", &ArchiveObjectData::class_name)
        .field("version", &ArchiveObjectData::version)
        .field("index", &ArchiveObjectData::index);

    // Bounding box data structure
    value_object<BoundingBoxData>("BoundingBoxData")
        .field("min", &BoundingBoxData::min)
        .field("max", &BoundingBoxData::max);

    // Matrix 3x3 data structure
    class_<Matrix3x3Data>("Matrix3x3Data")
        .function("get", &Matrix3x3Data::get)
        .function("getIndex", &Matrix3x3Data::getIndex)
        .function("toArray", &Matrix3x3Data::toArray);

    // Raw data result structure
    class_<RawDataResult>("RawDataResult")
        .property("data", &RawDataResult::data)
        .function("readUbyte", &RawDataResult::read_ubyte);

    // ReadArchive wrapper
    class_<ReadArchiveWrapper>("ReadArchive")
        .function("readObjectBegin", &ReadArchiveWrapper::read_object_begin)
        .function("readObjectEnd", &ReadArchiveWrapper::read_object_end)
        .function("readString", &ReadArchiveWrapper::read_string)
        .function("readInt", &ReadArchiveWrapper::read_int)
        .function("readFloat", &ReadArchiveWrapper::read_float)
        .function("readByte", &ReadArchiveWrapper::read_byte)
        .function("readWord", &ReadArchiveWrapper::read_word)
        .function("readEnum", &ReadArchiveWrapper::read_enum)
        .function("readBool", &ReadArchiveWrapper::read_bool)
        .function("readColor", &ReadArchiveWrapper::read_color)
        .function("readVec3", &ReadArchiveWrapper::read_vec3)
        .function("readVec2", &ReadArchiveWrapper::read_vec2)
        .function("readBbox", &ReadArchiveWrapper::read_bbox)
        .function("readMat3x3", &ReadArchiveWrapper::read_mat3x3)
        .function("readRaw", &ReadArchiveWrapper::read_raw)
        .function("skipObject", &ReadArchiveWrapper::skip_object);

    // Factory function
    function("createReadArchive", &create_read_archive, allow_raw_pointers());
    function("createReadArchiveFromArray", &create_read_archive_from_js_array, allow_raw_pointers());
}