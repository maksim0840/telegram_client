# Enable code coverage flags
SET(CMAKE_CXX_FLAGS "-g -O0 -Wall -fprofile-arcs -ftest-coverage")
SET(CMAKE_C_FLAGS "-g -O0 -Wall -W -fprofile-arcs -ftest-coverage")
SET(CMAKE_EXE_LINKER_FLAGS "-fprofile-arcs -ftest-coverage")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -fprofile-arcs -ftest-coverage")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# Include "gtest" library
include(FetchContent)
FetchContent_Declare(
 googletest
 URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
 DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

add_executable(test_keys_manager keys_manager/test.cpp)
target_link_libraries(test_keys_manager extension_keys_manager GTest::gtest_main)

# Подавление предупреждений при компиляции и линковке
target_compile_options(test_keys_manager PRIVATE -Wno-free-nonheap-object)
target_link_options(test_keys_manager PRIVATE -Wno-free-nonheap-object)

gtest_discover_tests(test_keys_manager)
