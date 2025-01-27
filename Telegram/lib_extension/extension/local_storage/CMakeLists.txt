add_library(extension_local_storage STATIC)
init_target(extension_local_storage "(extension)")

get_filename_component(src_loc ../.. REALPATH)

nice_target_sources(extension_local_storage ${src_loc}
PRIVATE
    extension/local_storage/local_storage.cpp
    extension/local_storage/local_storage.h
)

target_include_directories(extension_local_storage
PUBLIC
        ${src_loc}
)

target_link_libraries(extension_local_storage
PUBLIC
    desktop-app::external_qt_static_plugins
    desktop-app::external_qt
    desktop-app::external_openssl
)
