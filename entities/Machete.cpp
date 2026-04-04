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
// Barra de cooldown: posicion y tamaño relativos
// ============================================
void renderizarBarraCooldown(Juego* juego) {
    if (!juego->macheteEquipado) return;
    const int W = VW(juego), H = VH(juego);

    float progreso = calcularProgresoCooldown(juego);

    // Posicion: esquina inferior izquierda, 8% desde abajo
    const int barX    = (int)(W * 0.005f);
    const int barY    = H - (int)(H * 0.075f);
    const int barW    = (int)(W * 0.105f);   // ~10% del ancho
    const int barH    = (int)(H * 0.019f);
    const int iconoSz = (int)(H * 0.026f);
    const int offsetX = iconoSz + 6;

    // Icono del machete
    SDL_FRect icono = {(float)barX, (float)(barY - 4), (float)iconoSz, (float)iconoSz};
    SDL_RenderTexture(juego->renderer, juego->texMachete, NULL, &icono);

    // Fondo de la barra
    SDL_SetRenderDrawColor(juego->renderer, 0, 0, 0, 255);
    SDL_FRect borde = {(float)(barX + offsetX - 2), (float)(barY - 2),
                       (float)(barW + 4), (float)(barH + 4)};
    SDL_RenderFillRect(juego->renderer, &borde);
    SDL_SetRenderDrawColor(juego->renderer, 35, 35, 35, 255);
    SDL_FRect fondo = {(float)(barX + offsetX), (float)barY, (float)barW, (float)barH};
    SDL_RenderFillRect(juego->renderer, &fondo);

    // Relleno con color rojo->verde
    Uint8 r, g, b;
    if (progreso < 0.5f) { r = 255; g = (Uint8)(progreso * 2.0f * 200); b = 0; }
    else { r = (Uint8)((1.0f - (progreso - 0.5f) * 2.0f) * 255); g = 200; b = 0; }
    SDL_SetRenderDrawColor(juego->renderer, r, g, b, 255);
    SDL_FRect relleno = {(float)(barX + offsetX), (float)barY, barW * progreso, (float)barH};
    SDL_RenderFillRect(juego->renderer, &relleno);

    // Brillo
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(juego->renderer, 255, 255, 255, 55);
    SDL_FRect brillo = {(float)(barX + offsetX), (float)barY, barW * progreso, (float)(barH / 2)};
    SDL_RenderFillRect(juego->renderer, &brillo);
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);

    // Etiqueta
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
