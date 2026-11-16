cmake_minimum_required(VERSION 3.24)
project(GlyphPop VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_program(ZIG_EXECUTABLE zig REQUIRED)
find_package(Qt6 6.10 REQUIRED COMPONENTS Quick Qml DBus WaylandClient)
# The input-method popup role and grabbed-key delivery need Qt's private Wayland and QPA API, on purpose.
set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 REQUIRED COMPONENTS GuiPrivate WaylandClientPrivate)
# 6.6 added setScreen and setWantsToBeOnActiveScreen, and it needs Qt 6.10 itself.
find_package(LayerShellQt 6.6 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(XKB REQUIRED IMPORTED_TARGET xkbcommon)
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

set(CORE_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/zig-out")
set(CORE_LIBRARY "${CORE_PREFIX}/lib/libglyphpop_core.a")
file(GLOB CORE_SOURCES CONFIGURE_DEPENDS src/*.zig src/core.h data/unicode/*)
add_custom_command(
    OUTPUT "${CORE_LIBRARY}"
    COMMAND "${CMAKE_COMMAND}" -E env "ZIG_GLOBAL_CACHE_DIR=${CMAKE_CURRENT_BINARY_DIR}/zig-cache"
        "${ZIG_EXECUTABLE}" build -Doptimize=ReleaseFast --prefix "${CORE_PREFIX}"
        --cache-dir "${CMAKE_CURRENT_BINARY_DIR}/zig-local-cache"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS build.zig ${CORE_SOURCES}
    VERBATIM
)
add_custom_target(glyphpop_core_build DEPENDS "${CORE_LIBRARY}")
add_library(glyphpop_core STATIC IMPORTED GLOBAL)
set_target_properties(glyphpop_core PROPERTIES IMPORTED_LOCATION "${CORE_LIBRARY}")
add_dependencies(glyphpop_core glyphpop_core_build)

set(QML_SOURCES
    ui/App.qml
    ui/Theme.qml
    ui/PickerCard.qml
    ui/SearchField.qml
    ui/SectionSwitch.qml
    ui/GlyphGrid.qml
    ui/CategoryStrip.qml
    ui/ToneButton.qml
    ui/ToneMenu.qml
    ui/Segmented.qml
    ui/IconButton.qml
    ui/TuneIcon.qml
    ui/BackIcon.qml
    ui/ColorPicker.qml
    ui/OptionsPage.qml
)
foreach(source IN LISTS QML_SOURCES)
    get_filename_component(alias "${source}" NAME)
    set_source_files_properties("${source}" PROPERTIES QT_RESOURCE_ALIAS "${alias}")
endforeach()
set_source_files_properties(ui/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(glyphpop
    app/main.cpp
    app/picker.cpp
    app/picker.h
    app/glyphmodel.cpp
    app/glyphmodel.h
    app/inputmethod.cpp
    app/inputmethod.h
    app/popupshell.cpp
    app/popupshell.h
    app/virtualkeyboard.cpp
    app/virtualkeyboard.h
    app/themesource.cpp
    app/themesource.h
)
qt6_generate_wayland_protocol_client_sources(glyphpop PRIVATE_CODE FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/input-method-unstable-v2.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/virtual-keyboard-unstable-v1.xml
)
qt_add_qml_module(glyphpop
    URI GlyphPop
    VERSION 1.0
    QML_FILES ${QML_SOURCES}
)
add_dependencies(glyphpop glyphpop_core_build)
target_include_directories(glyphpop PRIVATE src app)
target_compile_definitions(glyphpop PRIVATE GLYPHPOP_VERSION="${PROJECT_VERSION}")
target_link_libraries(glyphpop PRIVATE
    glyphpop_core Qt6::Quick Qt6::Qml Qt6::DBus Qt6::GuiPrivate
    Qt6::WaylandClient Qt6::WaylandClientPrivate LayerShellQt::Interface PkgConfig::XKB)
install(TARGETS glyphpop RUNTIME DESTINATION bin)
install(FILES data/io.github.ItzELECTR0.GlyphPop.desktop DESTINATION share/applications)
