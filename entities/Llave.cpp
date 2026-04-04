#include "Llave.h"
#include "../core/Game.h"
#include "../utils/ScoreManager.h"
#include "../entities/Boss.h"
#include <cmath>
#include <cstdlib>

void actualizarLlave(Juego* juego) {
    if (juego->nivelActual >= 5) return;

    int umbral = 0;
    switch (juego->nivelActual) {
        case 1: umbral = PUNTOS_LLAVE_NIVEL_1; break;
        case 2: umbral = PUNTOS_LLAVE_NIVEL_2; break;
        case 3: umbral = PUNTOS_LLAVE_NIVEL_3; break;
        case 4: umbral = PUNTOS_LLAVE_NIVEL_4; break;
        default: return;
    }

    if (!juego->llave.activa && juego->puntosEnNivel >= umbral) {
        int intentos = 0; bool posOk = false;
        while (!posOk && intentos < 50) {
            float lx = 80.0f + (float)(rand() % (ANCHO_VENTANA - 160));
            float ly = 80.0f + (float)(rand() % (ALTO_VENTANA  - 160));
            float ddx = lx - juego->jugador.rect.x;
            float ddy = ly - juego->jugador.rect.y;
            if (sqrtf(ddx*ddx + ddy*ddy) > 200.0f) {
                juego->llave.rect       = {lx, ly, (float)LLAVE_TAMANO, (float)LLAVE_TAMANO};
                juego->llave.activa     = true;
                juego->llave.pulsoTimer = 0.0f;
                juego->llave.nivelDestino = juego->nivelActual + 1;
                posOk = true;
            }
            intentos++;
        }
    }

    if (!juego->llave.activa) return;
    juego->llave.pulsoTimer += 1.0f;

    SDL_FRect hitbox = {
        juego->llave.rect.x + 4, juego->llave.rect.y + 4,
        (float)(LLAVE_TAMANO - 8), (float)(LLAVE_TAMANO - 8)
    };
    if (SDL_HasRectIntersectionFloat(&juego->jugador.rect, &hitbox)) {
        juego->llave.activa = false;
        agregarPuntos(juego, PTS_LLAVE_BONUS,
            juego->llave.rect.x + LLAVE_TAMANO / 2, juego->llave.rect.y);
        juego->nivelActual++;
        juego->puntosEnNivel = 0;
        juego->llave.activa  = false;
        iniciarTransicionNivel(juego, juego->nivelActual);
    }
}
void renderizarLlave(Juego* juego) {
    if (!juego->llave.activa) return;
    float pulse = 0.5f + 0.5f * sinf(juego->llave.pulsoTimer * 0.07f);
    int alpha = (int)(80 + 100 * pulse);
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(juego->renderer, 255, 200, 0, (Uint8)alpha);
    SDL_FRect glow = {juego->llave.rect.x - 8, juego->llave.rect.y - 8,
        juego->llave.rect.w + 16, juego->llave.rect.h + 16};
    SDL_RenderFillRect(juego->renderer, &glow);
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);
    SDL_Texture* tex = juego->texLlave ? juego->texLlave : juego->texEnemigo;
    float escala = 1.0f + 0.08f * sinf(juego->llave.pulsoTimer * 0.05f);
    int tw = (int)(LLAVE_TAMANO * escala), th = (int)(LLAVE_TAMANO * escala);
    SDL_FRect dst = {
        juego->llave.rect.x - (tw - LLAVE_TAMANO) * 0.5f,
        juego->llave.rect.y - (th - LLAVE_TAMANO) * 0.5f,
        (float)tw, (float)th
    };
    SDL_RenderTexture(juego->renderer, tex, NULL, &dst);
    SDL_Color dorado = {255, 200, 0, (Uint8)(180 + 75 * pulse)};
    renderizarTextoPequeno(juego, "LLAVE!",
        (int)(juego->llave.rect.x - 10), (int)(juego->llave.rect.y - 28), dorado);
}
