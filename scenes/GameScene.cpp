#include "GameScene.h"
#include "../core/Game.h"
#include "../entities/Player.h"
#include "../entities/Enemy.h"
#include "../entities/Machete.h"
#include "../entities/Boss.h"
#include "../entities/Llave.h"
#include <cmath>

void dibujarJuego(Juego* juego) {
    SDL_RenderClear(juego->renderer);
    int nivelIdx = SDL_clamp(juego->nivelActual - 1, 0, 4);
    SDL_Texture* texFondoActual = juego->texFondos[nivelIdx];

    if (juego->transicion.activa) {
        int nivelDestino = SDL_clamp(juego->transicion.nivelNuevo - 1, 0, 4);
        SDL_Texture* texFondoDestino = juego->texFondos[nivelDestino];
        if (texFondoActual) SDL_RenderTexture(juego->renderer, texFondoActual, NULL, NULL);
        if (texFondoDestino) {
            float progreso = (float)(SDL_GetTicks() - juego->transicion.inicio) / (float)DURACION_TRANSICION;
            float alpha = SDL_clamp(progreso * 1.6f - 0.3f, 0.0f, 1.0f);
            SDL_SetTextureAlphaMod(texFondoDestino, (Uint8)(alpha * 255.0f));
            SDL_RenderTexture(juego->renderer, texFondoDestino, NULL, NULL);
            SDL_SetTextureAlphaMod(texFondoDestino, 255);
        }
    } else {
        if (texFondoActual) SDL_RenderTexture(juego->renderer, texFondoActual, NULL, NULL);
    }

    // Pilares del boss
    if (juego->estadoBoss == BOSS_ACTIVO || juego->estadoBoss == BOSS_ENFURECIDO) {
        for (int p = 0; p < MAX_PILARES; p++) {
            if (!juego->pilares[p].activo) continue;
            float pulse = 0.6f + 0.4f * sinf(juego->pilares[p].pulsoTimer * 0.06f);
            SDL_SetTextureColorMod(juego->texPilar,
                (Uint8)(200 * pulse), (Uint8)(100 * pulse), (Uint8)(255 * pulse));
            SDL_RenderTexture(juego->renderer, juego->texPilar, NULL, &juego->pilares[p].rect);
            SDL_SetTextureColorMod(juego->texPilar, 255, 255, 255);
        }
    }

    // Boss sprite
    if (juego->estadoBoss == BOSS_ACTIVO || juego->estadoBoss == BOSS_ENFURECIDO) {
        SDL_FRect bossRect = {VW(juego) * 0.475f, VH(juego) * 0.475f, (float)BOSS_TAMANO, (float)BOSS_TAMANO};
        if (juego->estadoBoss == BOSS_ENFURECIDO) {
            float p = 0.5f + 0.5f * sinf((float)SDL_GetTicks() * 0.01f);
            SDL_SetTextureColorMod(juego->texBoss, 255, (Uint8)(50 * p), (Uint8)(50 * p));
        }
        SDL_RenderTexture(juego->renderer, juego->texBoss, NULL, &bossRect);
        SDL_SetTextureColorMod(juego->texBoss, 255, 255, 255);
        renderizarBarraVidaBoss(juego);
    }

    // Trofeo
    if (juego->trofeoActivo) {
        float escTrofeo = 1.0f + 0.1f * sinf((float)SDL_GetTicks() * 0.005f);
        int tw = (int)(64 * escTrofeo), th = (int)(64 * escTrofeo);
        SDL_FRect tr = {
            juego->trofeoRect.x - (tw - 64) * 0.5f,
            juego->trofeoRect.y - (th - 64) * 0.5f,
            (float)tw, (float)th
        };
        SDL_RenderTexture(juego->renderer, juego->texTrofeo, NULL, &tr);
    }

    // Jugador
    SDL_RenderTexture(juego->renderer, juego->texJugador, NULL, &juego->jugador.rect);

    // Enemigos
    for (int i = 0; i < juego->enemigosActivos; i++) {
        Enemigo* en = &juego->enemigos[i];
        SDL_Texture* tex = nullptr;
        switch (en->tipo) {
            case ENEMIGO_RAPIDO:     tex = juego->texEnemigoRapido;     break;
            case ENEMIGO_TANQUE:     tex = juego->texEnemigoTanque;     break;
            case ENEMIGO_ZIGZAG:     tex = juego->texEnemigoZigzag;     break;
            case ENEMIGO_BOMBARDERO: tex = juego->texEnemigoBombardero; break;
            case ENEMIGO_ESPEJO:     tex = juego->texEnemigoEspejo;     break;
            default:                 tex = juego->texEnemigo;           break;
        }
        if (!tex) tex = juego->texEnemigo;
        SDL_RenderTexture(juego->renderer, tex, NULL, &en->rect);
        if (en->tipo == ENEMIGO_TANQUE && en->vida > 0 && en->vida <= 3) {
            float dotX = en->rect.x + en->rect.w / 2.0f - (en->vida * 10.0f) / 2.0f;
            float dotY = en->rect.y - 10.0f;
            for (int v = 0; v < en->vida; v++) {
                SDL_FRect dot = {dotX + v * 12.0f, dotY, 8.0f, 8.0f};
                SDL_SetRenderDrawColor(juego->renderer, 220, 50, 50, 255);
                SDL_RenderFillRect(juego->renderer, &dot);
            }
        }
    }

    // Machete
    if (juego->nivelActual >= 4 || juego->macheteEquipado)
        if (!juego->machete.recogido || juego->macheteEquipado)
            SDL_RenderTexture(juego->renderer, juego->texMachete, NULL, &juego->machete.rect);

    if (juego->machete.activo || juego->machete.animandoAtaque) {
        float cx = juego->jugador.rect.x + TAMANO_SPRITE / 2.0f;
        float cy = juego->jugador.rect.y + TAMANO_SPRITE / 2.0f;
        SDL_SetRenderDrawColor(juego->renderer, 255, 50, 50, 255);
        SDL_FRect rv = {cx - RANGO_ATAQUE, cy - RANGO_ATAQUE, (float)(RANGO_ATAQUE*2), (float)(RANGO_ATAQUE*2)};
        SDL_RenderRect(juego->renderer, &rv);
        if (juego->machete.animandoAtaque) {
            SDL_SetRenderDrawColor(juego->renderer, 255, 255, 0, 255);
            SDL_RenderLine(juego->renderer, cx, cy,
                juego->machete.rect.x + 24.0f, juego->machete.rect.y + 24.0f);
        }
    }

    mostrarPuntuacionPantalla(juego);
    renderizarBarraCooldown(juego);
    renderizarLlave(juego);
    renderizarHUDCombo(juego);
    renderizarFloatingTexts(juego);
}

void renderizar(Juego* juego) {
    dibujarJuego(juego);
    SDL_RenderPresent(juego->renderer);
}