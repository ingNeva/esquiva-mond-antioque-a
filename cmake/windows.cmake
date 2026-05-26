cmake_minimum_required(VERSION 3.20)
project(EsquivarBotellas CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================
# Salida del ejecutable
# ============================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ejecutable/linux)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/ejecutable/linux)

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
# Dependencias SDL3 via pkg-config
# ============================================
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3       REQUIRED IMPORTED_TARGET sdl3)
pkg_check_modules(SDL3_IMAGE REQUIRED IMPORTED_TARGET sdl3-image)
pkg_check_modules(SDL3_TTF   REQUIRED IMPORTED_TARGET sdl3-ttf)
pkg_check_modules(SDL3_MIXER REQUIRED IMPORTED_TARGET sdl3-mixer)

target_link_libraries(${PROJECT_NAME}
    PkgConfig::SDL3
    PkgConfig::SDL3_IMAGE
    PkgConfig::SDL3_TTF
    PkgConfig::SDL3_MIXER
)
