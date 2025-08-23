CMAKE_MINIMUM_REQUIRED_VERSION(3.10)
project(chessv2)

# Make executable
set(SOURCE_FILES_EXE
    main.cpp
)
add_executable(chessv2, ${SOURCE_FILES_EXE})

# # Make pybind module
# add_subdirectory(pybind11)
# set(SOURCE_FILES_MODULE
#     )
# pybind11_add_module(chessv2, ${SOURCE_FILES_MODULE})
