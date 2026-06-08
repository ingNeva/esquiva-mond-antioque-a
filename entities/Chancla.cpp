#include "Chancla.h"
#include "../core/Game.h"
#include "../core/World.h"
#include "../utils/ScoreManager.h"
#include <cmath>

// ============================================
// Inicializar
// ============================================
void inicializarChancla(Juego* juego) {
    Chancla& c = juego->chancla;
    c.activa        = false;
    c.regresando    = false;
    c.x             = 0.0f;
    c.y             = 0.0f;
    c.velX          = 0.0f;
    c.velY          = 0.0f;
    c.angulo        = 0.0f;
    c.ultimoUso     = 0;
}

// ============================================
// Lanzar
// ============================================
void lanzarChancla(Juego* juego) {
    Uint64 ahora = SDL_GetTicks();
    if (ahora - juego->chancla.ultimoUso < COOLDOWN_CHANCLA) return;
    if (juego->chancla.activa) return;  // ya está volando

    Chancla& c = juego->chancla;

    // Parte desde el centro del jugador
    c.x = juego->jugador.rect.x + juego->jugador.rect.w * 0.5f;
    c.y = juego->jugador.rect.y + juego->jugador.rect.h * 0.5f;

    // Dirección según donde mira el jugador
    float dx = 0.0f, dy = 0.0f;
    switch (juego->jugador.direccion) {
        case DIR_DERECHA:   dx =  1.0f; dy =  0.0f; break;
        case DIR_IZQUIERDA: dx = -1.0f; dy =  0.0f; break;
        case DIR_ABAJO:     dx =  0.0f; dy =  1.0f; break;
        case DIR_ARRIBA:    dx =  0.0f; dy = -1.0f; break;
    }

    c.velX      = dx * CHANCLA_VELOCIDAD;
    c.velY      = dy * CHANCLA_VELOCIDAD;
    c.angulo    = 0.0f;
    c.activa    = true;
    c.regresando = false;
    c.xOrigen   = c.x;
    c.yOrigen   = c.y;
    c.ultimoUso = ahora;
}

// ============================================
// Actualizar — física bumerán
// ============================================
void actualizarChancla(Juego* juego) {
    if (!juego->chancla.activa) return;

    Chancla& c = juego->chancla;

    // Rotar el sprite
    c.angulo += CHANCLA_VEL_ROTACION;
    if (c.angulo >= 360.0f) c.angulo -= 360.0f;

    if (!c.regresando) {
        // ── FASE IDA ──────────────────────────────────────
        c.x += c.velX;
        c.y += c.velY;

        // Distancia recorrida desde el origen
        float dx = c.x - c.xOrigen;
        float dy = c.y - c.yOrigen;
        float dist = sqrtf(dx*dx + dy*dy);

        if (dist >= CHANCLA_MAX_DISTANCIA) {
            // Llegó al límite → invertir dirección (regresar)
            c.regresando = true;
            c.velX = -c.velX;
            c.velY = -c.velY;
        }
    } else {
        // ── FASE VUELTA ───────────────────────────────────
        // Perseguir al jugador (bumerán inteligente)
        float px = juego->jugador.rect.x + juego->jugador.rect.w * 0.5f;
        float py = juego->jugador.rect.y + juego->jugador.rect.h * 0.5f;
        float dx = px - c.x;
        float dy = py - c.y;
        float dist = sqrtf(dx*dx + dy*dy);

        if (dist < CHANCLA_RADIO_RECOGIDA) {
            // Jugador la atrapa → desactivar
            c.activa    = false;
            c.regresando = false;
            return;
        }

        // Ajustar velocidad hacia el jugador suavemente
        float speed = CHANCLA_VELOCIDAD * 1.2f;  // un poco más rápida al volver
        c.velX = (dx / dist) * speed;
        c.velY = (dy / dist) * speed;
        c.x += c.velX;
        c.y += c.velY;
    }

    // ── Colisión con enemigos (ida Y vuelta) ────────────
    SDL_FRect hitbox = {
        c.x - CHANCLA_TAMANO * 0.5f,
        c.y - CHANCLA_TAMANO * 0.5f,
        (float)CHANCLA_TAMANO,
        (float)CHANCLA_TAMANO
    };

    for (int i = 0; i < juego->enemigosActivos; i++) {
        Enemigo* en = &juego->enemigos[i];
        if (SDL_HasRectIntersectionFloat(&hitbox, &en->rect)) {
            en->vida--;
            if (en->vida <= 0) {
                float ex = en->rect.x + en->rect.w * 0.5f;
                float ey = en->rect.y;
                mundoOnEnemigoMuerto(juego, i, ex, ey);
            }
        }
    }

    // ── Colisión con pilares del boss ──────────────────
    if (juego->estadoBoss == BOSS_ACTIVO || juego->estadoBoss == BOSS_ENFURECIDO) {
        for (int p = 0; p < MAX_PILARES; p++) {
            if (!juego->pilares[p].activo) continue;
            if (SDL_HasRectIntersectionFloat(&hitbox, &juego->pilares[p].rect)) {
                mundoOnPilarDestruido(juego, p);
            }
        }
    }

    // ── Salida de pantalla: forzar retorno ─────────────
    float W = (float)VW(juego);
    float H = (float)VH(juego);
    if (c.x < 0 || c.x > W || c.y < 0 || c.y > H) {
        c.regresando = true;
    }
}

// ============================================
// Renderizar — sprite rotado + estela
// ============================================
void renderizarChancla(Juego* juego) {
    if (!juego->chancla.activa) return;

    Chancla& c        = juego->chancla;
    SDL_Renderer* r   = juego->renderer;
    SDL_Texture*  tex = juego->texChancla
                        ? juego->texChancla
                        : juego->texMachete;   // fallback al machete si no cargó

    const float SZ = (float)CHANCLA_TAMANO;

    // ── Estela de copias semitransparentes ────────────
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

    const int   PASOS  = 5;
    const float SEP    = 10.0f;   // px entre cada fantasma

    // Dirección inversa al movimiento para colocar la estela
    float speed = sqrtf(c.velX*c.velX + c.velY*c.velY);
    float nx = (speed > 0.1f) ? c.velX / speed : 0.0f;
    float ny = (speed > 0.1f) ? c.velY / speed : 0.0f;

    for (int i = PASOS; i >= 1; i--) {
        float ex = c.x - nx * SEP * i - SZ * 0.5f;
        float ey = c.y - ny * SEP * i - SZ * 0.5f;
        Uint8 alpha = (Uint8)(25 + (PASOS - i) * 20);

        // Color: naranja-cálido en ida, cian en regreso
        if (!c.regresando)
            SDL_SetTextureColorMod(tex, 255, 160, 60);
        else
            SDL_SetTextureColorMod(tex, 80, 200, 255);

        SDL_SetTextureAlphaMod(tex, alpha);
        SDL_FRect dst = {ex, ey, SZ, SZ};
        double rot = (double)(c.angulo - i * 15.0f);
        SDL_FPoint centro = {SZ * 0.5f, SZ * 0.5f};
        SDL_RenderTextureRotated(r, tex, NULL, &dst, rot, &centro, SDL_FLIP_NONE);
    }

    // ── Sprite principal ──────────────────────────────
    SDL_SetTextureColorMod(tex, 255, 255, 255);
    SDL_SetTextureAlphaMod(tex, 255);

    SDL_FRect dstMain = {
        c.x - SZ * 0.5f,
        c.y - SZ * 0.5f,
        SZ, SZ
    };
    SDL_FPoint centroMain = {SZ * 0.5f, SZ * 0.5f};
    SDL_RenderTextureRotated(r, tex, NULL, &dstMain,
        (double)c.angulo, &centroMain, SDL_FLIP_NONE);

    // ── Destello de peligro (color varía ida/vuelta) ──
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    if (!c.regresando)
        SDL_SetRenderDrawColor(r, 255, 180, 50, 60);
    else
        SDL_SetRenderDrawColor(r, 50, 200, 255, 60);

    SDL_FRect glow = {
        c.x - SZ * 0.8f, c.y - SZ * 0.8f,
        SZ * 1.6f,        SZ * 1.6f
    };
    SDL_RenderFillRect(r, &glow);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    // Restaurar textura
    SDL_SetTextureColorMod(tex, 255, 255, 255);
    SDL_SetTextureAlphaMod(tex, 255);
}

// ============================================
// Barra de cooldown de la chancla
// Se dibuja debajo de la del machete (si existe)
// ============================================
void renderizarBarraCooldownChancla(Juego* juego) {
    const int W = VW(juego), H = VH(juego);

    // Progreso cooldown
    float progreso = 1.0f;
    Uint64 ahora = SDL_GetTicks();
    Uint64 transcurrido = ahora - juego->chancla.ultimoUso;
    if (juego->chancla.ultimoUso > 0 && transcurrido < COOLDOWN_CHANCLA) {
        progreso = (float)transcurrido / (float)COOLDOWN_CHANCLA;
    }

    const int iconoSz = (int)(H * 0.026f);
    const int barX    = (int)(W * 0.005f);
    // Apila debajo de la barra del machete (si está equipado) o arriba sola
    const int barY    = juego->macheteEquipado
                        ? H - (int)(H * 0.075f) - (int)(H * 0.055f)
                        : H - (int)(H * 0.075f);
    const int barW    = (int)(W * 0.105f);
    const int barH    = (int)(H * 0.019f);
    const int offsetX = iconoSz + 6;

    // Icono chancla
    SDL_Texture* tex = juego->texChancla ? juego->texChancla : juego->texMachete;
    SDL_FRect icono  = {(float)barX, (float)(barY - 4), (float)iconoSz, (float)iconoSz};
    SDL_RenderTexture(juego->renderer, tex, NULL, &icono);

    // Fondo barra
    SDL_SetRenderDrawColor(juego->renderer, 0, 0, 0, 255);
    SDL_FRect borde = {(float)(barX + offsetX - 2), (float)(barY - 2),
                       (float)(barW + 4), (float)(barH + 4)};
    SDL_RenderFillRect(juego->renderer, &borde);
    SDL_SetRenderDrawColor(juego->renderer, 35, 35, 35, 255);
    SDL_FRect fondo = {(float)(barX + offsetX), (float)barY, (float)barW, (float)barH};
    SDL_RenderFillRect(juego->renderer, &fondo);

    // Relleno — naranja->amarillo (diferente del machete que es rojo->verde)
    Uint8 rr = 255;
    Uint8 gg = (Uint8)(progreso * 200);
    Uint8 bb = 0;
    SDL_SetRenderDrawColor(juego->renderer, rr, gg, bb, 255);
    SDL_FRect relleno = {(float)(barX + offsetX), (float)barY,
                         barW * progreso, (float)barH};
    SDL_RenderFillRect(juego->renderer, &relleno);

    // Brillo
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(juego->renderer, 255, 255, 255, 55);
    SDL_FRect brillo = {(float)(barX + offsetX), (float)barY,
                        barW * progreso, (float)(barH / 2)};
    SDL_RenderFillRect(juego->renderer, &brillo);
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);

    // Etiqueta
    char etiqueta[32];
    SDL_Color colorEtiqueta;
    if (juego->chancla.activa) {
        SDL_snprintf(etiqueta, sizeof(etiqueta), "EN VUELO...");
        colorEtiqueta = {80, 200, 255, 255};
    } else if (progreso >= 1.0f) {
        SDL_snprintf(etiqueta, sizeof(etiqueta), "LISTO [C]");
        colorEtiqueta = {255, 200, 50, 255};
    } else {
        float restante = (COOLDOWN_CHANCLA - (float)transcurrido) / 1000.0f;
        SDL_snprintf(etiqueta, sizeof(etiqueta), "%.1fs", restante);
        colorEtiqueta = {200, 200, 200, 255};
    }
    renderizarTextoPequeno(juego, etiqueta,
        barX + offsetX + barW + 6, barY, colorEtiqueta);
}
