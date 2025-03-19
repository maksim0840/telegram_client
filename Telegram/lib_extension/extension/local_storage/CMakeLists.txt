
# Проверяем передачу пути до папки с базами данных (определяется в run.sh)
if(NOT DEFINED DB_PATH)
    set(DB_PATH "default/path/to/db" CACHE INTERNAL "Database path")  # Значение по умолчанию
endif()
set(DB_PATH ${DB_PATH} CACHE INTERNAL "Database path")

# Делаем параметр доступным
add_definitions(-DDB_PATH="${DB_PATH}")


add_library(extension_local_storage STATIC)
init_target(extension_local_storage "(extension)")

get_filename_component(src_loc ../.. REALPATH)

nice_target_sources(extension_local_storage ${src_loc}
PRIVATE
    extension/local_storage/local_storage.cpp
    extension/local_storage/local_storage.h
    extension/local_storage/keys_db.cpp
    extension/local_storage/keys_db.h
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
