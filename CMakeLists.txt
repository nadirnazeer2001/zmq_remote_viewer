cmake_minimum_required(VERSION 3.5)

project(zmq_remote_viewer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)  # Changed from 11 to 14
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt packages
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

# Improved ZeroMQ finding
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)
find_path(CPPZMQ_INCLUDE_DIR zmq.hpp
    PATHS /usr/include /usr/local/include ${ZMQ_INCLUDE_DIRS}
    PATH_SUFFIXES cppzmq
)

if(CPPZMQ_INCLUDE_DIR)
    include_directories(${CPPZMQ_INCLUDE_DIR} ${ZMQ_INCLUDE_DIRS})
    message(STATUS "Found cppzmq headers at: ${CPPZMQ_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "cppzmq headers not found! Install libcppzmq-dev or similar package")
endif()

# Improved OpenCV finding
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
if(OpenCV_FOUND)
    include_directories(${OpenCV_INCLUDE_DIRS})
    message(STATUS "Found OpenCV: ${OpenCV_VERSION}")
else()
    message(FATAL_ERROR "OpenCV not found! Install libopencv-dev")
endif()

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(zmq_remote_viewer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(zmq_remote_viewer SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(zmq_remote_viewer
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(zmq_remote_viewer PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    ${OpenCV_LIBS}
    ${ZMQ_LIBRARIES}
)

set_target_properties(zmq_remote_viewer PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(zmq_remote_viewer)
endif()
