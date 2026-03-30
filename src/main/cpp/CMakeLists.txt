cmake_minimum_required(VERSION 3.22.1)

project("pdfium_jni")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure PDFium library imports
add_library(pdfium SHARED IMPORTED)
set_target_properties(pdfium PROPERTIES IMPORTED_LOCATION
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../pdfium-android-arm64/lib/libpdfium.so"
)

# Add PDFium include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../pdfium-android-arm64/include
)

# JNI wrapper library
add_library(pdfium_jni SHARED
    pdfium_jni.cpp
)

# Link with Android libraries and PDFium
find_library(log-lib log)
find_library(android-lib android)
find_library(jnigraphics-lib jnigraphics)

target_link_libraries(pdfium_jni
    pdfium
    ${log-lib}
    ${android-lib}
    ${jnigraphics-lib}
)
