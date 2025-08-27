include(FetchContent)

# This warning only occurs in CMake 3.24 or greater.
# https://cmake.org/cmake/help/latest/policy/CMP0135.html#cmp0135
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
cmake_policy(SET CMP0135 NEW)
endif ()

set(BUILD_STATIC_LIBS ON)
set(BUILD_SHARED_LIBS OFF)
set(BUILD_SQUISH_WITH_OPENMP OFF CACHE BOOL "" FORCE)

# For WebAssembly builds, disable SSE in libsquish
if (EMSCRIPTEN)
    set(BUILD_SQUISH_EXTRA OFF CACHE BOOL "" FORCE)
    set(BUILD_SQUISH_WITH_SSE2 OFF CACHE BOOL "" FORCE)
    set(BUILD_SQUISH_WITH_ALTIVEC OFF CACHE BOOL "" FORCE)
endif()

function(px_add_dependency NAME URL URL_HASH)
    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/CMakeLists.txt)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/${NAME} EXCLUDE_FROM_ALL)
        set(${NAME}_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${NAME} CACHE PATH "Path to the sources for the ${NAME} library." FORCE)
    else ()
        FetchContent_Declare(
                ${NAME}
                URL ${URL}
                URL_HASH SHA1=${URL_HASH}
        )

        if (NOT ${NAME}_POPULATED)
            message(STATUS "Downloading ${NAME}")
            FetchContent_Populate(${NAME})
            add_subdirectory(${${NAME}_SOURCE_DIR} ${${NAME}_BINARY_DIR} EXCLUDE_FROM_ALL)
        endif ()

        set(${NAME}_SOURCE_DIR ${${NAME}_SOURCE_DIR} CACHE PATH "Path to the sources for the ${NAME} library." FORCE)
    endif ()
endfunction()

# Only include doctest if we're building tests (not needed for WebAssembly)
if (ZK_BUILD_TESTS)
    px_add_dependency(doctest https://github.com/doctest/doctest/archive/refs/tags/v2.4.9.zip d1563419fa370c34c90e028c2e903a70c8dc07b2)
endif()

px_add_dependency(libsquish https://github.com/lmichaelis/phoenix-libsquish/archive/cc82beff55210816e1bd531fc6057203dc309807.zip 953f5cd072cd6674d1aeaff5ff91225f2197283c)

# msvc: disable -wno-* flags
if (NOT MSVC)
    target_compile_options(squish PRIVATE -Wno-unused-but-set-variable)
endif ()

# For WebAssembly builds, disable SIMD to avoid SSE compilation issues
if (EMSCRIPTEN)
    target_compile_definitions(squish PRIVATE SQUISH_USE_SSE=0)
    # Remove any SSE-related compile options that libsquish might add
    get_target_property(SQUISH_COMPILE_OPTIONS squish COMPILE_OPTIONS)
    if(SQUISH_COMPILE_OPTIONS)
        list(FILTER SQUISH_COMPILE_OPTIONS EXCLUDE REGEX "-m(sse|avx|fpu)")
        set_target_properties(squish PROPERTIES COMPILE_OPTIONS "${SQUISH_COMPILE_OPTIONS}")
    endif()
endif()

target_include_directories(squish PUBLIC ${libsquish_SOURCE_DIR})
