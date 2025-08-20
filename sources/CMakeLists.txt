cmake_minimum_required(VERSION 3.24)

project(AndroidDrive VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Set Qt packages
find_package(QT NAMES Qt6)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS LinguistTools)

# Set source files
set(PROJECT_SOURCES
    androiddevice.cpp
    androiddevice.hpp
    androiddrive.cpp
    androiddrive.hpp
    debuglogger.cpp
    debuglogger.hpp
    devicelistmodel.cpp
    devicelistmodel.hpp
    devicelistwindow.cpp
    devicelistwindow.hpp
    dokanoperations.cpp
    dokanoperations.hpp
    helperfunctions.cpp
    helperfunctions.hpp
    main.cpp
    settings.cpp
    settings.hpp
    settingswindow.cpp
    settingswindow.hpp
    temporaryfile.cpp
    temporaryfile.hpp
    updates.cpp
    updates.hpp
    version.h
)
qt_add_executable(AndroidDrive
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    resource.qrc
    resource.rc
)

# Add include/link directories
target_include_directories(AndroidDrive PRIVATE "C:/Program Files/Dokan/Dokan Library-2.0.6/include")
target_link_libraries(AndroidDrive PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(AndroidDrive PRIVATE Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(AndroidDrive PRIVATE "C:/Program Files/Dokan/Dokan Library-2.0.6/lib/dokan2.lib")

# Compile executable
set_target_properties(AndroidDrive PROPERTIES
    ${BUNDLE_ID_OPTION}
    WIN32_EXECUTABLE TRUE
)
include(GNUInstallDirs)
install(TARGETS AndroidDrive
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Automatically run windeployqt when compiling
add_custom_command(TARGET AndroidDrive
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/windeployqt"
    COMMAND Qt6::windeployqt --dir "${CMAKE_CURRENT_BINARY_DIR}/windeployqt" "$<TARGET_FILE_DIR:AndroidDrive>/$<TARGET_FILE_NAME:AndroidDrive>"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/windeployqt/translations"
    COMMAND ${CMAKE_COMMAND} -E remove "${CMAKE_CURRENT_BINARY_DIR}/windeployqt/vc_redist.x64.exe"
)

# Download adb so that it can be used by the inno setup script
FetchContent_Declare(
    adb
    GIT_REPOSITORY https://github.com/openatx/adb-binaries
    GIT_TAG "master"
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
    SYSTEM
)
FetchContent_MakeAvailable(adb)

# Define lupdate files
qt_add_lupdate(TS_FILES
    translations/androiddrive_de.ts
    translations/androiddrive_empty.ts
    translations/androiddrive_fr.ts
    translations/androiddrive_hu.ts
    translations/androiddrive_it.ts
    translations/androiddrive_ja.ts
    translations/androiddrive_pt_BR.ts
    translations/androiddrive_sv.ts
    SOURCE_TARGETS AndroidDrive
)

# Automatically run lupdate when compiling
add_dependencies(AndroidDrive ${PROJECT_NAME}_lupdate)

qt_finalize_executable(AndroidDrive)
