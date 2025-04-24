add_library(extension_buttons STATIC)
init_target(extension_buttons "(extension)")

get_filename_component(src_loc ../.. REALPATH)

nice_target_sources(extension_buttons ${src_loc}
PRIVATE
    extension/buttons/top_bar_buttons.h
    extension/buttons/top_bar_buttons.cpp
    extension/buttons/message_bottom_info.h
    extension/buttons/message_bottom_info.cpp
    extension/buttons/icons.h
    extension/buttons/settings_buttons.cpp
    extension/buttons/settings_buttons.h
)

target_include_directories(extension_buttons
PUBLIC
    ${src_loc}
)

target_link_libraries(extension_buttons
PUBLIC
    desktop-app::external_qt_static_plugins
    desktop-app::external_qt
    desktop-app::lib_base
    desktop-app::lib_ui
    desktop-app::lib_rpl
    tdesktop::td_scheme
)
#./Telegram/lib_rpl/rpl/event_stream.h