add_library(extension_buttons STATIC)
init_target(extension_buttons "(extension)")

get_filename_component(src_loc ../.. REALPATH)

nice_target_sources(extension_buttons ${src_loc}
PRIVATE
    extension/buttons/top_bar_buttons.h
)

target_include_directories(extension_buttons
PUBLIC
    ${src_loc}
)

target_link_libraries(extension_buttons
PUBLIC
    desktop-app::external_qt_static_plugins
    desktop-app::external_qt
)
