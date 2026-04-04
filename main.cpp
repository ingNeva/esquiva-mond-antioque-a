#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
#else
    #include <unistd.h>
#endif

// Modulos del juego
#include "utils/Types.h"
#include "core/Game.h"
#include "core/AudioManager.h"
#include "core/InputManager.h"
#include "entities/Player.h"
#include "entities/Enemy.h"
#include "entities/Machete.h"
#include "entities/Boss.h"
#include "core/World.h"
#include "utils/ScoreManager.h"
#include "scenes/GameScene.h"
#include "scenes/MenuScene.h"
#include "scenes/CountdownScene.h"
#include "scenes/GameOverScene.h"
#include "scenes/OptionsScene.h"

// ============================================
// FUNCION PRINCIPAL
// ============================================
int main() {
    FILE* log = fopen("log.txt", "w");
    if (log) { fprintf(log, "Iniciando...\n"); fclose(log); }

    const char* basePath = SDL_GetBasePath();
    if (basePath) {
#ifdef _WIN32
        SetCurrentDirectoryA(basePath);
#else
        chdir(basePath);
#endif
        log = fopen("log.txt", "a");
        if (log) { fprintf(log, "BasePath: %s\n", basePath); fclose(log); }
    }

    srand((unsigned int)time(NULL));

    Juego juego = {};
    juego.estado                     = ESTADO_MENU;
    juego.opcionMenuSeleccionada     = 0;
    juego.posicionNuevoPuntaje       = -1;
    juego.nivel4Reproducido          = false;
    juego.gameOverReproducido        = false;
    juego.transicion                 = {};
    juego.estadoBoss                 = BOSS_INACTIVO;
    juego.bossHP                     = 0;
    juego.bossUltimoDisparo          = 0;
    juego.trofeoActivo               = false;
    juego.bossSpawneado              = false;
    juego.combo                      = 0;
    juego.mejorCombo                 = 0;
    juego.multiplicador              = 1.0f;
    juego.inicioCuentaRegresiva      = 0;
    juego.pilaresActivos             = 0;
    juego.opcionOpcionesSeleccionada = 0;
    // resolucionSeleccionada, pantallaCompleta, musicaActiva, volumenMusica
    // se cargan desde saves/config.bin (o valores por defecto si es primera ejecucion)

    crearDirectorioSaves();
    cargarPuntajes(&juego.tablaPuntajes);
    cargarConfig(&juego);   // carga resolucion, fullscreen, audio (primera vez: fullscreen por defecto)

    if (!inicializarSDL(&juego)) {
        log = fopen("log.txt", "a");
        if (log) { fprintf(log, "FALLO: inicializarSDL\n"); fclose(log); }
        return 1;
    }
    if (!cargarTexturas(&juego)) {
        log = fopen("log.txt", "a");
        if (log) { fprintf(log, "FALLO: cargarTexturas\n"); fclose(log); }
        limpiarRecursos(&juego); return 1;
    }
    if (!cargarFuente(&juego)) {
        log = fopen("log.txt", "a");
        if (log) { fprintf(log, "FALLO: cargarFuente\n"); fclose(log); }
        limpiarRecursos(&juego); return 1;
    }

    log = fopen("log.txt", "a");
    if (log) { fprintf(log, "Todo OK, entrando al game loop\n"); fclose(log); }

    inicializarJugador(&juego.jugador);
    inicializarEnemigos(&juego);
    inicializarMachete(&juego);
    juego.macheteAparecido      = false;
    juego.ultimoNivelDificultad = 0;
    juego.ejecutando            = true;

    // ============================================
    // GAME LOOP PRINCIPAL
    // ============================================
    while (juego.ejecutando) {
        if (!juego.transicion.activa)
            reproducirMusica(&juego, pistaSegunEstadoJuego(&juego));

        switch (juego.estado) {
            case ESTADO_MENU:
                manejarEventosMenu(&juego);
                if (juego.ejecutando) renderizarMenu(&juego);
                break;

            case ESTADO_INSTRUCCIONES:
                renderizarInstrucciones(&juego);
                break;

            case ESTADO_JUGANDO:
                manejarEventos(&juego);
                actualizarJugador(&juego.jugador, juego.gamepad);
                actualizarAnimacionAtaque(&juego);
                actualizarPosicionMacheteEquipado(&juego);
                mundoActualizar(&juego);
                if (!juego.ejecutando) {
                    juego.ejecutando = true;
                    if (calificaParaTop5(&juego.tablaPuntajes, juego.puntuacion)) {
                        iniciarIngresoNombre(&juego);
                        juego.estado = ESTADO_INGRESANDO_NOMBRE;
                    } else {
                        juego.estado = ESTADO_GAME_OVER;
                    }
                }
                renderizar(&juego);
                break;

            case ESTADO_INGRESANDO_NOMBRE:
                renderizarIngresoNombre(&juego);
                break;

            case ESTADO_PAUSADO:
                renderizarPausa(&juego);
                break;

            case ESTADO_GAME_OVER:
                renderizarGameOver(&juego);
                break;

            case ESTADO_TRANSICION_NIVEL:
                actualizarTransicionNivel(&juego);
                renderizarTransicionNivel(&juego);
                break;

            case ESTADO_CUENTA_REGRESIVA:
                renderizarCuentaRegresiva(&juego);
                break;

            case ESTADO_VICTORIA:
                renderizarVictoria(&juego);
                break;

            case ESTADO_OPCIONES:
                renderizarOpciones(&juego);
                break;
        }
        SDL_Delay(16);
    }

    limpiarRecursos(&juego);
    return 0;
}
