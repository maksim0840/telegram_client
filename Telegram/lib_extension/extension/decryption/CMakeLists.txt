add_library(extension_decryption STATIC)
init_target(extension_decryption "(extension)")

get_filename_component(src_loc ../.. REALPATH)

nice_target_sources(extension_decryption ${src_loc}
PRIVATE
    extension/decryption/mtp_buffer_decryption.cpp
    extension/decryption/mtp_buffer_decryption.h
)

target_include_directories(extension_decryption
PUBLIC
    ${src_loc}
)

target_link_libraries(extension_decryption
PUBLIC
    desktop-app::external_qt_static_plugins
    desktop-app::external_qt
    extension_commands_manager
    desktop-app::lib_base
    desktop-app::lib_ui
    tdesktop::td_mtproto
    tdesktop::td_scheme
)