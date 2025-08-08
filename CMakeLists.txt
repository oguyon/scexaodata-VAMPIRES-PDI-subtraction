# 1. Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# 2. Define the project name, version, and language
project(DynamicCsvReader VERSION 1.0 LANGUAGES C)

# 3. Add the executable target
# This tells CMake to create an executable named "csv_reader"
# from the source file "csv_reader.c".
add_executable(csv_reader src/csv_reader.c)

# 4. Set the C standard (optional but recommended)
# This ensures your compiler uses the C11 standard, which is good practice.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 5. Add compiler warnings (optional but highly recommended for development)
# This helps you write better, safer code by catching potential issues.
target_compile_options(csv_reader PRIVATE -Wall -Wextra -pedantic)
