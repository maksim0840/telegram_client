add_library(extension_encryption STATIC)
init_target(extension_encryption "(extension)")

get_filename_component(src_loc ../.. REALPATH)

nice_target_sources(extension_encryption ${src_loc}
PRIVATE
    extension/encryption/mtp_buffer_encryption.cpp
    extension/encryption/mtp_buffer_encryption.h
)

target_include_directories(extension_encryption
PUBLIC
    ${src_loc}
)

target_link_libraries(extension_encryption
PUBLIC
    desktop-app::external_qt_static_plugins
    desktop-app::external_qt
    extension_commands_manager
    desktop-app::lib_base
    desktop-app::lib_ui
    tdesktop::td_mtproto
)