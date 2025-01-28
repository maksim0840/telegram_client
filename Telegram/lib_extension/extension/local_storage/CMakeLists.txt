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
    SQLiteCpp
    sqlite3
    pthread
    dl
)

# Путь до папки с базами данных (определение макроса)
target_compile_definitions(extension_local_storage PRIVATE DB_PATH_DEFINED_BY_CMAKELISTS="${CMAKE_CURRENT_SOURCE_DIR}/../../db")
