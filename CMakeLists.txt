cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

project(warpgate)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU"))
  add_compile_options("-Wno-deprecated")
endif()

set(MATERIALS_JSON_PORTABLE 1 CACHE BOOL "")
set(CMAKE_VERBOSE_MAKEFILE 0 CACHE BOOL "")
set(BUILD_WARPGATE_GUI 0 CACHE BOOL "Enable experimental Warpgate GUI target. Requires Vulkan and Hikogui")

add_subdirectory(lib)

function(add_pkgconfig_dll_library name package dllname)
  string(TOUPPER ${name} name_upper)
  pkg_check_modules(${name_upper} REQUIRED ${package})
  pkg_get_variable(${name_upper}_PREFIX ${package} prefix)
  add_library(${name} SHARED IMPORTED GLOBAL)
  set_target_properties(${name} 
    PROPERTIES 
      IMPORTED_IMPLIB "${${name_upper}_LIBRARY_DIRS}/${${name_upper}_LIBRARIES}"
      IMPORTED_LOCATION "${${name_upper}_PREFIX}/bin/${dllname}"
  )
endfunction()

if(${BUILD_WARPGATE_GUI})
  find_package(Vulkan REQUIRED)
  find_package(PkgConfig REQUIRED)
  add_pkgconfig_dll_library(gtkmm gtkmm-4.0 "gtkmm-vc143-4.0-0.dll")
  add_pkgconfig_dll_library(glibmm glibmm-2.68 "glibmm-vc142-2.68-1.dll")
  add_pkgconfig_dll_library(giomm giomm-2.68 "giomm-vc142-2.68-1.dll")
  add_pkgconfig_dll_library(sigc++ sigc++-3.0 "sigc-3.0-0.dll")
  add_pkgconfig_dll_library(gobject gobject-2.0 "gobject-2.0-0.dll")
  add_pkgconfig_dll_library(gio glib-2.0 "gio-2.0-0.dll")
  add_pkgconfig_dll_library(glib gio-2.0 "glib-2.0-0.dll")
  add_pkgconfig_dll_library(gmodule gmodule-2.0 "gmodule-2.0-0.dll")
  add_pkgconfig_dll_library(gtk4 gtk4 "gtk-4-1.dll")
  add_pkgconfig_dll_library(gdk_pixbuf gdk-pixbuf-2.0 "gdk_pixbuf-2.0-0.dll")
  add_pkgconfig_dll_library(cairo_gobject cairo-gobject "cairo-gobject-2.dll")
  add_pkgconfig_dll_library(graphene graphene-1.0 "graphene-1.0-0.dll")
  add_pkgconfig_dll_library(cairomm cairomm-1.16 "cairomm-vc143-1.16-1.dll")
  add_pkgconfig_dll_library(pangomm pangomm-2.48 "pangomm-vc143-2.48-1.dll")
  add_pkgconfig_dll_library(ffi libffi "ffi-7.dll")
  add_pkgconfig_dll_library(zlib_shared zlib "zlib1.dll")
  add_pkgconfig_dll_library(pcre libpcre2-8 "pcre2-8.dll")
  list(GET GTK4_LIBRARY_DIRS 1 INTL_LIBRARY_DIR)
  add_library(intl SHARED IMPORTED GLOBAL)
  set_target_properties(intl 
    PROPERTIES 
      IMPORTED_IMPLIB "${INTL_LIBRARY_DIR}/intl.lib"
      IMPORTED_LOCATION "${GTK4_PREFIX}/bin/intl.dll"
  )
  add_library(iconv SHARED IMPORTED GLOBAL)
  set_target_properties(iconv 
    PROPERTIES 
      IMPORTED_IMPLIB "${INTL_LIBRARY_DIR}/iconv.lib"
      IMPORTED_LOCATION "${GTK4_PREFIX}/bin/iconv.dll"
  )
  add_library(libexpat SHARED IMPORTED GLOBAL)
  set_target_properties(libexpat 
    PROPERTIES 
      IMPORTED_IMPLIB "${INTL_LIBRARY_DIR}/libexpat.lib"
      IMPORTED_LOCATION "${GTK4_PREFIX}/bin/libexpat.dll"
  )
  add_pkgconfig_dll_library(libpng libpng16 "libpng16.dll")
  add_pkgconfig_dll_library(libcairo cairo "cairo-2.dll")
  add_pkgconfig_dll_library(libjpeg libjpeg "jpeg62.dll")
  add_pkgconfig_dll_library(libtiff libtiff-4 "tiff.dll")
  add_pkgconfig_dll_library(pangocairo pangocairo "pangocairo-1.0-0.dll")
  add_pkgconfig_dll_library(libpango pango "pango-1.0-0.dll")
  add_pkgconfig_dll_library(libpangoft2 pangoft2 "pangoft2-1.0-0.dll")
  add_pkgconfig_dll_library(libpangowin32 pangowin32 "pangowin32-1.0-0.dll")
  add_pkgconfig_dll_library(libharfbuzz harfbuzz "harfbuzz.dll")
  add_pkgconfig_dll_library(fribidi fribidi "fribidi-0.dll")
  add_pkgconfig_dll_library(epoxy epoxy "epoxy-0.dll")
  add_pkgconfig_dll_library(libcairo_script cairo-script "cairo-script-interpreter-2.dll")
  add_pkgconfig_dll_library(pixman pixman-1 "pixman-1-0.dll")
  add_pkgconfig_dll_library(libfreetype freetype2 "freetype-6.dll")
  add_pkgconfig_dll_library(libfontconfig fontconfig "fontconfig-1.dll")
endif()

if(${MATERIALS_JSON_PORTABLE})
  set(MATERIALS_JSON_LOCATION "resources/materials.json")
else()
  set(MATERIALS_JSON_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/resources/materials.json")
endif()

add_definitions(-DMATERIALS_JSON_LOCATION="${MATERIALS_JSON_LOCATION}" -DMATERIALS_JSON_PORTABLE=${MATERIALS_JSON_PORTABLE})

add_custom_target(materials_json
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/resources
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
target_link_libraries(test_zone PRIVATE zone_loader spdlog::spdlog synthium::synthium)

add_executable(adr_converter 
    src/adr_converter.cpp
    src/utils/actor_sockets.cpp
    src/utils/adr.cpp
    src/utils/gltf/common.cpp
    src/utils/gltf.cpp
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
  src/utils/textures.cpp
  src/utils/materials_3.cpp
)
target_include_directories(export PUBLIC include/ lib/internal/cnk_loader/include/)
target_link_libraries(export PRIVATE spdlog::spdlog synthium::synthium argparse Glob cnk_loader gli tinygltf)

add_executable(dme_converter 
    src/dme_converter.cpp
    src/utils/gltf/common.cpp
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
target_link_libraries(dme_converter PRIVATE dme_loader spdlog::spdlog tinygltf argparse synthium::synthium gli)

add_executable(chunk_converter
    src/chunk_converter.cpp
    src/utils/gltf/chunk.cpp
    src/utils/gltf/common.cpp
    src/utils/aabb.cpp
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

if(${BUILD_WARPGATE_GUI})
  add_executable(warpgate WIN32 
    src/warpgate.cpp
    src/utils/vulkan/model_renderer.cpp
    src/utils/hikogui/widgets/model_widget.cpp
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

  add_executable(hello_gtk WIN32
    src/hello_gtk.cpp  
  )

  target_link_directories(hello_gtk PRIVATE ${GTKMM_LIBRARY_DIRS})
  target_link_libraries(hello_gtk PRIVATE ${GTKMM_LIBRARIES} ${Vulkan_LIBRARY})
  target_include_directories(hello_gtk PRIVATE ${GTKMM_INCLUDE_DIRS} ${Vulkan_INCLUDE_DIR})
  target_compile_options(hello_gtk PRIVATE ${GTKMM_CFLAGS_OTHER})
  add_custom_command(TARGET hello_gtk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:gtkmm> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:glibmm> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:giomm> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:sigc++> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:gobject> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:glib> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:gio> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:gtk4> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:gdk_pixbuf> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:cairo_gobject> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:graphene> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:cairomm> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:pangomm> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:gmodule> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ffi> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:zlib_shared> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:intl> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:pcre> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libpng> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libpango> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libpangoft2> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libpangowin32> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libcairo> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:pangocairo> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libtiff> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libjpeg> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:iconv> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:epoxy> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:fribidi> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libharfbuzz> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libcairo_script> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:pixman> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libfreetype> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libfontconfig> $<TARGET_FILE_DIR:hello_gtk>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libexpat> $<TARGET_FILE_DIR:hello_gtk>
    )
    endif()
    
add_executable(zone_converter 
    src/zone_converter.cpp
    src/utils/gltf/common.cpp
    src/utils/gltf/chunk.cpp
    src/utils/aabb.cpp
    src/utils/adr.cpp
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

if(${BUILD_WARPGATE_GUI})
  add_dependencies(warpgate vulkan_warpgate_resources version)
endif()