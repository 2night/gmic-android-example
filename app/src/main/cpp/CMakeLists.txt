# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# We deploy it as library, but actually it is an executable
add_definitions(-Dgmic_build -Dcimg_display=0)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
add_executable( gmic.so gmic.cpp gmic_cli.cpp)
