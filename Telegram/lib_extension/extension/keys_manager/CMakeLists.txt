add_library(extension_keys_manager STATIC)
init_target(extension_keys_manager "(extension)")

get_filename_component(src_loc ../.. REALPATH)

nice_target_sources(extension_keys_manager ${src_loc}
PRIVATE
    extension/keys_manager/keys_manager.cpp
    extension/keys_manager/keys_manager.h
    extension/keys_manager/keys_format.cpp
    extension/keys_manager/keys_format.h
    extension/keys_manager/dh.cpp
    extension/keys_manager/dh.h
)

target_include_directories(extension_keys_manager
PUBLIC
    ${src_loc}
)

target_link_libraries(extension_keys_manager
PUBLIC
    desktop-app::external_openssl
)
