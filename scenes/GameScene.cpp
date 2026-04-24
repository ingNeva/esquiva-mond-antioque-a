#include "GameScene.h"
#include "../core/Game.h"
#include "../core/World.h"
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
        int nivelOrigenIdx  = SDL_clamp(juego->transicion.nivelNuevo - 2, 0, 4);
        int nivelDestino    = SDL_clamp(juego->transicion.nivelNuevo - 1, 0, 4);
        SDL_Texture* texFondoOrigen  = juego->texFondos[nivelOrigenIdx];
        SDL_Texture* texFondoDestino = juego->texFondos[nivelDestino];
        if (texFondoOrigen)  SDL_RenderTexture(juego->renderer, texFondoOrigen,  NULL, NULL);
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

    // Jugador — spritesheet animado por dirección
    SDL_Texture* texJugador = nullptr;
    switch (juego->jugador.direccion) {
        case DIR_DERECHA:   texJugador = juego->texPlayerRight; break;
        case DIR_IZQUIERDA: texJugador = juego->texPlayerLeft;  break;
        case DIR_ABAJO:     texJugador = juego->texPlayerDown;  break;
        case DIR_ARRIBA:    texJugador = juego->texPlayerUp;    break;
    }
    if (texJugador) {
        SDL_FRect src = { (float)(juego->jugador.frameAnim * 64), 0.0f, 64.0f, 64.0f };
        SDL_RenderTexture(juego->renderer, texJugador, &src, &juego->jugador.rect);
    } else {
        SDL_RenderTexture(juego->renderer, juego->texJugador, NULL, &juego->jugador.rect);
    }

    // Enemigos
    for (int i = 0; i < juego->enemigosActivos; i++) {
        Enemigo* en = &juego->enemigos[i];
        SDL_Texture* tex = nullptr;
        switch (en->tipo) {
            case ENEMIGO_RAPIDO:     tex = juego->texEnemigoRapido;     break;
            case ENEMIGO_TANQUE:
                // En nivel 5 el tanque usa la textura verde (espejo)
                tex = (juego->nivelActual >= 5)
                    ? juego->texEnemigoEspejo
                    : juego->texEnemigoTanque;
                break;
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

    // ── Machete en suelo (sin recoger) ────────────────────────────
    if ((juego->nivelActual >= 4 || juego->macheteEquipado) &&
        !juego->machete.recogido &&
        !juego->macheteEquipado)
    {
        SDL_RenderTexture(juego->renderer, juego->texMachete, NULL, &juego->machete.rect);
    }

    // ── Machete equipado: reposo ───────────────────────────────────
    if (juego->macheteEquipado && !juego->machete.animandoAtaque) {
        SDL_RenderTexture(juego->renderer, juego->texMachete, NULL, &juego->machete.rect);
    }

    // ── Machete equipado: ataque circular animado ──────────────────
    // Reemplaza la línea amarilla y el sprite estático anteriores.
    // renderizarMacheteGirando dibuja:
    //   1. Estela de 7 copias semitransparentes con rotación orbital
    //   2. Sprite principal rotado apuntando tangencialmente
    //   3. Destello brillante en la fase de impacto
    if (juego->machete.animandoAtaque) {
        renderizarMacheteGirando(juego);
    }

    mostrarPuntuacionPantalla(juego);
    renderizarBarraCooldown(juego);
    renderizarLlave(juego);
    renderizarHUDCombo(juego);
    renderizarFloatingTexts(juego);

    // ── Nivel 2: zonas de riesgo ─────────────────────────────────────
    if (juego->nivelActual == 2 && juego->zonasRiesgoCount > 0) {
        SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
        for (int z = 0; z < juego->zonasRiesgoCount; z++) {
            // Relleno rojo semitransparente
            SDL_SetRenderDrawColor(juego->renderer, 220, 30, 30, 80);
            SDL_RenderFillRect(juego->renderer, &juego->zonasRiesgo[z]);
            // Borde rojo sólido + efecto de pulso con el tiempo
            float pulso = 0.6f + 0.4f * sinf((float)SDL_GetTicks() * 0.004f);
            SDL_SetRenderDrawColor(juego->renderer,
                255, (Uint8)(20 * pulso), (Uint8)(20 * pulso), 200);
            SDL_RenderRect(juego->renderer, &juego->zonasRiesgo[z]);
            // Línea interior para dar sensación de profundidad
            SDL_FRect inner = {
                juego->zonasRiesgo[z].x + 3,
                juego->zonasRiesgo[z].y + 3,
                juego->zonasRiesgo[z].w - 6,
                juego->zonasRiesgo[z].h - 6
            };
            SDL_SetRenderDrawColor(juego->renderer, 255, 80, 80, 60);
            SDL_RenderRect(juego->renderer, &inner);
        }
        SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);
    }

    // ── Nivel 3: overlay de niebla ───────────────────────────────────
    if (juego->nivelActual == 3 && juego->nieblaActiva) {
        Uint64 ahora   = SDL_GetTicks();
        float  alpha   = 0.0f;
        // Fade in: primeros 600ms
        float  tiempoDesdeInicio = (float)(ahora - (juego->nieblaFin - NIEBLA_DURACION_MS));
        float  tiempoParaFin     = (float)(juego->nieblaFin - ahora);
        if (tiempoDesdeInicio < 600.0f)
            alpha = (tiempoDesdeInicio / 600.0f);
        // Fade out: últimos 600ms
        else if (tiempoParaFin < 600.0f)
            alpha = (tiempoParaFin / 600.0f);
        else
            alpha = 1.0f;
        Uint8 a = (Uint8)(alpha * NIEBLA_ALPHA_MAX);
        SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(juego->renderer, 10, 10, 20, a);
        SDL_FRect pantalla = { 0, 0, (float)VW(juego), (float)VH(juego) };
        SDL_RenderFillRect(juego->renderer, &pantalla);
        SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);
    }

    // ── Nivel 4: onda expansiva ──────────────────────────────────────
    if (juego->nivelActual == 4 && juego->ondaActiva) {
        float cx = VW(juego) * 0.5f;
        float cy = VH(juego) * 0.5f;
        float r  = juego->ondaRadio;
        // Dibuja el anillo como ~60 puntos en circunferencia
        float pulso = 0.6f + 0.4f * sinf((float)SDL_GetTicks() * 0.02f);
        Uint8 alpha = (Uint8)(180 * (1.0f - r / ONDA_RADIO_MAX));  // desvanece al expandirse
        SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
        // Anillo exterior — amarillo eléctrico
        SDL_SetRenderDrawColor(juego->renderer,
            255, (Uint8)(200 * pulso), 0, alpha);
        const int PASOS = 80;
        for (int i = 0; i < PASOS; i++) {
            float ang0 = (float)i       / PASOS * 2.0f * (float)M_PI;
            float ang1 = (float)(i + 1) / PASOS * 2.0f * (float)M_PI;
            for (float dr = -ONDA_GROSOR * 0.5f; dr <= ONDA_GROSOR * 0.5f; dr += 1.5f) {
                float x0 = cx + cosf(ang0) * (r + dr);
                float y0 = cy + sinf(ang0) * (r + dr);
                float x1 = cx + cosf(ang1) * (r + dr);
                float y1 = cy + sinf(ang1) * (r + dr);
                SDL_RenderLine(juego->renderer, x0, y0, x1, y1);
            }
        }
        SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);
    }
}

void renderizar(Juego* juego) {
    dibujarJuego(juego);
    SDL_RenderPresent(juego->renderer);
}
