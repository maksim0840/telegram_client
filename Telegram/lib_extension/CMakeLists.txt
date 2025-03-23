add_subdirectory(extension/encryption)
add_subdirectory(extension/decryption)
add_subdirectory(extension/chat_members)
add_subdirectory(extension/keys_manager)
add_subdirectory(extension/commands_manager)
add_subdirectory(extension/local_storage)
add_subdirectory(extension/buttons)
add_subdirectory(ThirdParty/SQLiteCpp)

# связываем другие модули с расширением
target_link_libraries(Telegram
PUBLIC
    extension_encryption
    extension_decryption
    extension_chat_members
    extension_buttons
)
