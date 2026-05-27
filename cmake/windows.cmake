cmake_minimum_required(VERSION 3.20)
project(EsquivarBotellas CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================
# Salida del ejecutable
# ============================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ejecutable/windows)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/ejecutable/windows)

# ============================================
# Fuentes
# ============================================
set(SOURCES
    main.cpp
    utils/ScoreManager.cpp
    core/AudioManager.cpp
    core/Game.cpp
    core/InputManager.cpp
    core/World.cpp
    entities/Player.cpp
    entities/Machete.cpp
    entities/Enemy.cpp
    entities/Boss.cpp
    entities/Llave.cpp
    scenes/GameScene.cpp
    scenes/MenuScene.cpp
    scenes/CountdownScene.cpp
    scenes/GameOverScene.cpp
    scenes/OptionsScene.cpp
    scenes/LevelSelectScene.cpp
)

# ============================================
# Crear ejecutable
# ============================================
add_executable(${PROJECT_NAME} ${SOURCES})

# ============================================
# Dependencias SDL3 — MSYS2 UCRT64
# ============================================
set(SDL3_ROOT "C:/msys64/ucrt64")

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SDL3_ROOT}/include
    ${SDL3_ROOT}/include/SDL3
)

target_link_directories(${PROJECT_NAME} PRIVATE
    ${SDL3_ROOT}/lib
)

target_link_libraries(${PROJECT_NAME}
    SDL3
    SDL3_image
    SDL3_ttf
    SDL3_mixer
)

# ============================================
# Sin consola negra en Release
# ============================================
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()