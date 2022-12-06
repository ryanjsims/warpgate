option(ARGPARSE_INSTALL OFF)
add_subdirectory(argparse)

option(GLI_TEST_ENABLE OFF)
add_subdirectory(gli)

option(TINYGLTF_INSTALL OFF)
option(TINYGLTF_BUILD_LOADER_EXAMPLE OFF)
add_subdirectory(tinygltf)

add_subdirectory(lzham_alpha)

add_subdirectory(spdlog)

add_subdirectory(synthium)