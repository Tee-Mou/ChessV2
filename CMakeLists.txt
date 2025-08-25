cmake_minimum_required(VERSION 3.10)
project(myChess2 CXX)

# Make executable
set(SOURCE_FILES_EXE
    main.cpp
    src/BitboardTables.cpp
    src/BitOps.cpp
    src/MoveGen.cpp
)
add_executable(myChess2 ${SOURCE_FILES_EXE})

# # Make pybind module
# add_subdirectory(pybind11)
# set(SOURCE_FILES_MODULE
#     )
# pybind11_add_module(myChess2, ${SOURCE_FILES_MODULE})
