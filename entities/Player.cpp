#include "Player.h"
#include "../core/Game.h"
#include <string>

void inicializarJugador(Jugador* jugador) {
    // Centrado en la ventana logica base; se reposiciona en cada frame por el renderer
    jugador->rect.x    = (float)(ANCHO_VENTANA - TAMANO_SPRITE) / 2;
    jugador->rect.y    = (float)(ALTO_VENTANA  - TAMANO_SPRITE) / 2;
    jugador->rect.w    = (float)TAMANO_SPRITE;
    jugador->rect.h    = (float)TAMANO_SPRITE;
    jugador->velocidad = 4;
}

void mostrarPuntuacionPantalla(Juego* juego) {
    const int W = VW(juego), H = VH(juego);
    // Score: esquina superior izquierda con margen relativo
    SDL_Color blanco = {255, 255, 255, 255};
    std::string txt = "Score: " + std::to_string(juego->puntuacion);
    renderizarTexto(juego, txt.c_str(), (int)(W * 0.005f), (int)(H * 0.009f), blanco);

    int nivel = nivelActual(juego->puntuacion);
    char txtNivel[32];
    SDL_snprintf(txtNivel, sizeof(txtNivel), "Nivel: %d", nivel);
    SDL_Color colorNivel;
    switch (nivel) {
        case 1: case 2: case 3: colorNivel = {100, 220, 100, 255}; break;
        case 4: colorNivel = {255, 165,   0, 255}; break;
        case 5: colorNivel = {220,  50,  50, 255}; break;
        default: colorNivel = {255, 255, 255, 255}; break;
    }
    renderizarTexto(juego, txtNivel, (int)(W * 0.005f), (int)(H * 0.048f), colorNivel);

    // Audio: esquina superior derecha
    SDL_Color colorAudio = juego->musicaActiva
        ? (SDL_Color){80, 255, 120, 255}
        : (SDL_Color){180, 180, 180, 255};
    char textoAudio[32];
    if (juego->musicaActiva)
        SDL_snprintf(textoAudio, sizeof(textoAudio), "Vol:%d%%", juego->volumenMusica * 100 / 128);
    else
        SDL_snprintf(textoAudio, sizeof(textoAudio), "SIN AUDIO");
    renderizarTextoPequeno(juego, textoAudio, W - (int)(W * 0.07f), (int)(H * 0.009f), colorAudio);
}
