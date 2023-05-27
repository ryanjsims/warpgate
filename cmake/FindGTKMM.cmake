find_package(PkgConfig REQUIRED)
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
add_library(libxml2 SHARED IMPORTED GLOBAL)
set_target_properties(libxml2 
PROPERTIES 
  IMPORTED_IMPLIB "${INTL_LIBRARY_DIR}/libxml2.lib"
  IMPORTED_LOCATION "${GTK4_PREFIX}/bin/libxml2.dll"
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
add_pkgconfig_dll_library(libepoxy epoxy "epoxy-0.dll")
add_pkgconfig_dll_library(libcairo_script cairo-script "cairo-script-interpreter-2.dll")
add_pkgconfig_dll_library(pixman pixman-1 "pixman-1-0.dll")
add_pkgconfig_dll_library(libfreetype freetype2 "freetype-6.dll")
add_pkgconfig_dll_library(libfontconfig fontconfig "fontconfig-1.dll")
add_pkgconfig_dll_library(librsvg librsvg-2.0 "rsvg-2.0-vs17.dll")

function(copy_gtk_dlls target)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:gtkmm> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:glibmm> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:giomm> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:sigc++> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:gobject> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:glib> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:gio> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:gtk4> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:gdk_pixbuf> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:cairo_gobject> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:graphene> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:cairomm> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:pangomm> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:gmodule> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ffi> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:zlib_shared> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:intl> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:pcre> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libpng> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libpango> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libpangoft2> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libpangowin32> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libcairo> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:pangocairo> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libtiff> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libjpeg> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:iconv> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libepoxy> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:fribidi> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libharfbuzz> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libcairo_script> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:pixman> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libfreetype> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libfontconfig> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libexpat> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:librsvg> $<TARGET_FILE_DIR:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libxml2> $<TARGET_FILE_DIR:${target}>
  )
endfunction()