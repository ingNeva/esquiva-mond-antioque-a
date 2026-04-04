#include "LevelSelectScene.h"
#include "../core/Game.h"
#include "../utils/SaveManager.h"
#include "../entities/Player.h"
#include "../entities/Enemy.h"
#include "../entities/Machete.h"
#include "../scenes/CountdownScene.h"
#include <cmath>

// ============================================
// Nombres y descripciones de cada nivel
// Ajusta según los niveles reales del juego
// ============================================
static const char* NOMBRES_NIVEL[MAX_NIVELES] = {
    "Nivel 1  -  El Barrio",
    "Nivel 2  -  La Cantina",
    "Nivel 3  -  El Monte",
    "Nivel 4  -  El Jefe Final",
};

static const char* DESC_NIVEL[MAX_NIVELES] = {
    "Enemigos basicos. Aprende a esquivar.",
    "Enemigos rapidos y zigzag.",
    "Tanques y bombarderos. Usa el machete.",
    "El jefe te espera. Buena suerte.",
};

// ============================================
// Renderizar
// ============================================
void renderizarSeleccionNivel(Juego* juego) {
    SDL_RenderClear(juego->renderer);
    const int W = VW(juego), H = VH(juego);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color gris     = {100, 100, 100, 255};
    SDL_Color verde    = { 80, 255, 120, 255};
    SDL_Color rojo     = {200,  60,  60, 255};

    // ---- Titulo ----
    renderizarTextoCentrado(juego, "SELECCIONAR NIVEL", (int)(H * 0.08f), amarillo);

    // Linea decorativa bajo titulo
    SDL_SetRenderDrawColor(juego->renderer, 255, 220, 0, 120);
    SDL_RenderLine(juego->renderer,
        (float)(W * 0.10f), (float)(H * 0.16f),
        (float)(W * 0.90f), (float)(H * 0.16f));

    // ---- Tarjetas de nivel ----
    const int totalNiveles = MAX_NIVELES;
    const int cardW  = (int)(W * 0.70f);
    const int cardH  = (int)(H * 0.11f);
    const int cardX  = W / 2 - cardW / 2;
    const int inicioY = (int)(H * 0.20f);
    const int gapY    = (int)(H * 0.135f);
    const int sel     = juego->opcionLevelSelectSeleccionada;

    for (int i = 0; i < totalNiveles; i++) {
        bool desbloqueado = juego->nivelesDesbloqueados[i];
        bool seleccionado = (sel == i);
        int  cardY        = inicioY + i * gapY;

        // Fondo de la tarjeta
        SDL_FRect card = {(float)cardX, (float)cardY, (float)cardW, (float)cardH};
        if (seleccionado && desbloqueado) {
            // Fondo brillante seleccionado
            SDL_SetRenderDrawColor(juego->renderer, 60, 55, 10, 220);
            SDL_RenderFillRect(juego->renderer, &card);
            SDL_SetRenderDrawColor(juego->renderer, 255, 220, 0, 255);
        } else if (desbloqueado) {
            SDL_SetRenderDrawColor(juego->renderer, 25, 35, 25, 200);
            SDL_RenderFillRect(juego->renderer, &card);
            SDL_SetRenderDrawColor(juego->renderer, 80, 120, 80, 255);
        } else {
            // Bloqueado: más oscuro
            SDL_SetRenderDrawColor(juego->renderer, 20, 20, 20, 200);
            SDL_RenderFillRect(juego->renderer, &card);
            SDL_SetRenderDrawColor(juego->renderer, 55, 55, 55, 255);
        }
        SDL_RenderRect(juego->renderer, &card);

        // Numero de nivel + nombre
        SDL_Color colorTitulo = !desbloqueado ? gris
                              : seleccionado  ? amarillo
                                             : verde;

        char linea[64];
        SDL_snprintf(linea, sizeof(linea), "%s", NOMBRES_NIVEL[i]);
        renderizarTexto(juego, linea,
            cardX + (int)(W * 0.025f),
            cardY  + (int)(cardH * 0.08f),
            colorTitulo);

        // Descripcion o candado
        if (desbloqueado) {
            SDL_Color colorDesc = seleccionado ? blanco : gris;
            renderizarTextoPequeno(juego, DESC_NIVEL[i],
                cardX + (int)(W * 0.03f),
                cardY  + (int)(cardH * 0.55f),
                colorDesc);
        } else {
            // Icono candado textual + mensaje
            renderizarTextoPequeno(juego, "[BLOQUEADO]  Supera el nivel anterior para desbloquear",
                cardX + (int)(W * 0.03f),
                cardY  + (int)(cardH * 0.55f),
                rojo);
        }

        // Flecha indicadora al lado izquierdo
        if (seleccionado) {
            SDL_Color colorFlecha = desbloqueado ? amarillo : gris;
            renderizarTexto(juego, ">",
                cardX - (int)(W * 0.025f),
                cardY  + (int)(cardH * 0.08f),
                colorFlecha);
        }
    }

    // ---- Pie de pagina ----
    renderizarTextoPequenoC(juego,
        "Flechas / DPad: navegar    Enter / Cruz: jugar    ESC / Circulo: volver",
        H - (int)(H * 0.05f), gris);

    SDL_RenderPresent(juego->renderer);
}

// ============================================
// Manejo de eventos
// ============================================
void manejarEventosSeleccionNivel(Juego* juego) {
    const int totalNiveles = MAX_NIVELES;
    const int H = VH(juego), W = VW(juego);
    const int cardW  = (int)(W * 0.70f);
    const int cardH  = (int)(H * 0.11f);
    const int cardX  = W / 2 - cardW / 2;
    const int inicioY = (int)(H * 0.20f);
    const int gapY    = (int)(H * 0.135f);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }

        if (e.type == SDL_EVENT_KEY_DOWN) {
            switch (e.key.key) {
            case SDLK_UP: case SDLK_W:
                juego->opcionLevelSelectSeleccionada =
                    (juego->opcionLevelSelectSeleccionada - 1 + totalNiveles) % totalNiveles;
                break;

            case SDLK_DOWN: case SDLK_S:
                juego->opcionLevelSelectSeleccionada =
                    (juego->opcionLevelSelectSeleccionada + 1) % totalNiveles;
                break;

            case SDLK_RETURN: case SDLK_KP_ENTER: {
                int sel = juego->opcionLevelSelectSeleccionada;
                if (juego->nivelesDesbloqueados[sel]) {
                    juego->nivelActual = sel + 1;  // niveles base-1
                    juego->puntosEnNivel = 0;
                    juego->puntuacion    = 0;
                    iniciarCuentaRegresiva(juego);
                }
                break;
            }

            case SDLK_ESCAPE:
                juego->estado = ESTADO_MENU;
                break;

            default: break;
            }
        }

        // Gamepad
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            switch (e.gbutton.button) {
            case SDL_GAMEPAD_BUTTON_DPAD_UP:
                juego->opcionLevelSelectSeleccionada =
                    (juego->opcionLevelSelectSeleccionada - 1 + totalNiveles) % totalNiveles;
                break;
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
                juego->opcionLevelSelectSeleccionada =
                    (juego->opcionLevelSelectSeleccionada + 1) % totalNiveles;
                break;
            case SDL_GAMEPAD_BUTTON_SOUTH: {
                int sel = juego->opcionLevelSelectSeleccionada;
                if (juego->nivelesDesbloqueados[sel]) {
                    juego->nivelActual   = sel + 1;
                    juego->puntosEnNivel = 0;
                    juego->puntuacion    = 0;
                    iniciarCuentaRegresiva(juego);
                }
                break;
            }
            case SDL_GAMEPAD_BUTTON_EAST:   // Circulo PS / B Xbox = volver
                juego->estado = ESTADO_MENU;
                break;
            default: break;
            }
        }

        // Mouse hover
        if (e.type == SDL_EVENT_MOUSE_MOTION) {
            float my = e.motion.y;
            for (int i = 0; i < totalNiveles; i++) {
                float fy = (float)(inicioY + i * gapY);
                if (my >= fy && my <= fy + cardH)
                    juego->opcionLevelSelectSeleccionada = i;
            }
        }

        // Click
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
            float my = e.button.y;
            for (int i = 0; i < totalNiveles; i++) {
                float fy = (float)(inicioY + i * gapY);
                if (my >= fy && my <= fy + cardH) {
                    juego->opcionLevelSelectSeleccionada = i;
                    if (juego->nivelesDesbloqueados[i]) {
                        juego->nivelActual   = i + 1;
                        juego->puntosEnNivel = 0;
                        juego->puntuacion    = 0;
                        iniciarCuentaRegresiva(juego);
                    }
                }
            }
        }

        if (e.type == SDL_EVENT_WINDOW_RESIZED) recargarFuentes(juego);

        if (e.type == SDL_EVENT_GAMEPAD_ADDED && !juego->gamepad)
            juego->gamepad = SDL_OpenGamepad(e.gdevice.which);
        if (e.type == SDL_EVENT_GAMEPAD_REMOVED && juego->gamepad) {
            SDL_CloseGamepad(juego->gamepad); juego->gamepad = nullptr;
        }
    }
}
