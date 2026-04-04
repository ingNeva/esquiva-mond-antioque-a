#include "Machete.h"
#include "../core/Game.h"
#include "../core/World.h"
#include "../utils/ScoreManager.h"
#include <cmath>
#include <cstdlib>

void inicializarMachete(Juego* juego) {
    juego->machete.recogido        = false;
    juego->machete.activo          = false;
    juego->machete.ultimoUso       = 0;
    juego->machete.rect.w          = (float)TAMANO_SPRITE;
    juego->machete.rect.h          = (float)TAMANO_SPRITE;
    juego->macheteEquipado         = false;
    juego->machete.animandoAtaque  = false;
    juego->machete.inicioAnimacion = 0;
    juego->machete.anguloActual    = 0.0f;
}

void aparecerMachete(Juego* juego) {
    juego->machete.rect.x   = (float)(rand() % (ANCHO_VENTANA - TAMANO_SPRITE));
    juego->machete.rect.y   = (float)(rand() % (ALTO_VENTANA  - TAMANO_SPRITE));
    juego->machete.recogido = false;
}

void usarMachete(Juego* juego) {
    Uint64 tiempoActual = SDL_GetTicks();
    if (tiempoActual - juego->machete.ultimoUso < COOLDOWN_MACHETE) return;
    juego->machete.activo          = true;
    juego->machete.ultimoUso       = tiempoActual;
    juego->machete.animandoAtaque  = true;
    juego->machete.inicioAnimacion = tiempoActual;
    juego->machete.anguloActual    = 0.0f;

    float cx = juego->jugador.rect.x + TAMANO_SPRITE / 2.0f;
    float cy = juego->jugador.rect.y + TAMANO_SPRITE / 2.0f;

    for (int i = 0; i < juego->enemigosActivos; i++) {
        Enemigo* en = &juego->enemigos[i];
        float dx = (en->rect.x + en->rect.w / 2.0f) - cx;
        float dy = (en->rect.y + en->rect.h / 2.0f) - cy;
        if (sqrtf(dx*dx + dy*dy) <= RANGO_ATAQUE) {
            en->vida--;
            if (en->vida <= 0)
                mundoOnEnemigoMuerto(juego, i, en->rect.x + en->rect.w / 2.0f, en->rect.y);
        }
    }
    if (juego->estadoBoss == BOSS_ACTIVO || juego->estadoBoss == BOSS_ENFURECIDO) {
        for (int p = 0; p < MAX_PILARES; p++) {
            if (!juego->pilares[p].activo) continue;
            float pdx = (juego->pilares[p].rect.x + 24.0f) - cx;
            float pdy = (juego->pilares[p].rect.y + 24.0f) - cy;
            if (sqrtf(pdx*pdx + pdy*pdy) <= RANGO_ATAQUE)
                mundoOnPilarDestruido(juego, p);
        }
    }
}

float calcularProgresoCooldown(Juego* juego) {
    if (juego->machete.ultimoUso == 0) return 1.0f;
    Uint64 t = SDL_GetTicks() - juego->machete.ultimoUso;
    if (t >= COOLDOWN_MACHETE) return 1.0f;
    return (float)t / (float)COOLDOWN_MACHETE;
}

void actualizarAnimacionAtaque(Juego* juego) {
    if (!juego->machete.animandoAtaque) return;
    Uint64 t = SDL_GetTicks() - juego->machete.inicioAnimacion;
    if (t >= DURACION_ANIMACION_ATAQUE) {
        juego->machete.animandoAtaque = false;
        juego->machete.anguloActual   = 0.0f;
    } else {
        juego->machete.anguloActual = ((float)t / DURACION_ANIMACION_ATAQUE) * 360.0f;
    }
}

void calcularPosicionMacheteGirando(Juego* juego, float* posX, float* posY) {
    float cx  = juego->jugador.rect.x + TAMANO_SPRITE / 2.0f;
    float cy  = juego->jugador.rect.y + TAMANO_SPRITE / 2.0f;
    float rad = juego->machete.anguloActual * (float)M_PI / 180.0f;
    *posX = cx + cosf(rad) * RADIO_GIRO_MACHETE - 24.0f;
    *posY = cy + sinf(rad) * RADIO_GIRO_MACHETE - 24.0f;
}

void actualizarPosicionMacheteEquipado(Juego* juego) {
    if (!juego->macheteEquipado) return;
    if (juego->machete.animandoAtaque) {
        float px, py;
        calcularPosicionMacheteGirando(juego, &px, &py);
        juego->machete.rect.x = px;
        juego->machete.rect.y = py;
    } else {
        juego->machete.rect.x = juego->jugador.rect.x + OFFSET_MACHETE_X;
        juego->machete.rect.y = juego->jugador.rect.y + OFFSET_MACHETE_Y;
    }
}

// ============================================
// NUEVO: renderizarMacheteGirando
// Dibuja el machete rotado en su posición orbital
// con una estela de copias semitransparentes que
// dan sensación de golpe circular real.
// Usa solo texMachete (sin assets nuevos).
// ============================================
void renderizarMacheteGirando(Juego* juego) {
    if (!juego->machete.animandoAtaque) return;

    SDL_Renderer* r  = juego->renderer;
    SDL_Texture*  tx = juego->texMachete;
    float cx = juego->jugador.rect.x + TAMANO_SPRITE / 2.0f;
    float cy = juego->jugador.rect.y + TAMANO_SPRITE / 2.0f;

    // Progreso 0.0 → 1.0 de la animación completa
    float progreso = juego->machete.anguloActual / 360.0f;

    // ── 1. Estela: N copias fantasmas detrás del machete ──────────
    // Cada fantasma se dibuja en un ángulo anterior con alpha decreciente
    const int   PASOS_ESTELA  = 7;       // cuántas copias en la estela
    const float SEPARACION    = 18.0f;   // grados entre cada copia
    const float TAMANO_SPRITE_F = (float)TAMANO_SPRITE;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(tx, SDL_BLENDMODE_BLEND);

    for (int i = PASOS_ESTELA; i >= 1; i--) {
        float anguloEstela = juego->machete.anguloActual - i * SEPARACION;
        float radEstela    = anguloEstela * (float)M_PI / 180.0f;
        float ex = cx + cosf(radEstela) * RADIO_GIRO_MACHETE - 24.0f;
        float ey = cy + sinf(radEstela) * RADIO_GIRO_MACHETE - 24.0f;

        // Alpha: más transparente cuanto más atrás en la estela
        Uint8 alpha = (Uint8)(30 + (PASOS_ESTELA - i) * 18);

        // Color amarillo-naranja durante la fase de impacto (mitad del giro)
        // Color blanco-azulado en el inicio/final
        if (progreso > 0.25f && progreso < 0.75f) {
            SDL_SetTextureColorMod(tx, 255, 200, 50);   // naranja cálido
        } else {
            SDL_SetTextureColorMod(tx, 180, 220, 255);  // azul frío
        }
        SDL_SetTextureAlphaMod(tx, alpha);

        SDL_FRect dst = { ex, ey, TAMANO_SPRITE_F, TAMANO_SPRITE_F };
        // El sprite se rota para que "apunte" tangencialmente al círculo
        // (ángulo orbital + 90° para que la hoja quede perpendicular al radio)
        double rotacion = (double)anguloEstela + 90.0;
        SDL_FPoint centro = { TAMANO_SPRITE_F * 0.5f, TAMANO_SPRITE_F * 0.5f };
        SDL_RenderTextureRotated(r, tx, NULL, &dst, rotacion, &centro, SDL_FLIP_NONE);
    }

    // ── 2. Sprite principal del machete ───────────────────────────
    float px, py;
    calcularPosicionMacheteGirando(juego, &px, &py);

    // Color blanco puro, opaco
    SDL_SetTextureColorMod(tx, 255, 255, 255);
    SDL_SetTextureAlphaMod(tx, 255);

    SDL_FRect dstMain = { px, py, TAMANO_SPRITE_F, TAMANO_SPRITE_F };
    double rotacionMain = (double)juego->machete.anguloActual + 90.0;
    SDL_FPoint centroMain = { TAMANO_SPRITE_F * 0.5f, TAMANO_SPRITE_F * 0.5f };
    SDL_RenderTextureRotated(r, tx, NULL, &dstMain, rotacionMain, &centroMain, SDL_FLIP_NONE);

    // ── 3. Destello en el borde del arco (fase de impacto) ────────
    // Un círculo pequeño brillante que viaja con el machete
    if (progreso > 0.15f && progreso < 0.85f) {
        float flash = 0.5f + 0.5f * sinf(progreso * (float)M_PI * 4.0f);
        Uint8 flashAlpha = (Uint8)(80 + flash * 120);
        SDL_SetRenderDrawColor(r, 255, 240, 100, flashAlpha);
        SDL_FRect destello = {
            px + TAMANO_SPRITE_F * 0.25f,
            py + TAMANO_SPRITE_F * 0.25f,
            TAMANO_SPRITE_F * 0.5f,
            TAMANO_SPRITE_F * 0.5f
        };
        SDL_RenderFillRect(r, &destello);
    }

    // Restaurar estado de la textura
    SDL_SetTextureColorMod(tx, 255, 255, 255);
    SDL_SetTextureAlphaMod(tx, 255);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

// ============================================
// Barra de cooldown: posicion y tamaño relativos
// ============================================
void renderizarBarraCooldown(Juego* juego) {
    if (!juego->macheteEquipado) return;
    const int W = VW(juego), H = VH(juego);

    float progreso = calcularProgresoCooldown(juego);

    const int barX    = (int)(W * 0.005f);
    const int barY    = H - (int)(H * 0.075f);
    const int barW    = (int)(W * 0.105f);
    const int barH    = (int)(H * 0.019f);
    const int iconoSz = (int)(H * 0.026f);
    const int offsetX = iconoSz + 6;

    SDL_FRect icono = {(float)barX, (float)(barY - 4), (float)iconoSz, (float)iconoSz};
    SDL_RenderTexture(juego->renderer, juego->texMachete, NULL, &icono);

    SDL_SetRenderDrawColor(juego->renderer, 0, 0, 0, 255);
    SDL_FRect borde = {(float)(barX + offsetX - 2), (float)(barY - 2),
                       (float)(barW + 4), (float)(barH + 4)};
    SDL_RenderFillRect(juego->renderer, &borde);
    SDL_SetRenderDrawColor(juego->renderer, 35, 35, 35, 255);
    SDL_FRect fondo = {(float)(barX + offsetX), (float)barY, (float)barW, (float)barH};
    SDL_RenderFillRect(juego->renderer, &fondo);

    Uint8 r2, g2, b2;
    if (progreso < 0.5f) { r2 = 255; g2 = (Uint8)(progreso * 2.0f * 200); b2 = 0; }
    else { r2 = (Uint8)((1.0f - (progreso - 0.5f) * 2.0f) * 255); g2 = 200; b2 = 0; }
    SDL_SetRenderDrawColor(juego->renderer, r2, g2, b2, 255);
    SDL_FRect relleno = {(float)(barX + offsetX), (float)barY, barW * progreso, (float)barH};
    SDL_RenderFillRect(juego->renderer, &relleno);

    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(juego->renderer, 255, 255, 255, 55);
    SDL_FRect brillo = {(float)(barX + offsetX), (float)barY, barW * progreso, (float)(barH / 2)};
    SDL_RenderFillRect(juego->renderer, &brillo);
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);

    char etiqueta[48];
    SDL_Color colorEtiqueta;
    if (progreso >= 1.0f) {
        SDL_snprintf(etiqueta, sizeof(etiqueta), "LISTO [ESPACIO]");
        colorEtiqueta = {0, 230, 80, 255};
    } else {
        float restante = (COOLDOWN_MACHETE -
            (float)(SDL_GetTicks() - juego->machete.ultimoUso)) / 1000.0f;
        SDL_snprintf(etiqueta, sizeof(etiqueta), "%.1fs", restante);
        colorEtiqueta = {200, 200, 200, 255};
    }
    renderizarTextoPequeno(juego, etiqueta,
        barX + offsetX + barW + 6, barY, colorEtiqueta);
}
