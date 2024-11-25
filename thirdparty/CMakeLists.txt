# Adding some hardcoded thirdparty libs
add_subdirectory(GSL)
include_directories(SYSTEM GSL/include)

add_subdirectory(glm)
include_directories(SYSTEM glm)

add_subdirectory(draco)
find_package(draco REQUIRED)

add_subdirectory(tinygltf SYSTEM)
#file(CREATE_LINK "${CMAKE_CURRENT_LIST_DIR}/tinygltf/json.hpp" "${CMAKE_CURRENT_LIST_DIR}/tinygltf/tinygltf/json.hpp" COPY_ON_ERROR)
#file(CREATE_LINK "${CMAKE_CURRENT_LIST_DIR}/tinygltf/stb_image.h" "${CMAKE_CURRENT_LIST_DIR}/tinygltf/tinygltf/stb_image.h" COPY_ON_ERROR)
#file(CREATE_LINK "${CMAKE_CURRENT_LIST_DIR}/tinygltf/stb_image_write.h" "${CMAKE_CURRENT_LIST_DIR}/tinygltf/tinygltf/stb_image_write.h" COPY_ON_ERROR)
#file(CREATE_LINK "${CMAKE_CURRENT_LIST_DIR}/tinygltf/tiny_gltf.h" "${CMAKE_CURRENT_LIST_DIR}/tinygltf/tinygltf/tiny_gltf.h" COPY_ON_ERROR)

target_include_directories(tinygltf PUBLIC draco/src)
target_include_directories(tinygltf PUBLIC draco/build)
target_compile_definitions(tinygltf PUBLIC TINYGLTF_ENABLE_DRACO)
#target_link_libraries(tinygltf PUBLIC thirdparty::draco)


add_library(thirdparty::GSL ALIAS GSL)
add_library(thirdparty::glm ALIAS glm)
#add_library(thirdparty::draco ALIAS draco)
add_library(thirdparty::tinygltf ALIAS tinygltf)
