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
}