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
    src/engine/core/Component.h
    src/engine/core/Object.h
    src/engine/core/Object.cpp
    src/engine/core/Transform.h
    src/engine/core/World.h
    src/engine/core/World.cpp
    src/engine/lifecycle/Lifecycle.h
    src/engine/render/RenderComponent.h
    src/engine/render/Renderer.h
    src/engine/render/Renderer.cpp
    src/engine/render/RenderScene.h
    src/engine/render/RenderSystem.h
    src/engine/render/RenderSystem.cpp
    src/engine/simulation/SimulationComponent.h
)

set(CLOTH_SOURCES
    src/cloth/ClothWorld.h
    src/cloth/ClothWorld.cpp
    src/cloth/ClothObject.h
    src/cloth/ClothObject.cpp
    src/cloth/components/ClothRenderComponent.h
    src/cloth/components/ClothRenderComponent.cpp
    src/cloth/components/ClothSimulationComponent.h
    src/cloth/components/ClothSimulationComponent.cpp
    src/cloth/components/PBDClothSimulationComponent.h
    src/cloth/components/PBDClothSimulationComponent.cpp
    src/cloth/components/XPBDClothSimulationComponent.h
    src/cloth/components/XPBDClothSimulationComponent.cpp
    src/cloth/core/Particle.h
    src/cloth/core/Cloth.h
    src/cloth/core/Cloth.cpp
)

set(PLATFORM_QT_SOURCES
    src/platform/qt/MainWindow.h
    src/platform/qt/MainWindow.cpp
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
