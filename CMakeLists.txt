cmake_minimum_required(VERSION 3.20)
project(EsquivarBotellas CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================
# Salida del ejecutable
# ============================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ejecutable/windows)

# Crear carpeta automáticamente
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
)

# ============================================
# Crear ejecutable
# ============================================
add_executable(${PROJECT_NAME} ${SOURCES})

# ============================================
# Dependencias SDL3
# ============================================
find_package(SDL3        REQUIRED)
find_package(SDL3_image  REQUIRED)
find_package(SDL3_ttf    REQUIRED)
find_package(SDL3_mixer  REQUIRED)

target_link_libraries(${PROJECT_NAME}
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
    SDL3_mixer::SDL3_mixer
)

# ============================================
# Windows: no abrir consola
# ============================================
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# ============================================
# Copiar TODAS las DLLs necesarias automáticamente
# Usa script externo para evitar conflictos de $ con Ninja
# ============================================
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND powershell -ExecutionPolicy Bypass -File
            "${CMAKE_SOURCE_DIR}/copy_dlls.ps1"
            -ExePath "$<TARGET_FILE:${PROJECT_NAME}>"
            -DestDir "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copiando DLLs a ejecutable/windows..."
    )
endif()
