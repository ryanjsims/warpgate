cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

project(warpgate)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU"))
  add_compile_options("-Wno-deprecated")
endif()

set(MATERIALS_JSON_PORTABLE 1 CACHE BOOL "")
set(CMAKE_VERBOSE_MAKEFILE 0 CACHE BOOL "")
set(BUILD_WARPGATE_HIKOGUI 0 CACHE BOOL "Enable experimental Warpgate Hikogui target. Requires Vulkan and Hikogui")
set(BUILD_WARPGATE_GUI 0 CACHE BOOL "Enable experimental Warpgate GTK target. Requires pkg-config files and dynamic libraries for gtkmm4.0 and dependencies (see FindGTKMM.cmake)")

add_subdirectory(lib)

if(${BUILD_WARPGATE_HIKOGUI} OR ${BUILD_WARPGATE_GUI})
  find_package(Vulkan REQUIRED)
endif()
if(${BUILD_WARPGATE_GUI})
  include(cmake/FindGTKMM.cmake)
endif()

if(${MATERIALS_JSON_PORTABLE})
  set(MATERIALS_JSON_LOCATION "share/materials.json")
else()
  set(MATERIALS_JSON_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/share/materials.json")
endif()

add_definitions(-DMATERIALS_JSON_LOCATION="${MATERIALS_JSON_LOCATION}" -DMATERIALS_JSON_PORTABLE=${MATERIALS_JSON_PORTABLE})

add_custom_target(materials_json
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/share
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/materials.json
)

add_executable(test_adr
  src/test_adr.cpp
  src/utils/adr.cpp
)

SET(PUGIXML_LINKED_LIBRARY pugixml::static)

target_include_directories(test_adr PUBLIC include/)
target_link_libraries(test_adr PRIVATE ${PUGIXML_LINKED_LIBRARY} spdlog::spdlog synthium::synthium)

add_executable(test_dme src/test_dme.cpp)
target_include_directories(test_dme PUBLIC include/ dme_loader/include/)
target_link_libraries(test_dme PRIVATE dme_loader spdlog::spdlog synthium::synthium)

add_executable(test_cnk
  src/test_cnk.cpp
)
target_link_libraries(test_cnk PRIVATE cnk_loader spdlog::spdlog)

add_executable(test_mrn
  src/test_mrn.cpp
)
target_include_directories(test_mrn PUBLIC include/)
target_link_libraries(test_mrn PUBLIC mrn_loader spdlog::spdlog)

add_executable(test_zone
  src/test_zone.cpp
)
target_include_directories(test_zone PUBLIC include/)
target_link_libraries(test_zone PRIVATE zone_loader spdlog::spdlog synthium::synthium gli)

add_executable(adr_converter 
    src/adr_converter.cpp
    src/utils/actor_sockets.cpp
    src/utils/adr.cpp
    src/utils/gltf/common.cpp
    src/utils/gltf.cpp
    src/utils/common.cpp 
    src/utils/materials_3.cpp 
    src/utils/sign.cpp 
    src/utils/textures.cpp
    src/utils/tsqueue.cpp 
)
target_include_directories(adr_converter PUBLIC 
  include/
  ${CMAKE_BINARY_DIR}/include/
  lib/internal/dme_loader/include/
  lib/external/argparse/include/
  lib/external/half/include/
  lib/external/tinygltf/
  lib/external/synthium/include)
target_link_libraries(adr_converter PRIVATE dme_loader ${PUGIXML_LINKED_LIBRARY} spdlog::spdlog tinygltf argparse synthium::synthium gli)

add_executable(decompress
  src/decompress.cpp
)
target_include_directories(decompress PUBLIC include/ PRIVATE lib/external/synthium/external/zlib ${CMAKE_BINARY_DIR}/lib/external/synthium/external/zlib)
target_link_libraries(decompress PRIVATE spdlog::spdlog argparse ZLIB::ZLIB)

add_executable(export
  src/export.cpp
  src/utils/common.cpp
  src/utils/textures.cpp
  src/utils/materials_3.cpp
)
target_include_directories(export PUBLIC include/ lib/internal/cnk_loader/include/)
target_link_libraries(export PRIVATE spdlog::spdlog synthium::synthium argparse Glob cnk_loader gli tinygltf)

add_executable(dme_converter 
    src/dme_converter.cpp
    src/utils/gltf/common.cpp
    src/utils/common.cpp
    src/utils/gltf.cpp
    src/utils/materials_3.cpp 
    src/utils/sign.cpp 
    src/utils/textures.cpp
    src/utils/tsqueue.cpp 
)
target_include_directories(dme_converter PUBLIC 
  include/
  ${CMAKE_BINARY_DIR}/include/
  lib/internal/dme_loader/include/
  lib/external/argparse/include/
  lib/external/half/include/
  lib/external/tinygltf/
  lib/external/synthium/include)
target_link_libraries(dme_converter PRIVATE dme_loader spdlog::spdlog tinygltf argparse synthium::synthium gli ${PUGIXML_LINKED_LIBRARY})

add_executable(chunk_converter
    src/chunk_converter.cpp
    src/utils/gltf/chunk.cpp
    src/utils/gltf/common.cpp
    src/utils/aabb.cpp
    src/utils/common.cpp
    src/utils/materials_3.cpp 
    src/utils/sign.cpp 
    src/utils/textures.cpp
    src/utils/tsqueue.cpp
)
target_include_directories(chunk_converter 
  PUBLIC 
    include/ 
    ${CMAKE_BINARY_DIR}/include/ 
    lib/internal/cnk_loader/include/
    lib/internal/dme_loader/include/
    lib/external/argparse/include/ 
    lib/external/half/include/ 
    lib/external/synthium/include
    lib/external/tinygltf/ 
)
target_link_libraries(chunk_converter 
  PRIVATE 
    argparse 
    cnk_loader
    Glob
    gli
    spdlog::spdlog 
    synthium::synthium 
    tinygltf 
)

add_executable(mrn_converter
    src/mrn_converter.cpp
)
target_include_directories(mrn_converter PUBLIC include/)
target_link_libraries(mrn_converter PRIVATE argparse Glob gli mrn_loader spdlog::spdlog synthium::synthium tinygltf)

if(${BUILD_WARPGATE_HIKOGUI})
  add_executable(warpgate_hi WIN32 
    src/warpgate_hi.cpp
    src/utils/vulkan/model_renderer.cpp
    src/utils/hikogui/widgets/model_widget.cpp
    src/utils/common.cpp
    src/utils/materials_3.cpp
  )

  target_include_directories(warpgate PUBLIC include/ lib/external/half/include/)
  target_link_libraries(warpgate PRIVATE hikogui gli synthium::synthium dme_loader mrn_loader spdlog::spdlog tinygltf)

  #set_property(TARGET warpgate PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

  add_custom_target(vulkan_warpgate_shaders)
  add_shader_target(vulkan_warpgate_shaders
      src/utils/vulkan/shaders/character.vert
      src/utils/vulkan/shaders/character.frag
      src/utils/vulkan/shaders/grid.vert
      src/utils/vulkan/shaders/grid.frag
      src/utils/vulkan/shaders/ModelRigid.vert
      src/utils/vulkan/shaders/ModelRigid.frag
      src/utils/vulkan/shaders/structure.vert
      src/utils/vulkan/shaders/structure.frag
  )

  add_custom_target(vulkan_warpgate_resources
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/lib/external/hikogui/resources ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/resources
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/resources
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/lib/external/hikogui/shaders ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/resources/shaders
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/src/utils/vulkan/shaders ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/resources/shaders
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/resources
      DEPENDS
      hikogui vulkan_warpgate_shaders
      resources/warpgate.png
  )
endif()
if(${BUILD_WARPGATE_GUI})
  add_executable(hello_gtk WIN32
    src/hello_gtk.cpp  
  )

  target_link_directories(hello_gtk PRIVATE ${GTKMM_LIBRARY_DIRS})
  target_link_libraries(hello_gtk PRIVATE ${GTKMM_LIBRARIES} ${Vulkan_LIBRARY})
  target_include_directories(hello_gtk PRIVATE ${GTKMM_INCLUDE_DIRS} ${Vulkan_INCLUDE_DIR})
  target_compile_options(hello_gtk PRIVATE ${GTKMM_CFLAGS_OTHER})
  copy_gtk_dlls(hello_gtk)

  file(TOUCH ${CMAKE_BINARY_DIR}/warpgate_icon.o)
  add_executable(warpgate
    src/warpgate.cpp
    src/utils/gtk/bone.cpp
    src/utils/gtk/common.cpp
    src/utils/gtk/loaded_list_item.cpp
    src/utils/gtk/mesh.cpp
    src/utils/gtk/model_renderer.cpp
    src/utils/gtk/model.cpp
    src/utils/gtk/skeleton.cpp
    src/utils/gtk/shader.cpp
    src/utils/gtk/texture.cpp
    src/utils/gtk/window.cpp
    src/utils/gltf/common.cpp
    src/utils/actor_sockets.cpp
    src/utils/adr.cpp
    src/utils/common.cpp
    src/utils/gltf.cpp
    src/utils/materials_3.cpp
    src/utils/sign.cpp
    src/utils/textures.cpp
    src/utils/tsqueue.cpp
    ${CMAKE_BINARY_DIR}/warpgate_icon.o
  )

  target_link_directories(warpgate PRIVATE ${GTKMM_LIBRARY_DIRS} ${LIBEPOXY_LIBRARY_DIRS})
  target_link_libraries(warpgate
    PRIVATE 
      gli 
      synthium::synthium
      dme_loader
      mrn_loader
      spdlog::spdlog
      tinygltf
      ${PUGIXML_LINKED_LIBRARY}
      ${GTKMM_LIBRARIES}
      ${LIBEPOXY_LIBRARIES}
      ${Vulkan_LIBRARY}
  )
  target_include_directories(warpgate PRIVATE include/ lib/external/half/include/ ${GTKMM_INCLUDE_DIRS} ${Vulkan_INCLUDE_DIR})
  target_compile_options(warpgate PRIVATE ${GTKMM_CFLAGS_OTHER} ${EPOXY_CFLAGS_OTHER})
  if(WIN32)
    target_compile_definitions(warpgate PUBLIC /wdC4250)
  else()

  endif()
  copy_gtk_dlls(warpgate)

  add_custom_target(opengl_warpgate_resources
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/utils/vulkan/shaders ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/share/shaders
  )

  add_custom_target(gdk_pixbuf_loaders
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/lib/gdk-pixbuf-2.0 ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/lib/gdk-pixbuf-2.0
  )

  add_custom_target(bundle_icons
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/share/icons ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/share/icons
  )

  add_custom_target(update_icon_cache
    COMMAND "${GTK4_PREFIX}/bin/gtk4-update-icon-cache.exe" -t -q ${CMAKE_SOURCE_DIR}/share/icons/hicolor
  )

  add_custom_target(compile_svg
    COMMAND cairosvg -W 32 -H 32 ${CMAKE_SOURCE_DIR}/share/icons/hicolor/scalable/apps/warpgate.svg -o ${CMAKE_SOURCE_DIR}/share/icons/hicolor/32x32/apps/warpgate.png
    COMMAND cairosvg -W 48 -H 48 ${CMAKE_SOURCE_DIR}/share/icons/hicolor/scalable/apps/warpgate.svg -o ${CMAKE_SOURCE_DIR}/share/icons/hicolor/48x48/apps/warpgate.png
    COMMAND cairosvg -W 64 -H 64 ${CMAKE_SOURCE_DIR}/share/icons/hicolor/scalable/apps/warpgate.svg -o ${CMAKE_SOURCE_DIR}/share/icons/hicolor/64x64/apps/warpgate.png
    COMMAND cairosvg -W 128 -H 128 ${CMAKE_SOURCE_DIR}/share/icons/hicolor/scalable/apps/warpgate.svg -o ${CMAKE_SOURCE_DIR}/share/icons/hicolor/128x128/apps/warpgate.png
    COMMAND cairosvg -W 256 -H 256 ${CMAKE_SOURCE_DIR}/share/icons/hicolor/scalable/apps/warpgate.svg -o ${CMAKE_SOURCE_DIR}/share/icons/hicolor/256x256/apps/warpgate.png
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
    DEPENDS move_cairo
  )

  add_custom_command(TARGET compile_svg PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libcairo> ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:pixman> ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libpng> ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:zlib_shared> ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libfreetype> ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libfontconfig> ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libexpat> ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/
  )

  add_custom_target(compile_icon
    COMMAND python ${CMAKE_SOURCE_DIR}/script/png2ico.py ${CMAKE_SOURCE_DIR}/share/icons/hicolor/256x256/apps/warpgate.png ${CMAKE_SOURCE_DIR}/warpgate.ico
    COMMAND windres ${CMAKE_SOURCE_DIR}/warpgate.rc -o ${CMAKE_BINARY_DIR}/warpgate_icon.o
    DEPENDS
    ${CMAKE_SOURCE_DIR}/share/icons/hicolor/256x256/apps/warpgate.png
    ${CMAKE_SOURCE_DIR}/script/png2ico.py
    BYPRODUCTS ${CMAKE_BINARY_DIR}/warpgate_icon.o
  )

  add_custom_command(TARGET warpgate POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:zlib> ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/zlib.dll
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${GTK4_PREFIX}/share/glib-2.0/ ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/share/glib-2.0/
  )

  add_dependencies(compile_icon compile_svg)
  add_dependencies(update_icon_cache compile_svg)
  add_dependencies(bundle_icons update_icon_cache)
  add_dependencies(warpgate opengl_warpgate_resources gdk_pixbuf_loaders materials_json bundle_icons compile_icon)
endif()
    
add_executable(zone_converter 
    src/zone_converter.cpp
    src/utils/gltf/common.cpp
    src/utils/gltf/chunk.cpp
    src/utils/aabb.cpp
    src/utils/adr.cpp
    src/utils/common.cpp
    src/utils/gltf.cpp
    src/utils/materials_3.cpp
    src/utils/sign.cpp 
    src/utils/textures.cpp
    src/utils/tsqueue.cpp 
)
target_include_directories(zone_converter PUBLIC 
  include/
  ${CMAKE_BINARY_DIR}/include/
  lib/internal/cnk_loader/include/
  lib/internal/dme_loader/include/
  lib/external/argparse/include/
  lib/external/half/include/
  lib/external/tinygltf/
  lib/external/synthium/include
)
target_link_libraries(zone_converter PRIVATE cnk_loader dme_loader zone_loader ${PUGIXML_LINKED_LIBRARY} spdlog::spdlog tinygltf argparse synthium::synthium gli Glob)

find_package(Git)
add_custom_target(version
  ${CMAKE_COMMAND} -D SRC=${CMAKE_SOURCE_DIR}/include/version.h.in
                   -D DST=${CMAKE_SOURCE_DIR}/include/version.h
                   -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
                   -P ${CMAKE_SOURCE_DIR}/lib/external/cmake-git-versioning/GenerateVersionHeader.cmake
)

add_dependencies(adr_converter version materials_json)
add_dependencies(chunk_converter version materials_json)
add_dependencies(dme_converter version materials_json)
add_dependencies(export version materials_json)
add_dependencies(zone_converter version materials_json)

add_dependencies(test_cnk version)
add_dependencies(test_dme version)
add_dependencies(test_mrn version)
add_dependencies(test_zone version)

if(${BUILD_WARPGATE_HIKOGUI})
  add_dependencies(warpgate_hi vulkan_warpgate_resources version)
endif()