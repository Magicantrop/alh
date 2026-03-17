cmake_minimum_required(VERSION 3.10)
project(AtlantisLittleHelper VERSION 1.0.0 LANGUAGES CXX)

# Basic settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_LINUX TRUE)
endif()

# Find wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base adv html xml net)
if(wxWidgets_FOUND)
    include(${wxWidgets_USE_FILE})
    message(STATUS "Found wxWidgets: ${wxWidgets_INCLUDE_DIRS}")
endif()

# Find Python
find_package(Python REQUIRED COMPONENTS Interpreter Development)
if(Python_FOUND)
    message(STATUS "Found Python: ${Python_EXECUTABLE}")
endif()

# Source files list
set(SOURCES
    ahapp_config.cpp
    ahapp_data.cpp
    ahapp_flags.cpp
    ahapp_frames.cpp
    ahapp_hexes.cpp
    ahapp_main.cpp
    ahapp_misc.cpp
    ahapp_orders.cpp
    ahapp_reports.cpp
    ahapp_utils.cpp
    ahapp_views.cpp
    ahframe.cpp
    atlaparser_core.cpp
    atlaparser_events.cpp
    atlaparser_orders.cpp
    cfgfile.cpp
    collection.cpp
    consts_ah.cpp
    cstr.cpp
    data.cpp
    data_unit.cpp
    editpane.cpp
    editsframe.cpp
    errs.cpp
    extend.cpp
    extend_no.cpp
    files.cpp
    flagsdlg.cpp
    hash.cpp
    hexfilterdlg.cpp
    listcoledit.cpp
    listpane.cpp
    mapframe.cpp
    mappane.cpp
    msgframe.cpp
    objs.cpp
    optionsdlg.cpp
    py_extend.cpp
    routeplanner.cpp
    shaftsframe.cpp
    takedlg.cpp
    template.cpp
    templateselectdlg.cpp
    transportdlg.cpp
    unitfilterdlg.cpp
    unitframe.cpp
    unitframefltr.cpp
    unitpane.cpp
    unitpanefltr.cpp
    unitsplitdlg.cpp
    utildlgs.cpp
)

# Header files list
set(HEADERS
    ahapp.h
    ahframe.h
    atlaparser.h
    bool.h
    cfgfile.h
    collection.h
    compat.h
    consts.h
    consts_ah.h
    cstr.h
    data.h
    editpane.h
    editsframe.h
    errs.h
    extend.h
    files.h
    flagsdlg.h
    hash.h
    hexfilterdlg.h
    listcoledit.h
    listpane.h
    mapframe.h
    mappane.h
    msgframe.h
    objs.h
    optionsdlg.h
    regexparser.h
    routeplanner.h
    shaftsframe.h
    stdafx.h
    stdhdr.h
    takedlg.h
    template.h
    templateselectdlg.h
    transportdlg.h
    unitfilterdlg.h
    unitframe.h
    unitframefltr.h
    unitpane.h
    unitpanefltr.h
    unitsplitdlg.h
    utildlgs.h
    version.h
)

# Add resource file for Windows
if(PLATFORM_WINDOWS)
    set(SOURCES ${SOURCES} alh.rc)
endif()

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Set target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "alh"
    PREFIX ""
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${wxWidgets_LIBRARIES}
    Python::Python
)

# Add include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${wxWidgets_INCLUDE_DIRS}
    ${Python_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Platform-specific settings
if(PLATFORM_WINDOWS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        WIN32
        _WINDOWS
        _UNICODE
        UNICODE
        __WXMSW__
    )
    
    target_link_libraries(${PROJECT_NAME} PRIVATE
        comctl32
        rpcrt4
        oleaut32
        ole32
        winspool
        shlwapi
        version
    )
elseif(PLATFORM_MACOS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        __WXMAC__
    )
    
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
    )
    
    # Create .app bundle
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE YES
        MACOSX_BUNDLE_BUNDLE_NAME "Atlantis Little Helper"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
    )
elseif(PLATFORM_LINUX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        __WXGTK__
    )
    
    find_package(X11 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${X11_LIBRARIES}
    )
endif()

# Compiler options
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W3>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

# Installation
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION bin
)

# Install configuration files
install(FILES
    ah.cfg
    ah.regexp.cfg
    ah_ru.cfg
    DESTINATION share/${PROJECT_NAME}
)

# Optional documentation build
option(BUILD_DOCS "Build documentation" OFF)
if(BUILD_DOCS AND DOXYGEN_FOUND)
    find_package(Doxygen REQUIRED)
    doxygen_add_docs(docs ${SOURCES} ${HEADERS})
endif()

# Testing
enable_testing()
add_test(NAME help COMMAND ${PROJECT_NAME} --help)