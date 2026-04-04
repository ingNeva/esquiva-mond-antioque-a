#include "GameOverScene.h"
#include "GameScene.h"
#include "../core/Game.h"
#include "../core/AudioManager.h"
#include "../utils/ScoreManager.h"
#include <cmath>

// ============================================
// Ingreso de nombre para el top5
// ============================================
void iniciarIngresoNombre(Juego* juego) {
    SDL_memset(juego->nombreIngresado, 0, MAX_NOMBRE);
    juego->longitudNombre       = 0;
    juego->posicionNuevoPuntaje = -1;
    SDL_StartTextInput(juego->ventana);
}

void renderizarIngresoNombre(Juego* juego) {
    SDL_RenderClear(juego->renderer);
    const int W = VW(juego), H = VH(juego);
    const int cx = W / 2;

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color verde    = { 80, 255, 120, 255};
    SDL_Color gris     = {130, 130, 130, 255};

    // Titulo y puntuacion — centrados horizontalmente
    renderizarTextoCentrado(juego, "NUEVO RECORD!", (int)(H * 0.10f), amarillo);

    char msgPuntaje[64];
    SDL_snprintf(msgPuntaje, sizeof(msgPuntaje), "Puntuacion: %d", juego->puntuacion);
    renderizarTextoCentrado(juego, msgPuntaje, (int)(H * 0.18f), blanco);

    renderizarTextoPequenoC(juego,
        "Ingresa tu nombre (max 31 caracteres) y presiona Enter",
        (int)(H * 0.27f), gris);

    // Caja de texto centrada
    const int cajaW = (int)(W * 0.30f);
    const int cajaH = (int)(H * 0.055f);
    const int cajaX = cx - cajaW / 2;
    const int cajaY = (int)(H * 0.33f);

    SDL_SetRenderDrawColor(juego->renderer, 50, 50, 50, 255);
    SDL_FRect caja = {(float)cajaX, (float)cajaY, (float)cajaW, (float)cajaH};
    SDL_RenderFillRect(juego->renderer, &caja);
    SDL_SetRenderDrawColor(juego->renderer, 255, 220, 0, 255);
    SDL_RenderRect(juego->renderer, &caja);

    char textoMostrado[MAX_NOMBRE + 2];
    bool cursorVisible = (SDL_GetTicks() / 500) % 2 == 0;
    SDL_snprintf(textoMostrado, sizeof(textoMostrado), "%s%s",
        juego->nombreIngresado, cursorVisible ? "_" : " ");
    // Texto dentro de la caja, con pequeño margen izquierdo
    renderizarTexto(juego, textoMostrado,
        cajaX + (int)(W * 0.008f),
        cajaY + (cajaH - tamanoFuente(juego)) / 2,
        verde);

    // Top5 en cuarto derecho
    renderizarTop5(juego, (int)(W * 0.72f), (int)(H * 0.10f), -1);

    renderizarTextoPequenoC(juego,
        "Enter: confirmar    ESC: cancelar (puntaje no guardado)",
        H - (int)(H * 0.05f), gris);

    SDL_RenderPresent(juego->renderer);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { SDL_StopTextInput(juego->ventana); juego->ejecutando = false; return; }
        if (e.type == SDL_EVENT_TEXT_INPUT) {
            int espacio = MAX_NOMBRE - 1 - juego->longitudNombre;
            if (espacio > 0) {
                int len    = (int)SDL_strlen(e.text.text);
                int copiar = (len < espacio) ? len : espacio;
                SDL_strlcat(juego->nombreIngresado, e.text.text, MAX_NOMBRE);
                juego->longitudNombre += copiar;
            }
        }
        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_BACKSPACE && juego->longitudNombre > 0) {
                juego->longitudNombre--;
                juego->nombreIngresado[juego->longitudNombre] = '\0';
            }
            if ((e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER) && juego->longitudNombre > 0) {
                SDL_StopTextInput(juego->ventana);
                juego->posicionNuevoPuntaje = insertarPuntaje(
                    &juego->tablaPuntajes, juego->nombreIngresado, juego->puntuacion);
                juego->estado = ESTADO_GAME_OVER;
            }
            if (e.key.key == SDLK_ESCAPE) {
                SDL_StopTextInput(juego->ventana);
                juego->posicionNuevoPuntaje = -1;
                juego->estado = ESTADO_GAME_OVER;
            }
        }
        if (e.type == SDL_EVENT_WINDOW_RESIZED) recargarFuentes(juego);
    }
}

// ============================================
// Game Over
// ============================================
void renderizarGameOver(Juego* juego) {
    SDL_RenderClear(juego->renderer);
    const int W = VW(juego), H = VH(juego);
    const int margenI = (int)(W * 0.05f);

    SDL_Color rojo     = {220,  50,  50, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color verde    = { 80, 255, 120, 255};

    renderizarTexto(juego, "GAME OVER", margenI, (int)(H * 0.14f), rojo);

    char scoreTexto[64];
    SDL_snprintf(scoreTexto, sizeof(scoreTexto), "Puntuacion: %d", juego->puntuacion);
    renderizarTexto(juego, scoreTexto, margenI, (int)(H * 0.23f), blanco);

    char nivelTexto[64];
    SDL_snprintf(nivelTexto, sizeof(nivelTexto), "Nivel alcanzado: %d", juego->nivelActual);
    renderizarTexto(juego, nivelTexto, margenI, (int)(H * 0.31f), amarillo);

    if (juego->posicionNuevoPuntaje >= 0) {
        char msgPos[64];
        SDL_snprintf(msgPos, sizeof(msgPos), "TOP 5! Puesto #%d", juego->posicionNuevoPuntaje + 1);
        renderizarTexto(juego, msgPos, margenI, (int)(H * 0.39f), verde);
    }

    renderizarTexto(juego, "Enter / Cruz    Menu principal", margenI, (int)(H * 0.50f), amarillo);
    renderizarTexto(juego, "ESC / START     Salir",          margenI, (int)(H * 0.58f), amarillo);

    // Divisor central + Top5
    SDL_SetRenderDrawColor(juego->renderer, 70, 70, 70, 255);
    SDL_RenderLine(juego->renderer,
        (float)(W / 2), (float)(H * 0.07f),
        (float)(W / 2), (float)(H * 0.93f));
    renderizarTop5(juego, (int)(W * 0.53f), (int)(H * 0.09f), juego->posicionNuevoPuntaje);

    SDL_RenderPresent(juego->renderer);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }
        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER) {
                reiniciarJuego(juego); juego->estado = ESTADO_MENU;
            }
            if (e.key.key == SDLK_ESCAPE) juego->ejecutando = false;
        }
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
                reiniciarJuego(juego); juego->estado = ESTADO_MENU;
            }
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_START) juego->ejecutando = false;
        }
        if (e.type == SDL_EVENT_WINDOW_RESIZED) recargarFuentes(juego);
    }
}

// ============================================
// Pausa
// ============================================
void renderizarPausa(Juego* juego) {
    dibujarJuego(juego);
    const int W = VW(juego), H = VH(juego);

    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(juego->renderer, 0, 0, 0, 150);
    SDL_FRect overlay = {0.0f, 0.0f, (float)W, (float)H};
    SDL_RenderFillRect(juego->renderer, &overlay);
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color rojo     = {220,  50,  50, 255};

    // Todo centrado horizontalmente
    renderizarTextoCentrado(juego, "PAUSADO",                             (int)(H * 0.18f), amarillo);
    renderizarTextoCentrado(juego, "ESC / START          Continuar",      (int)(H * 0.29f), blanco);
    renderizarTextoCentrado(juego, "Enter / Cruz(PS3)    Volver al menu", (int)(H * 0.37f), blanco);
    renderizarTextoCentrado(juego, "Q                    Salir del juego",(int)(H * 0.45f), rojo);
    renderizarTextoCentrado(juego, "M                    Musica ON/OFF",  (int)(H * 0.53f), blanco);
    renderizarTextoCentrado(juego, "+ / -                Volumen",        (int)(H * 0.61f), blanco);

    SDL_Color colorAudio = juego->musicaActiva
        ? (SDL_Color){80, 255, 120, 255}
        : (SDL_Color){180, 180, 180, 255};
    char textoAudio[48];
    SDL_snprintf(textoAudio, sizeof(textoAudio),
        juego->musicaActiva ? "Audio: ON  Vol:%d%%" : "Audio: SILENCIADO",
        juego->volumenMusica * 100 / 128);
    renderizarTextoPequenoC(juego, textoAudio, (int)(H * 0.69f), colorAudio);

    SDL_RenderPresent(juego->renderer);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }
        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_ESCAPE) juego->estado = ESTADO_JUGANDO;
            if (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER) {
                reiniciarJuego(juego); juego->estado = ESTADO_MENU;
            }
            if (e.key.key == SDLK_Q) juego->ejecutando = false;
            if (e.key.key == SDLK_M) toggleMusicaMute(juego);
            if (e.key.key == SDLK_EQUALS || e.key.key == SDLK_PLUS) ajustarVolumen(juego,  16);
            if (e.key.key == SDLK_MINUS)                             ajustarVolumen(juego, -16);
        }
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_START) juego->estado = ESTADO_JUGANDO;
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
                reiniciarJuego(juego); juego->estado = ESTADO_MENU;
            }
        }
        if (e.type == SDL_EVENT_WINDOW_RESIZED) recargarFuentes(juego);
    }
}

// ============================================
// Victoria
// ============================================
void renderizarVictoria(Juego* juego) {
    SDL_RenderClear(juego->renderer);
    const int W = VW(juego), H = VH(juego);
    const int cx = W / 2;

    SDL_Color dorado   = {255, 200,  30, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color gris     = {130, 130, 130, 255};

    float brillo = 0.15f + 0.08f * sinf((float)SDL_GetTicks() * 0.003f);
    SDL_SetRenderDrawColor(juego->renderer, (Uint8)(brillo*80), (Uint8)(brillo*60), 0, 255);
    SDL_FRect bg = {0, 0, (float)W, (float)H};
    SDL_RenderFillRect(juego->renderer, &bg);

    if (juego->texTrofeo) {
        float escTrofeo = 1.0f + 0.12f * sinf((float)SDL_GetTicks() * 0.005f);
        int tw = (int)(H * 0.12f * escTrofeo);   // tamaño relativo al alto
        int th = tw;
        SDL_FRect tr = {(float)(cx - tw/2), (float)(H * 0.07f), (float)tw, (float)th};
        SDL_RenderTexture(juego->renderer, juego->texTrofeo, NULL, &tr);
    }

    renderizarTextoCentrado(juego, "!BIEN HECHO!", (int)(H * 0.25f), dorado);

    char scoreTexto[64];
    SDL_snprintf(scoreTexto, sizeof(scoreTexto), "Puntuacion final: %d", juego->puntuacion);
    renderizarTextoCentrado(juego, scoreTexto, (int)(H * 0.34f), blanco);

    char comboTexto[64];
    SDL_snprintf(comboTexto, sizeof(comboTexto), "Mejor racha: %d", juego->mejorCombo);
    renderizarTextoPequenoC(juego, comboTexto, (int)(H * 0.42f), amarillo);

    // Divisor + Top5 en lado derecho
    SDL_SetRenderDrawColor(juego->renderer, 70, 70, 0, 255);
    SDL_RenderLine(juego->renderer,
        (float)(cx + (int)(W * 0.10f)), (float)(H * 0.07f),
        (float)(cx + (int)(W * 0.10f)), (float)(H * 0.93f));
    renderizarTop5(juego, cx + (int)(W * 0.12f), (int)(H * 0.09f), -1);

    renderizarTextoPequenoC(juego, "Enter / Cruz  ->  Guardar y volver al menu",
        H - (int)(H * 0.06f), gris);

    SDL_RenderPresent(juego->renderer);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }
        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER || e.key.key == SDLK_ESCAPE) {
                if (calificaParaTop5(&juego->tablaPuntajes, juego->puntuacion)) {
                    iniciarIngresoNombre(juego); juego->estado = ESTADO_INGRESANDO_NOMBRE;
                } else {
                    reiniciarJuego(juego); juego->estado = ESTADO_MENU;
                }
            }
        }
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && e.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
            if (calificaParaTop5(&juego->tablaPuntajes, juego->puntuacion)) {
                iniciarIngresoNombre(juego); juego->estado = ESTADO_INGRESANDO_NOMBRE;
            } else {
                reiniciarJuego(juego); juego->estado = ESTADO_MENU;
            }
        }
        if (e.type == SDL_EVENT_WINDOW_RESIZED) recargarFuentes(juego);
    }
}
