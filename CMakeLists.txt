cmake_minimum_required(VERSION 3.16)

project(PBDClothSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)
find_package(OpenGL REQUIRED)

set(APP_SOURCES
    src/main.cpp
)

set(ENGINE_SOURCES
    src/engine/Engine.h
    src/engine/Engine.cpp
    src/engine/lifecycle/Lifecycle.h
    src/engine/physics/PhysicsSystem.h
    src/engine/render/RenderSystem.h
    src/engine/simulation/SimulationSystem.h
)

set(CLOTH_SOURCES
    src/cloth/ClothWorld.h
    src/cloth/ClothWorld.cpp
    src/cloth/core/Particle.h
    src/cloth/core/Cloth.h
    src/cloth/core/Cloth.cpp
    src/cloth/physics/ClothPhysicsSystem.h
    src/cloth/physics/ClothPhysicsSystem.cpp
    src/cloth/physics/ClothSimulator.h
    src/cloth/physics/ClothSimulator.cpp
    src/cloth/render/ClothRenderData.h
    src/cloth/render/ClothRenderer.h
    src/cloth/render/ClothRenderer.cpp
    src/cloth/render/ClothRenderSystem.h
    src/cloth/render/ClothRenderSystem.cpp
    src/cloth/simulation/ClothSimulationSystem.h
    src/cloth/simulation/ClothSimulationSystem.cpp
)

set(PLATFORM_QT_SOURCES
    src/platform/qt/MainWindow.h
    src/platform/qt/MainWindow.cpp
    src/platform/qt/QtRenderRequestSystem.h
    src/platform/qt/QtRenderRequestSystem.cpp
    src/platform/qt/ViewportWidget.h
    src/platform/qt/ViewportWidget.cpp
)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
    ${APP_SOURCES}
    ${ENGINE_SOURCES}
    ${CLOTH_SOURCES}
    ${PLATFORM_QT_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
    OpenGL::GL
)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /permissive-
        /Zc:__cplusplus
    )
endif()

if (WIN32)
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")

    if (WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                "$<IF:$<CONFIG:Debug>,--debug,--release>"
                --compiler-runtime
                --no-translations
                "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Deploying Qt runtime dependencies"
        )
    else()
        message(WARNING "windeployqt was not found. The executable may need Qt DLLs next to it.")
    endif()
endif()
