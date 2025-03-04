add_library(extension_decryption STATIC)
init_target(extension_decryption "(extension)")

get_filename_component(src_loc ../.. REALPATH)

nice_target_sources(extension_decryption ${src_loc}
PRIVATE
    extension/decryption/message_text_decryption.cpp
    extension/decryption/message_text_decryption.h
)

target_include_directories(extension_decryption
PUBLIC
    ${src_loc}
)

target_link_libraries(extension_decryption
PUBLIC
    desktop-app::external_qt_static_plugins
    desktop-app::external_qt
    tdesktop::td_scheme # доступ к объектам автогенерирующегося файла scheme.h
)