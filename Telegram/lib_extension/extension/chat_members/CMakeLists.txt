add_library(extension_chat_members STATIC)
init_target(extension_chat_members "(extension)")

get_filename_component(src_loc ../.. REALPATH)

nice_target_sources(extension_chat_members ${src_loc}
PRIVATE
    extension/chat_members/chat_members.cpp
    extension/chat_members/chat_members.h
)

target_include_directories(extension_chat_members
PUBLIC
    ${src_loc}
)

target_link_libraries(extension_chat_members
PUBLIC
    desktop-app::external_qt_static_plugins
    desktop-app::external_qt
)