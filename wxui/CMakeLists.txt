# NovaRemote CMake build. Used for macOS (and Linux). On Windows the normal
# build is NovaRemote.sln; this file also works there with the Reference wx copy.
#
#   macOS, self-contained .app (downloads and builds wxWidgets statically):
#     cmake -S wxui -B build -DCMAKE_BUILD_TYPE=Release -DNOVAREMOTE_FETCH_WX=ON
#     cmake --build build -j
#   macOS, against Homebrew wxWidgets (brew install wxwidgets):
#     cmake -S wxui -B build -DCMAKE_BUILD_TYPE=Release
#     cmake --build build -j
#
# AI: keep the source list in sync with wxui.vcxproj. Do not add wx components
# beyond core and base without a reason; the program only needs those two.

cmake_minimum_required(VERSION 3.16)
project(NovaRemote LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(NOVAREMOTE_WX_VERSION 3.2.12)
option(NOVAREMOTE_FETCH_WX
       "Download wxWidgets ${NOVAREMOTE_WX_VERSION} and build it statically into the app"
       OFF)

set(NOVAREMOTE_SOURCES
    src/app.cpp
    src/canvas.cpp
    src/meter.cpp
    src/mixpages.cpp
    src/pages.cpp
    src/pages_more.cpp
    src/painter.cpp
    src/protocol.cpp
    src/serial.cpp
    src/settings.cpp
    src/ui.cpp
    src/canvas.h
    src/geom.h
    src/meter.h
    src/pages.h
    src/painter.h
    src/protocol.h
    src/serial.h
    src/settings.h
    src/ui.h)

# ---------------------------------------------------------------- wxWidgets
if(NOVAREMOTE_FETCH_WX)
    include(FetchContent)
    # Static, core + base only. The extra libraries are switched off so the
    # wx build takes a few minutes instead of twenty.
    set(wxBUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(wxBUILD_MONOLITHIC OFF CACHE BOOL "" FORCE)
    set(wxBUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(wxBUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(wxBUILD_DEMOS OFF CACHE BOOL "" FORCE)
    set(wxBUILD_INSTALL OFF CACHE BOOL "" FORCE)
    # Precompiled headers cannot be built for two architectures at once
    # (universal binary), so leave them off.
    set(wxBUILD_PRECOMP OFF CACHE BOOL "" FORCE)
    foreach(opt wxUSE_STC wxUSE_WEBVIEW wxUSE_RICHTEXT wxUSE_AUI wxUSE_PROPGRID
                wxUSE_RIBBON wxUSE_XRC wxUSE_HTML wxUSE_MEDIACTRL wxUSE_OPENGL)
        set(${opt} OFF CACHE BOOL "" FORCE)
    endforeach()
    FetchContent_Declare(wxWidgets
        URL https://github.com/wxWidgets/wxWidgets/releases/download/v${NOVAREMOTE_WX_VERSION}/wxWidgets-${NOVAREMOTE_WX_VERSION}.tar.bz2
        URL_HASH SHA256=a62719bb5e1dcc41c1a6fc1eecd499c7ee8521f70402c7b1bb3342a1e16344e0)
    FetchContent_MakeAvailable(wxWidgets)
    set(NOVAREMOTE_WX_LIBS wx::core wx::base)
else()
    if(WIN32 AND NOT wxWidgets_ROOT_DIR)
        # The slim copy shipped in Reference: headers, mswu setup.h, import libs, 2 DLLs.
        set(wxWidgets_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Reference/WX_Release_${NOVAREMOTE_WX_VERSION}")
        set(wxWidgets_LIB_DIR "${wxWidgets_ROOT_DIR}/lib/vc14x_dll")
        set(wxWidgets_CONFIGURATION mswu)
    endif()
    find_package(wxWidgets 3.2 REQUIRED COMPONENTS core base)
    include(${wxWidgets_USE_FILE})
    set(NOVAREMOTE_WX_LIBS ${wxWidgets_LIBRARIES})
endif()

# ---------------------------------------------------------------- program
add_executable(NovaRemote WIN32 MACOSX_BUNDLE ${NOVAREMOTE_SOURCES})
target_include_directories(NovaRemote PRIVATE src)
target_link_libraries(NovaRemote PRIVATE ${NOVAREMOTE_WX_LIBS})

if(MSVC)
    target_compile_definitions(NovaRemote PRIVATE
        wxMSVC_VERSION_ABI_COMPAT _CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS
        WIN32_LEAN_AND_MEAN _UNICODE UNICODE)
    target_compile_options(NovaRemote PRIVATE /W3 /permissive-)
    if(NOT NOVAREMOTE_FETCH_WX)
        add_custom_command(TARGET NovaRemote POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${wxWidgets_LIB_DIR}/wxbase32u_vc14x.dll"
                "${wxWidgets_LIB_DIR}/wxmsw32u_core_vc14x.dll"
                "$<TARGET_FILE_DIR:NovaRemote>")
    endif()
else()
    target_compile_options(NovaRemote PRIVATE -Wall -Wno-unused-parameter)
endif()

if(APPLE)
    set_target_properties(NovaRemote PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/mac/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME "NovaRemote"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.zaxcom.novaremote"
        MACOSX_BUNDLE_BUNDLE_VERSION "0.92.10"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "0.92.10")
endif()
