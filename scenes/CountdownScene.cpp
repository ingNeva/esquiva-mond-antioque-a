#include "CountdownScene.h"
#include "../core/Game.h"
#include "../entities/Player.h"
#include "../entities/Enemy.h"
#include "../entities/Machete.h"
#include <cmath>

void iniciarCuentaRegresiva(Juego* juego) {
    inicializarJugador(&juego->jugador);
    inicializarEnemigos(juego);
    inicializarMachete(juego);
    juego->macheteAparecido      = false;
    juego->ultimoNivelDificultad = 0;
    juego->posicionNuevoPuntaje  = -1;
    juego->nivel4Reproducido     = false;
    juego->gameOverReproducido   = false;
    juego->transicion            = {};
    juego->estadoBoss            = BOSS_INACTIVO;
    juego->bossHP                = 0;
    juego->bossSpawneado         = false;
    juego->trofeoActivo          = false;
    juego->pilaresActivos        = 0;
    for (int p = 0; p < MAX_PILARES; p++) juego->pilares[p].activo = false;
    juego->combo = 0; juego->mejorCombo = 0; juego->multiplicador = 1.0f;
    for (int f = 0; f < MAX_FLOATING_TEXT; f++) juego->floatingTexts[f].activo = false;
    juego->llave.activa     = false;
    juego->llave.pulsoTimer = 0.0f;
    juego->nivelActual   = 1;
    juego->puntosEnNivel = 0;
    juego->pistaSonando     = PISTA_NINGUNA;
    juego->inicioCuentaRegresiva = SDL_GetTicks();
    juego->estado = ESTADO_CUENTA_REGRESIVA;
}

void renderizarCuentaRegresiva(Juego* juego) {
    Uint64 elapsed = SDL_GetTicks() - juego->inicioCuentaRegresiva;
    if (elapsed >= 3200) { juego->estado = ESTADO_JUGANDO; return; }

    const int W = VW(juego), H = VH(juego);

    if (juego->texFondos[0]) SDL_RenderTexture(juego->renderer, juego->texFondos[0], NULL, NULL);
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(juego->renderer, 0, 0, 0, 145);
    SDL_FRect overlay = {0, 0, (float)W, (float)H};
    SDL_RenderFillRect(juego->renderer, &overlay);
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color gris     = {160, 160, 160, 255};
    renderizarTextoPequeno(juego, "WASD / Flechas: mover   |   ESPACIO: machete   |   ESC: pausar",
        (int)(W * 0.10f), (int)(H * 0.38f), gris);
    renderizarTextoPequeno(juego, "Esquiva los enemigos -- el machete aparece al nivel 4",
        (int)(W * 0.15f), (int)(H * 0.42f), blanco);

    int cuenta = 3 - (int)(elapsed / 1000);
    if (cuenta < 1) cuenta = 1;
    char numStr[4];
    SDL_snprintf(numStr, sizeof(numStr), "%d", cuenta);
    float fase   = fmodf((float)(elapsed % 1000) / 1000.0f, 1.0f);
    float escala = 1.0f + (1.0f - fase) * 0.5f;
    int tw = 0, th = 0;
    TTF_GetStringSize(juego->fuente, numStr, 0, &tw, &th);
    int bw = (int)(tw * escala), bh = (int)(th * escala);
    SDL_Surface* sup = TTF_RenderText_Solid(juego->fuente, numStr, 0, amarillo);
    if (sup) {
        SDL_Texture* tex = SDL_CreateTextureFromSurface(juego->renderer, sup);
        if (tex) {
            SDL_FRect dst = {(float)((W - bw) / 2), (float)(H/2 - bh/2), (float)bw, (float)bh};
            SDL_RenderTexture(juego->renderer, tex, NULL, &dst);
            SDL_DestroyTexture(tex);
        }
        SDL_DestroySurface(sup);
    }
    SDL_RenderPresent(juego->renderer);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }
        if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) juego->estado = ESTADO_MENU;
    }
}

void renderizarInstrucciones(Juego* juego) {
    SDL_RenderClear(juego->renderer);
    const int W = VW(juego), H = VH(juego);
    const int cx = W / 2;
    // Columna de texto: 20% desde la izquierda
    const int col = (int)(W * 0.10f);
    // Espaciado vertical escalado a la altura
    const float dY = H / 22.0f;  // ~22 lineas caben con margen

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color gris     = {130, 130, 130, 255};
    SDL_Color verde    = { 80, 220,  80, 255};
    SDL_Color naranja  = {255, 165,   0, 255};
    SDL_Color rojo     = {220,  50,  50, 255};

    renderizarTexto(juego, "INSTRUCCIONES", cx - (int)(W*0.08f), (int)(dY*1), amarillo);

    renderizarTexto(juego, "TECLADO",                                       col, (int)(dY* 2.5f), amarillo);
    renderizarTexto(juego, "W / A / S / D       Mover al jugador",          col, (int)(dY* 4.0f), blanco);
    renderizarTexto(juego, "ESPACIO             Atacar con el machete",      col, (int)(dY* 5.2f), blanco);
    renderizarTexto(juego, "ESC                 Pausar el juego",            col, (int)(dY* 6.4f), blanco);
    renderizarTexto(juego, "M                   Silenciar / activar musica", col, (int)(dY* 7.6f), blanco);
    renderizarTexto(juego, "+ / -               Subir / bajar volumen",      col, (int)(dY* 8.8f), blanco);

    renderizarTexto(juego, "CONTROL PS3/PS4/XBOX",                          col, (int)(dY*10.5f), amarillo);
    renderizarTexto(juego, "Stick izq / D-pad   Mover al jugador",          col, (int)(dY*12.0f), blanco);
    renderizarTexto(juego, "Boton Sur (Cruz/A)  Atacar con el machete",     col, (int)(dY*13.2f), blanco);
    renderizarTexto(juego, "START               Pausar el juego",           col, (int)(dY*14.4f), blanco);

    renderizarTexto(juego, "NIVELES",                                        col, (int)(dY*16.0f), amarillo);
    renderizarTextoPequeno(juego, "Niv 1-3  (0-349 pts):  musica tranquila, enemigos basicos",  col, (int)(dY*17.2f), verde);
    renderizarTextoPequeno(juego, "Niv 4    (350-699 pts): intro de jefe, aparece el machete",  col, (int)(dY*18.2f), naranja);
    renderizarTextoPequeno(juego, "Niv 5    (700+ pts):   musica epica, maxima dificultad",     col, (int)(dY*19.2f), rojo);

    renderizarTextoPequeno(juego, "ESC / Circulo(PS3) para volver",
        cx - (int)(W*0.10f), H - (int)(H*0.04f), gris);
    SDL_RenderPresent(juego->renderer);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }
        if (e.type == SDL_EVENT_KEY_DOWN &&
           (e.key.key == SDLK_ESCAPE || e.key.key == SDLK_BACKSPACE))
            juego->estado = ESTADO_MENU;
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN &&
            e.gbutton.button == SDL_GAMEPAD_BUTTON_EAST)
            juego->estado = ESTADO_MENU;
    }
}
