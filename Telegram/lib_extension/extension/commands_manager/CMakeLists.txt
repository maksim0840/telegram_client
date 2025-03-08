add_library(extension_commands_manager STATIC)
init_target(extension_commands_manager "(extension)")

get_filename_component(src_loc ../.. REALPATH)

nice_target_sources(extension_commands_manager ${src_loc}
PRIVATE
    extension/commands_manager/chat_commands_manager.cpp
    extension/commands_manager/chat_commands_manager.h
    extension/commands_manager/message_options.cpp
    extension/commands_manager/message_options.h
)

target_include_directories(extension_commands_manager
PUBLIC
    ${src_loc}
)

target_link_libraries(extension_commands_manager
PUBLIC
    extension_local_storage
    extension_keys_manager
)
