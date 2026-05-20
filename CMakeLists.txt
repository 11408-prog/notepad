cmake_minimum_required(VERSION 3.21)

project(NotePad
    VERSION 0.8.6
    DESCRIPTION "Qt based multi-purpose note and developer tool"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    PrintSupport
    Charts
)

set(NOTEPAD_SOURCES
    main.cpp
    appsettings.cpp
    compilerrunner.cpp
    codeeditor.cpp
    databasemanager.cpp
    filebrowserwidget.cpp
    datamodel.cpp
    chartwidget.cpp
    dataanalysiswidget.cpp
    pythonscriptrunner.cpp
    imagedisplaywidget.cpp
    imageviewerwidget.cpp
    imageenhancer.cpp
    settingswidget.cpp
    terminalwidget.cpp
    mainwindow.cpp
)

set(NOTEPAD_HEADERS
    codeeditor.h
    appsettings.h
    compilerrunner.h
    databasemanager.h
    filebrowserwidget.h
    datamodel.h
    chartwidget.h
    dataanalysiswidget.h
    pythonscriptrunner.h
    imagedisplaywidget.h
    imageviewerwidget.h
    imageenhancer.h
    settingswidget.h
    terminalwidget.h
    mainwindow.h
    note.h
)

add_executable(NotePad
    ${NOTEPAD_SOURCES}
    ${NOTEPAD_HEADERS}
    mainwindow.ui
)

target_include_directories(NotePad PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(NotePad PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::PrintSupport
    Qt6::Charts
)

if(WIN32)
    target_link_libraries(NotePad PRIVATE user32 shell32 advapi32)
endif()

include(cmake/ThirdParty.cmake)
notepad_configure_third_party(NotePad)
