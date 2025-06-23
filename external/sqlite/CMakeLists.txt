# File: external/sqlite/CMakeLists.txt

# Definisce una libreria statica chiamata "sqlite3_lib"
# che viene costruita compilando il file "sqlite3.c"
add_library(sqlite3_lib STATIC sqlite3.c)

# Dice a qualsiasi programma che userà questa libreria
# di guardare in questa cartella per trovare i file header (sqlite3.h)
target_include_directories(sqlite3_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})