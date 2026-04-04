#include "Boss.h"
#include "../core/Game.h"
#include "../utils/ScoreManager.h"
#include <cmath>
#include <cstdlib>

// ============================================
// Pilares - posiciones relativas al area de juego
// ============================================
void spawnPilares(Juego* juego) {
    juego->pilaresActivos = 0;
    int intentos = 0;
    // Margen del 8% de cada borde
    const int marginX = (int)(ANCHO_VENTANA * 0.08f);
    const int marginY = (int)(ALTO_VENTANA  * 0.08f);
    for (int p = 0; p < MAX_PILARES; p++) {
        bool posOk = false; float px = 0, py = 0;
        while (!posOk && intentos < 100) {
            px = (float)(marginX + rand() % (ANCHO_VENTANA - marginX * 2));
            py = (float)(marginY + rand() % (ALTO_VENTANA  - marginY * 2));
            float bossX = VW(juego) * 0.475f;
            float bossY = VH(juego) * 0.475f;
            float ddx = px - bossX;
            float ddy = py - bossY;
            posOk = sqrtf(ddx*ddx + ddy*ddy) > 160.0f;
            intentos++;
        }
        juego->pilares[p].rect       = {px, py, 48.0f, 48.0f};
        juego->pilares[p].activo     = true;
        juego->pilares[p].pulsoTimer = (float)(p * 15);
        juego->pilaresActivos++;
    }
}

// ============================================
// Disparo de proyectil
// ============================================
void dispararProyectil(Juego* juego) {
    if (juego->enemigosActivos >= MAX_ENEMIGOS) return;
    float cx = VW(juego) * 0.475f + BOSS_TAMANO / 2.0f;
    float cy = VH(juego) * 0.475f + BOSS_TAMANO / 2.0f;
    float px = juego->jugador.rect.x + TAMANO_SPRITE / 2.0f;
    float py = juego->jugador.rect.y + TAMANO_SPRITE / 2.0f;
    float angulos[3];
    int   numDisparos = 1;
    float angBase = atan2f(py - cy, px - cx);
    if      (juego->bossHP >= 3) { angulos[0] = angBase; numDisparos = 1; }
    else if (juego->bossHP == 2) { angulos[0] = angBase - 0.52f; angulos[1] = angBase + 0.52f; numDisparos = 2; }
    else                         { angulos[0] = angBase - 0.61f; angulos[1] = angBase; angulos[2] = angBase + 0.61f; numDisparos = 3; }
    for (int d = 0; d < numDisparos; d++) {
        if (juego->enemigosActivos >= MAX_ENEMIGOS) break;
        Enemigo* proy = &juego->enemigos[juego->enemigosActivos++];
        proy->rect.x = cx - TAMANO_SPRITE / 2.0f; proy->rect.y = cy - TAMANO_SPRITE / 2.0f;
        proy->rect.w = 32.0f; proy->rect.h = 32.0f;
        proy->velX = cosf(angulos[d]) * BOSS_VEL_PROYECTIL;
        proy->velY = sinf(angulos[d]) * BOSS_VEL_PROYECTIL;
        proy->tipo = ENEMIGO_RAPIDO; proy->vida = 1;
        proy->anguloZigzag = 0.0f; proy->timerBomba = 0.0f;
    }
}

void inicializarBoss(Juego* juego) {
    juego->estadoBoss        = BOSS_ACTIVO;
    juego->bossHP            = BOSS_HP_MAX;
    juego->bossUltimoDisparo = SDL_GetTicks();
    spawnPilares(juego);
}

void actualizarBoss(Juego* juego) {
    if (juego->estadoBoss == BOSS_INACTIVO) {
        if (!juego->bossSpawneado) { inicializarBoss(juego); juego->bossSpawneado = true; }
        return;
    }
    if (juego->estadoBoss == BOSS_MUERTO) return;
    for (int p = 0; p < MAX_PILARES; p++)
        if (juego->pilares[p].activo) juego->pilares[p].pulsoTimer += 1.0f;
    Uint64 cadencia = (juego->estadoBoss == BOSS_ENFURECIDO) ? BOSS_CADENCIA_RABIA : BOSS_CADENCIA_NORMAL;
    Uint64 ahora = SDL_GetTicks();
    if (ahora - juego->bossUltimoDisparo >= cadencia) {
        dispararProyectil(juego);
        juego->bossUltimoDisparo = ahora;
    }
}

// ============================================
// Barra de vida del boss - relativa a VW/VH
// ============================================
void renderizarBarraVidaBoss(Juego* juego) {
    const int W  = VW(juego), H = VH(juego);
    const int barW = (int)(W * 0.21f);        // ~21% del ancho
    const int barH = (int)(H * 0.020f);       // ~2% de la altura
    const int barX = W / 2 - barW / 2;
    const int barY = (int)(H * 0.015f);

    SDL_Color blanco = {255, 255, 255, 255};
    renderizarTextoPequeno(juego, "El Patron",
        barX + barW / 2 - (int)(W * 0.026f), barY - (int)(H * 0.020f), blanco);

    SDL_SetRenderDrawColor(juego->renderer, 20, 20, 20, 255);
    SDL_FRect bg = {(float)barX - 2, (float)barY - 2, (float)barW + 4, (float)barH + 4};
    SDL_RenderFillRect(juego->renderer, &bg);

    float fraccion = (float)juego->bossHP / (float)BOSS_HP_MAX;
    Uint8 br  = (fraccion <= 0.4f) ? 200 : 108;
    Uint8 bg2 = 0;
    Uint8 bb  = (fraccion <= 0.4f) ? 0 : 131;
    if (juego->bossHP <= 2) {
        float parpadeo = 0.5f + 0.5f * sinf((float)SDL_GetTicks() * 0.01f);
        br = (Uint8)(br * parpadeo + 220 * (1.0f - parpadeo));
    }
    SDL_SetRenderDrawColor(juego->renderer, br, bg2, bb, 255);
    SDL_FRect fill = {(float)barX, (float)barY, barW * fraccion, (float)barH};
    SDL_RenderFillRect(juego->renderer, &fill);

    SDL_SetRenderDrawColor(juego->renderer, 200, 150, 255, 255);
    SDL_FRect border = {(float)barX, (float)barY, (float)barW, (float)barH};
    SDL_RenderRect(juego->renderer, &border);

    SDL_SetRenderDrawColor(juego->renderer, 40, 40, 40, 255);
    for (int s = 1; s < BOSS_HP_MAX; s++) {
        float sx = barX + barW * s / (float)BOSS_HP_MAX;
        SDL_RenderLine(juego->renderer, sx, (float)barY, sx, (float)(barY + barH));
    }
}

// ============================================
// Transicion de nivel - relativa a VW/VH
// ============================================
void iniciarTransicionNivel(Juego* juego, int nivelNuevo) {
    juego->transicion.activa         = true;
    juego->transicion.nivelNuevo     = nivelNuevo;
    juego->transicion.inicio         = SDL_GetTicks();
    juego->transicion.estadoAnterior = ESTADO_JUGANDO;
    if (juego->musicaActiva) {
        MIX_StopTrack(juego->trackMusica, 0);
        juego->pistaSonando = PISTA_NINGUNA;
    }
    juego->estado = ESTADO_TRANSICION_NIVEL;
}

void actualizarTransicionNivel(Juego* juego) {
    Uint64 elapsed = SDL_GetTicks() - juego->transicion.inicio;
    if (elapsed >= DURACION_TRANSICION) {
        juego->transicion.activa = false;
        juego->estado = ESTADO_JUGANDO;
        juego->pistaSonando = PISTA_NINGUNA;
    }
}

void renderizarTransicionNivel(Juego* juego) {
    const int W  = VW(juego), H = VH(juego);
    Uint64 elapsed  = SDL_GetTicks() - juego->transicion.inicio;
    float  progreso = (float)elapsed / (float)DURACION_TRANSICION;
    int    nivel    = juego->transicion.nivelNuevo;

    SDL_RenderClear(juego->renderer);
    int idxDestino = SDL_clamp(nivel - 1, 0, 4);
    int idxOrigen  = SDL_clamp(nivel - 2, 0, 4);

    if (juego->texFondos[idxOrigen])
        SDL_RenderTexture(juego->renderer, juego->texFondos[idxOrigen], NULL, NULL);
    if (juego->texFondos[idxDestino]) {
        float alphaNew = SDL_clamp((progreso - 0.3f) / 0.5f, 0.0f, 1.0f);
        SDL_SetTextureAlphaMod(juego->texFondos[idxDestino], (Uint8)(alphaNew * 255.0f));
        SDL_RenderTexture(juego->renderer, juego->texFondos[idxDestino], NULL, NULL);
        SDL_SetTextureAlphaMod(juego->texFondos[idxDestino], 255);
    }

    float fase = sinf(progreso * (float)M_PI * 8.0f) * 0.5f + 0.5f;
    Uint8 ov_r, ov_g, ov_b, barR, barG, barB;
    if (nivel == 4) {
        ov_r = (Uint8)(255*fase + 200*(1-fase)); ov_g = (Uint8)(100*fase + 40*(1-fase)); ov_b = 0;
        barR = 200; barG = 80; barB = 0;
    } else {
        ov_r = (Uint8)(180*fase + 255*(1-fase)); ov_g = 0; ov_b = (Uint8)(255*fase + 180*(1-fase));
        barR = 120; barG = 0; barB = 220;
    }
    Uint8 ov_alpha = (Uint8)(sinf(progreso * (float)M_PI) * 160.0f);

    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(juego->renderer, ov_r, ov_g, ov_b, ov_alpha);
    SDL_FRect overlay = {0.0f, 0.0f, (float)W, (float)H};
    SDL_RenderFillRect(juego->renderer, &overlay);
    if (progreso < 0.15f) {
        float flashAlpha = (0.15f - progreso) / 0.15f;
        SDL_SetRenderDrawColor(juego->renderer, 255, 255, 255, (Uint8)(flashAlpha * 200.0f));
        SDL_RenderFillRect(juego->renderer, &overlay);
    }
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);

    if (progreso > 0.2f && progreso < 0.85f) {
        SDL_Color colorTitulo, colorSubtitulo;
        if (nivel == 4) {
            colorTitulo    = {255, 220,  40, 255};
            colorSubtitulo = {255, 140,   0, 255};
        } else {
            colorTitulo    = {220, 100, 255, 255};
            colorSubtitulo = {180,  40, 255, 255};
        }
        const char* tituloNivel = (nivel == 4) ? "NIVEL 4" : "NIVEL 5";
        const char* subtitulo   = (nivel == 4) ? "!EL JEFE SE ACERCA!" : "!MAXIMA DIFICULTAD!";

        // Centrado real con VW/VH
        int txW = 0, txH = 0, stW = 0, stH = 0;
        TTF_GetStringSize(juego->fuente,        tituloNivel, 0, &txW, &txH);
        TTF_GetStringSize(juego->fuentePequena, subtitulo,   0, &stW, &stH);
        renderizarTexto(juego,        tituloNivel, W/2 - txW/2, H/2 - (int)(H*0.052f), colorTitulo);
        renderizarTextoPequeno(juego, subtitulo,   W/2 - stW/2, H/2 + (int)(H*0.007f), colorSubtitulo);

        const int barW = (int)(W * 0.21f), barH = (int)(H * 0.007f);
        const int barX = W/2 - barW/2, barY = H/2 + (int)(H * 0.065f);
        SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(juego->renderer, 30, 30, 30, 180);
        SDL_FRect barFondo = {(float)barX, (float)barY, (float)barW, (float)barH};
        SDL_RenderFillRect(juego->renderer, &barFondo);
        SDL_SetRenderDrawColor(juego->renderer, barR, barG, barB, 255);
        SDL_FRect barRelleno = {(float)barX, (float)barY, barW * progreso, (float)barH};
        SDL_RenderFillRect(juego->renderer, &barRelleno);
        SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);
    }
    SDL_RenderPresent(juego->renderer);
}