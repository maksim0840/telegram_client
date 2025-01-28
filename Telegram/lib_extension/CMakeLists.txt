add_subdirectory(extension/encryption)
add_subdirectory(extension/local_storage)
add_subdirectory(ThirdParty/SQLiteCpp)

# связываем другие модули с расширением
target_link_libraries(Telegram
PUBLIC
    extension_encryption
)