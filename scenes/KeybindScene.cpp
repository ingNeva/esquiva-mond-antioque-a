#include "KeybindScene.h"
#include "../core/Game.h"

// ============================================
// Nombres amigables de acciones
// ============================================
static const char* NOMBRES_ACCIONES[] = {
    "Mover arriba",
    "Mover abajo",
    "Mover izquierda",
    "Mover derecha",
    "Atacar (machete)",
    "Pausar",
};
static const int TOTAL_TECLAS = 6;

// ============================================
// Retorna puntero a la scancode de cada acción
// ============================================
static SDL_Scancode* obtenerTecla(KeyConfig& kc, int idx) {
    switch (idx) {
        case 0: return &kc.moverArriba;
        case 1: return &kc.moverAbajo;
        case 2: return &kc.moverIzquierda;
        case 3: return &kc.moverDerecha;
        case 4: return &kc.atacar;
        case 5: return &kc.pausa;
        default: return nullptr;
    }
}

// ============================================
// Comprueba si una scancode ya está en uso
// Devuelve el índice de la acción que la usa, o -1
// ============================================
static int scancodeEnUso(const KeyConfig& kc, SDL_Scancode sc, int ignorarIdx) {
    const SDL_Scancode scancodes[] = {
        kc.moverArriba, kc.moverAbajo, kc.moverIzquierda,
        kc.moverDerecha, kc.atacar, kc.pausa
    };
    for (int i = 0; i < TOTAL_TECLAS; i++) {
        if (i == ignorarIdx) continue;
        if (scancodes[i] == sc) return i;
    }
    return -1;
}

// ============================================
// Estado local de la escena (persistente entre frames)
// ============================================
static int    s_opcionSeleccionada = 0;
static bool   s_esperandoTecla    = false;
static char   s_mensajeError[96]  = {};
static Uint64 s_timerError        = 0;

// ============================================
// Scancodes que no se permiten asignar
// ============================================
static bool scancodeProhibida(SDL_Scancode sc) {
    return sc == SDL_SCANCODE_UNKNOWN
        || sc == SDL_SCANCODE_LALT    || sc == SDL_SCANCODE_RALT
        || sc == SDL_SCANCODE_LGUI    || sc == SDL_SCANCODE_RGUI
        || sc == SDL_SCANCODE_LCTRL   || sc == SDL_SCANCODE_RCTRL
        || sc == SDL_SCANCODE_LSHIFT  || sc == SDL_SCANCODE_RSHIFT
        || sc == SDL_SCANCODE_PRINTSCREEN
        || sc == SDL_SCANCODE_SCROLLLOCK
        || sc == SDL_SCANCODE_NUMLOCKCLEAR;
}

// ============================================
// Render + eventos de la escena de teclas
// ============================================
void renderizarTeclas(Juego* juego) {
    SDL_RenderClear(juego->renderer);
    const int W = VW(juego), H = VH(juego);
    const int cx      = W / 2;
    const int labelX  = cx - (int)(W * 0.24f);
    const int valorX  = cx + (int)(W * 0.04f);
    const int inicioY = (int)(H * 0.16f);
    const int paso    = (int)(H * 0.09f);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color gris     = {130, 130, 130, 255};
    SDL_Color verde    = { 80, 255, 120, 255};
    SDL_Color rojo     = {255,  80,  80, 255};
    SDL_Color cyan     = { 80, 210, 255, 255};

    // Título
    renderizarTextoCentrado(juego, "TECLAS DE JUEGO", (int)(H * 0.05f), amarillo);
    renderizarTextoPequenoC(juego,
        s_esperandoTecla ? "Presiona la tecla nueva..." : "Enter/Seleccionar: reasignar    R: restablecer todo",
        (int)(H * 0.10f), s_esperandoTecla ? cyan : gris);

    // Filas de acciones
    for (int i = 0; i < TOTAL_TECLAS; i++) {
        bool seleccionada = (s_opcionSeleccionada == i);
        bool esperandoEsta = s_esperandoTecla && seleccionada;

        SDL_Color colorNombre = seleccionada ? amarillo : blanco;
        char linea[64];
        SDL_snprintf(linea, sizeof(linea), "%s%s",
            seleccionada ? "> " : "  ", NOMBRES_ACCIONES[i]);
        renderizarTexto(juego, linea, labelX, inicioY + i * paso, colorNombre);

        if (esperandoEsta) {
            renderizarTexto(juego, "[ ??? ]", valorX, inicioY + i * paso, cyan);
        } else {
            SDL_Scancode* sc = obtenerTecla(juego->keyConfig, i);
            const char* nombreTecla = SDL_GetScancodeName(*sc);
            char valor[64];
            SDL_snprintf(valor, sizeof(valor), "[ %s ]",
                (nombreTecla && *nombreTecla) ? nombreTecla : "???");
            SDL_Color colorValor = seleccionada ? verde : blanco;
            renderizarTexto(juego, valor, valorX, inicioY + i * paso, colorValor);
        }
    }

    // Mensaje de error/conflicto (desaparece tras 2 segundos)
    if (s_mensajeError[0] && SDL_GetTicks() < s_timerError) {
        renderizarTextoPequenoC(juego, s_mensajeError,
            inicioY + TOTAL_TECLAS * paso + (int)(H * 0.01f), rojo);
    } else {
        s_mensajeError[0] = '\0';
    }

    // Separador + instrucciones
    int sepY = inicioY + TOTAL_TECLAS * paso + (int)(H * 0.055f);
    SDL_SetRenderDrawColor(juego->renderer, 60, 60, 60, 255);
    SDL_RenderLine(juego->renderer,
        (float)(cx - (int)(W * 0.24f)), (float)sepY,
        (float)(cx + (int)(W * 0.24f)), (float)sepY);
    renderizarTextoPequenoC(juego,
        "Arriba/Abajo: navegar    ESC: volver a opciones",
        sepY + (int)(H * 0.025f), gris);

    SDL_RenderPresent(juego->renderer);

    // ── Eventos ──────────────────────────────────────────────
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            juego->ejecutando = false;
            s_esperandoTecla = false;
            return;
        }

        // Modo: esperando pulsación para reasignar
        if (s_esperandoTecla) {
            if (e.type == SDL_EVENT_KEY_DOWN) {
                SDL_Scancode nueva = e.key.scancode;

                // Cancelar con Escape
                if (nueva == SDL_SCANCODE_ESCAPE) {
                    s_esperandoTecla = false;
                    return;
                }

                // Rechazar teclas prohibidas
                if (scancodeProhibida(nueva)) {
                    SDL_snprintf(s_mensajeError, sizeof(s_mensajeError),
                        "Esa tecla no se puede asignar.");
                    s_timerError = SDL_GetTicks() + 2000;
                    s_esperandoTecla = false;
                    return;
                }

                // Detectar conflicto
                int conflicto = scancodeEnUso(juego->keyConfig, nueva, s_opcionSeleccionada);
                if (conflicto >= 0) {
                    // Intercambio: la acción en conflicto toma la tecla vieja
                    SDL_Scancode* scVieja = obtenerTecla(juego->keyConfig, s_opcionSeleccionada);
                    SDL_Scancode* scConfl = obtenerTecla(juego->keyConfig, conflicto);
                    SDL_Scancode temp = *scVieja;
                    *scConfl = temp;
                    *scVieja = nueva;
                    SDL_snprintf(s_mensajeError, sizeof(s_mensajeError),
                        "Intercambiada con: %s", NOMBRES_ACCIONES[conflicto]);
                    s_timerError = SDL_GetTicks() + 2500;
                } else {
                    SDL_Scancode* sc = obtenerTecla(juego->keyConfig, s_opcionSeleccionada);
                    *sc = nueva;
                }

                guardarConfig(juego);
                s_esperandoTecla = false;
            }
            return; // no procesar más eventos mientras espera
        }

        // Modo normal: navegar y seleccionar
        if (e.type == SDL_EVENT_KEY_DOWN) {
            switch (e.key.key) {
                case SDLK_UP:
                    s_opcionSeleccionada =
                        (s_opcionSeleccionada - 1 + TOTAL_TECLAS) % TOTAL_TECLAS;
                    break;
                case SDLK_DOWN:
                    s_opcionSeleccionada =
                        (s_opcionSeleccionada + 1) % TOTAL_TECLAS;
                    break;
                case SDLK_RETURN: case SDLK_KP_ENTER:
                    s_esperandoTecla = true;
                    s_mensajeError[0] = '\0';
                    break;
                case SDLK_R:
                    // Restablecer todo a por defecto
                    juego->keyConfig = KeyConfig{};
                    guardarConfig(juego);
                    SDL_snprintf(s_mensajeError, sizeof(s_mensajeError),
                        "Teclas restablecidas a los valores por defecto.");
                    s_timerError = SDL_GetTicks() + 2500;
                    break;
                case SDLK_ESCAPE: case SDLK_BACKSPACE:
                    s_esperandoTecla = false;
                    juego->estado = ESTADO_OPCIONES;
                    break;
                default: break;
            }
        }
        // Gamepad: navegar con D-Pad, confirmar con Sur, salir con Este
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            switch (e.gbutton.button) {
                case SDL_GAMEPAD_BUTTON_DPAD_UP:
                    s_opcionSeleccionada =
                        (s_opcionSeleccionada - 1 + TOTAL_TECLAS) % TOTAL_TECLAS;
                    break;
                case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
                    s_opcionSeleccionada =
                        (s_opcionSeleccionada + 1) % TOTAL_TECLAS;
                    break;
                case SDL_GAMEPAD_BUTTON_SOUTH:
                    s_esperandoTecla = true;
                    s_mensajeError[0] = '\0';
                    break;
                case SDL_GAMEPAD_BUTTON_EAST:
                    s_esperandoTecla = false;
                    juego->estado = ESTADO_OPCIONES;
                    break;
                default: break;
            }
        }
        if (e.type == SDL_EVENT_WINDOW_RESIZED) recargarFuentes(juego);
    }
}
