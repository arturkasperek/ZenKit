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

    // Basic types
    value_object<zenkit::Vec2>("Vec2")
        .field("x", &zenkit::Vec2::x)
        .field("y", &zenkit::Vec2::y);
    
    value_object<zenkit::Vec3>("Vec3")
        .field("x", &zenkit::Vec3::x)
        .field("y", &zenkit::Vec3::y)
        .field("z", &zenkit::Vec3::z);

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

    // Matrix 4x4 data structure
    class_<Matrix4x4Data>("Matrix4x4Data")
        .function("get", select_overload<float(size_t) const>(&Matrix4x4Data::get))
        .function("get", select_overload<float(size_t, size_t) const>(&Matrix4x4Data::get))
        .function("toArray", &Matrix4x4Data::toArray)
        .function("size", &Matrix4x4Data::size);

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

    // ModelHierarchy classes
    class_<zenkit::ModelHierarchy>("ModelHierarchy")
        .property("nodes", &zenkit::ModelHierarchy::nodes)
        .property("rootTranslation", +[](const zenkit::ModelHierarchy& h) {
            auto obj = emscripten::val::object();
            obj.set("x", h.root_translation.x);
            obj.set("y", h.root_translation.y);
            obj.set("z", h.root_translation.z);
            return obj;
        });

    class_<zenkit::ModelHierarchyNode>("ModelHierarchyNode")
        .property("parentIndex", &zenkit::ModelHierarchyNode::parent_index)
        .property("name", &zenkit::ModelHierarchyNode::name)
        .function("getTransform", +[](const zenkit::ModelHierarchyNode& node) {
            return Matrix4x4Data(node.transform);
        });

    // Register vector types
    register_vector<zenkit::ModelHierarchyNode>("VectorModelHierarchyNode");

    // ModelMesh class registration (needed for getMesh to work with allow_raw_pointers)
    // We don't expose properties directly since we use allow_raw_pointers() for the return value
    class_<zenkit::ModelMesh>("ModelMesh");

    // ModelHierarchyWrapper for loading .MDH files separately
    class_<ModelHierarchyWrapper>("ModelHierarchyLoader")
        .function("loadFromArray", &ModelHierarchyWrapper::loadFromArray)
        .function("getLastError", &ModelHierarchyWrapper::getLastError)
        .function("getHierarchy", &ModelHierarchyWrapper::getHierarchy, allow_raw_pointers());

    // ModelMeshWrapper for loading .MDM files separately
    class_<ModelMeshWrapper>("ModelMeshLoader")
        .function("loadFromArray", &ModelMeshWrapper::loadFromArray)
        .function("getLastError", &ModelMeshWrapper::getLastError)
        .function("getMesh", &ModelMeshWrapper::getMesh, allow_raw_pointers());

    // Model class for loading .MDL files
    class_<ModelWrapper>("Model")
        .function("load", &ModelWrapper::load)
        .function("loadFromArray", &ModelWrapper::loadFromArray)
        .function("getLastError", &ModelWrapper::getLastError)
        .property("isLoaded", &ModelWrapper::isLoaded)
        .function("getHierarchy", &ModelWrapper::getHierarchy, allow_raw_pointers())
        .function("setHierarchy", &ModelWrapper::setHierarchy, allow_raw_pointers())
        .function("setMesh", &ModelWrapper::setMesh, allow_raw_pointers())
        .function("getSoftSkinMeshes", &ModelWrapper::getSoftSkinMeshes)
        .function("getAttachmentNames", &ModelWrapper::getAttachmentNames)
        .function("getAttachment", &ModelWrapper::getAttachment, allow_raw_pointers())
        .function("convertAttachmentToProcessedMesh", &ModelWrapper::convertAttachmentToProcessedMesh, allow_raw_pointers())
        .function("convertSoftSkinMeshToProcessedMesh", &ModelWrapper::convertSoftSkinMeshToProcessedMesh, allow_raw_pointers())
        .function("calculateGeometryOffset", &ModelWrapper::calculateGeometryOffset, allow_raw_pointers());

    // Factory function for models
    function("createModel", select_overload<std::unique_ptr<ModelWrapper>()>([]() {
        return std::make_unique<ModelWrapper>();
    }));

    // Factory functions for separate hierarchy and mesh loaders
    function("createModelHierarchyLoader", select_overload<std::unique_ptr<ModelHierarchyWrapper>()>([]() {
        return std::make_unique<ModelHierarchyWrapper>();
    }));

    function("createModelMeshLoader", select_overload<std::unique_ptr<ModelMeshWrapper>()>([]() {
        return std::make_unique<ModelMeshWrapper>();
    }));

    // MorphMesh classes
    class_<MorphAnimationWrapper>("MorphAnimation")
        .property("name", &MorphAnimationWrapper::getName)
        .property("layer", &MorphAnimationWrapper::getLayer)
        .property("blendIn", &MorphAnimationWrapper::getBlendIn)
        .property("blendOut", &MorphAnimationWrapper::getBlendOut)
        .property("duration", &MorphAnimationWrapper::getDuration)
        .property("speed", &MorphAnimationWrapper::getSpeed)
        .property("flags", &MorphAnimationWrapper::getFlags)
        .property("frameCount", &MorphAnimationWrapper::getFrameCount)
        .function("getVertices", &MorphAnimationWrapper::getVertices)
        .function("getSamples", &MorphAnimationWrapper::getSamples);

    // Register vector types
    register_vector<zenkit::MorphAnimation>("VectorMorphAnimation");

    // MorphMesh wrapper
    class_<MorphMeshWrapper>("MorphMesh")
        .function("loadFromArray", &MorphMeshWrapper::loadFromArray)
        .function("getLastError", &MorphMeshWrapper::getLastError)
        .property("isLoaded", &MorphMeshWrapper::isLoaded)
        .function("getMesh", &MorphMeshWrapper::getMesh, allow_raw_pointers())
        .function("getAnimationsCount", &MorphMeshWrapper::getAnimationsCount)
        .function("getMorphPositionsCount", &MorphMeshWrapper::getMorphPositionsCount)
        .function("convertToProcessedMesh", &MorphMeshWrapper::convertToProcessedMesh)
        .function("getAnimationNames", &MorphMeshWrapper::getAnimationNames);

    // Factory function for morph meshes
    function("createMorphMesh", select_overload<std::unique_ptr<MorphMeshWrapper>()>([]() {
        return std::make_unique<MorphMeshWrapper>();
    }));
}

// DaedalusScript and DaedalusVm bindings
EMSCRIPTEN_BINDINGS(zenkit_daedalus) {
    using namespace zenkit::wasm;
    using namespace emscripten;

    // Result types for function calls (BoolResult is already registered in world_bindings.cc)
    class_<Result<int32_t>>("IntResult")
        .property("success", &Result<int32_t>::success)
        .property("data", &Result<int32_t>::data)
        .property("errorMessage", &Result<int32_t>::error_message);

    class_<Result<std::string>>("StringResult")
        .property("success", &Result<std::string>::success)
        .property("data", &Result<std::string>::data)
        .property("errorMessage", &Result<std::string>::error_message);

    class_<Result<emscripten::val>>("ValResult")
        .property("success", &Result<emscripten::val>::success)
        .property("data", &Result<emscripten::val>::data)
        .property("errorMessage", &Result<emscripten::val>::error_message);

    // DaedalusScript wrapper
    class_<DaedalusScriptWrapper>("DaedalusScript")
        .constructor<>()
        .function("loadFromArray", &DaedalusScriptWrapper::loadFromArray)
        .function("getLastError", &DaedalusScriptWrapper::getLastError)
        .property("isLoaded", &DaedalusScriptWrapper::isLoaded)
        .property("symbolCount", &DaedalusScriptWrapper::getSymbolCount);

    // Factory function for DaedalusScript
    function("createDaedalusScript", select_overload<std::unique_ptr<DaedalusScriptWrapper>()>([]() {
        return std::make_unique<DaedalusScriptWrapper>();
    }));

    // DaedalusVm wrapper (no constructor exposed, use factory function instead)
    class_<DaedalusVmWrapper>("DaedalusVm")
        .property("symbolCount", &DaedalusVmWrapper::getSymbolCount)
        .function("getSymbolString", &DaedalusVmWrapper::getSymbolString)
        .function("getSymbolInt", &DaedalusVmWrapper::getSymbolInt)
        .function("getSymbolFloat", &DaedalusVmWrapper::getSymbolFloat)
        .function("hasSymbol", &DaedalusVmWrapper::hasSymbol)
        .function("getSymbolNameByIndex", &DaedalusVmWrapper::getSymbolNameByIndex)
        .function("getInstancePropertyByIndex", &DaedalusVmWrapper::getInstancePropertyByIndex)
        .function("callFunction", &DaedalusVmWrapper::callFunction, emscripten::allow_raw_pointers())
        .function("registerExternal", &DaedalusVmWrapper::registerExternal)
        .function("setGlobalSelf", &DaedalusVmWrapper::setGlobalSelf)
        .function("setGlobalOther", &DaedalusVmWrapper::setGlobalOther)
        .function("initInstanceByIndex", &DaedalusVmWrapper::initInstanceByIndex)
        .function("setDefaultExternalHandler", &DaedalusVmWrapper::setDefaultExternalHandler);

    // Factory function for DaedalusVm (takes ownership of script via pointer)
    function("createDaedalusVm", select_overload<std::unique_ptr<DaedalusVmWrapper>(DaedalusScriptWrapper*)>([](DaedalusScriptWrapper* script) {
        return std::make_unique<DaedalusVmWrapper>(script);
    }), allow_raw_pointers());

    // CutsceneLibrary wrapper
    class_<CutsceneLibraryWrapper>("CutsceneLibrary")
        .constructor<>()
        .function("loadFromArray", &CutsceneLibraryWrapper::loadFromArray)
        .function("getLastError", &CutsceneLibraryWrapper::getLastError)
        .property("isLoaded", &CutsceneLibraryWrapper::isLoaded)
        .property("blockCount", &CutsceneLibraryWrapper::getBlockCount)
        .function("getBlockByName", &CutsceneLibraryWrapper::getBlockByName);

    // Factory function for CutsceneLibrary
    function("createCutsceneLibrary", select_overload<std::unique_ptr<CutsceneLibraryWrapper>()>([]() {
        return std::make_unique<CutsceneLibraryWrapper>();
    }));

    // ModelScript wrapper
    class_<ModelScriptWrapper>("ModelScript")
        .constructor<>()
        .function("loadFromArray", &ModelScriptWrapper::loadFromArray)
        .function("getLastError", &ModelScriptWrapper::getLastError)
        .function("getSkeletonName", &ModelScriptWrapper::getSkeletonName)
        .function("isSkeletonMeshDisabled", &ModelScriptWrapper::isSkeletonMeshDisabled)
        .function("getMeshCount", &ModelScriptWrapper::getMeshCount)
        .function("getMeshName", &ModelScriptWrapper::getMeshName)
        .function("getDisabledAnimationCount", &ModelScriptWrapper::getDisabledAnimationCount)
        .function("getDisabledAnimationName", &ModelScriptWrapper::getDisabledAnimationName)
        .function("getAnimationCount", &ModelScriptWrapper::getAnimationCount)
        .function("getAnimationName", &ModelScriptWrapper::getAnimationName)
        .function("getAnimationLayer", &ModelScriptWrapper::getAnimationLayer)
        .function("getAnimationNext", &ModelScriptWrapper::getAnimationNext)
        .function("getAnimationBlendIn", &ModelScriptWrapper::getAnimationBlendIn)
        .function("getAnimationBlendOut", &ModelScriptWrapper::getAnimationBlendOut)
        .function("getAnimationFlags", &ModelScriptWrapper::getAnimationFlags)
        .function("getAnimationModel", &ModelScriptWrapper::getAnimationModel)
        .function("getAnimationFirstFrame", &ModelScriptWrapper::getAnimationFirstFrame)
        .function("getAnimationLastFrame", &ModelScriptWrapper::getAnimationLastFrame)
        .function("getAnimationFps", &ModelScriptWrapper::getAnimationFps)
        .function("getAnimationSpeed", &ModelScriptWrapper::getAnimationSpeed);

    // Factory function for ModelScript
    function("createModelScript", select_overload<std::unique_ptr<ModelScriptWrapper>()>([]() {
        return std::make_unique<ModelScriptWrapper>();
    }));

    // ModelAnimation wrapper
    class_<ModelAnimationWrapper>("ModelAnimation")
        .constructor<>()
        .function("loadFromArray", &ModelAnimationWrapper::loadFromArray)
        .function("getLastError", &ModelAnimationWrapper::getLastError)
        .function("getName", &ModelAnimationWrapper::getName)
        .function("getNext", &ModelAnimationWrapper::getNext)
        .function("getLayer", &ModelAnimationWrapper::getLayer)
        .function("getFrameCount", &ModelAnimationWrapper::getFrameCount)
        .function("getNodeCount", &ModelAnimationWrapper::getNodeCount)
        .function("getNodeIndexCount", &ModelAnimationWrapper::getNodeIndexCount)
        .function("getFps", &ModelAnimationWrapper::getFps)
        .function("getFpsSource", &ModelAnimationWrapper::getFpsSource)
        .function("getSampleCount", &ModelAnimationWrapper::getSampleCount)
        .function("getSample", &ModelAnimationWrapper::getSample)
        .function("getNodeIndex", &ModelAnimationWrapper::getNodeIndex);

    // Factory function for ModelAnimation
    function("createModelAnimation", select_overload<std::unique_ptr<ModelAnimationWrapper>()>([]() {
        return std::make_unique<ModelAnimationWrapper>();
    }));
}