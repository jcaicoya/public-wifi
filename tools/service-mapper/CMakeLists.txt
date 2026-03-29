cmake_minimum_required(VERSION 3.28)
project(service_mapper)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_AUTOMOC ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.7.3/msvc2022_64")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(service_mapper
    main.cpp
    ServiceMapper.h
    ServiceMapper.cpp
)

target_link_libraries(service_mapper PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets
)

# Copy Qt DLLs on Windows
if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    foreach(QT_LIB Core Gui Widgets)
        add_custom_command(TARGET service_mapper POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
            "$<TARGET_FILE_DIR:service_mapper>")
    endforeach()
endif()
