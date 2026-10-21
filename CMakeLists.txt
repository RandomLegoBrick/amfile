cmake_minimum_required(VERSION 4.4)
project(amfile)

set(CMAKE_CXX_STANDARD 20)
set(PDFIUM_ROOT "${CMAKE_SOURCE_DIR}/pdfium")

find_package(wxWidgets REQUIRED COMPONENTS core base)
find_package(SQLite3 REQUIRED)

include(${wxWidgets_USE_FILE})

add_library(pdfium SHARED IMPORTED GLOBAL)

set_target_properties(pdfium PROPERTIES
    IMPORTED_LOCATION
        "${PDFIUM_ROOT}/lib/libpdfium.so"
    INTERFACE_INCLUDE_DIRECTORIES
        "${PDFIUM_ROOT}/include"
)


add_executable(amfile main.cpp)

target_link_libraries(amfile PRIVATE ${wxWidgets_LIBRARIES} SQLite3::SQLite3)

target_link_libraries(amfile PRIVATE pdfium)

set_target_properties(amfile PROPERTIES
    BUILD_RPATH "${PDFIUM_ROOT}/lib"
)