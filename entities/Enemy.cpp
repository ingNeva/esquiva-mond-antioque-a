#include "Enemy.h"
#include "../core/Game.h"
#include <cmath>
#include <cstdlib>

void inicializarEnemigos(Juego* juego) {
    juego->enemigosActivos = 1;
    juego->puntuacion      = 0;
    generarEnemigo(&juego->enemigos[0], 1);
}

void generarEnemigo(Enemigo* en, int nivel) {
    int lado = rand() % 4;
    en->rect.w = (float)TAMANO_SPRITE;
    en->rect.h = (float)TAMANO_SPRITE;
    switch (lado) {
        case 0:
            en->rect.x = -(float)TAMANO_SPRITE;
            en->rect.y = (float)(rand() % (ALTO_VENTANA - TAMANO_SPRITE));
            en->velX = 5.0f; en->velY = 0.0f; break;
        case 1:
            en->rect.x = (float)ANCHO_VENTANA;
            en->rect.y = (float)(rand() % (ALTO_VENTANA - TAMANO_SPRITE));
            en->velX = -5.0f; en->velY = 0.0f; break;
        case 2:
            en->rect.x = (float)(rand() % (ANCHO_VENTANA - TAMANO_SPRITE));
            en->rect.y = -(float)TAMANO_SPRITE;
            en->velX = 0.0f; en->velY = 5.0f; break;
        default:
            en->rect.x = (float)(rand() % (ANCHO_VENTANA - TAMANO_SPRITE));
            en->rect.y = (float)ALTO_VENTANA;
            en->velX = 0.0f; en->velY = -5.0f; break;
    }
    en->vida = 1; en->anguloZigzag = 0.0f; en->timerBomba = 0.0f;

    int r = rand() % 100;
    if (nivel >= 5) {
        if      (r < 30) en->tipo = ENEMIGO_ESPEJO;
        else if (r < 55) en->tipo = ENEMIGO_BOMBARDERO;
        else if (r < 75) en->tipo = ENEMIGO_ZIGZAG;
        else if (r < 90) en->tipo = ENEMIGO_TANQUE;
        else             en->tipo = ENEMIGO_RAPIDO;
    } else if (nivel == 4) {
        if      (r < 30) en->tipo = ENEMIGO_BOMBARDERO;
        else if (r < 55) en->tipo = ENEMIGO_ZIGZAG;
        else if (r < 75) en->tipo = ENEMIGO_TANQUE;
        else if (r < 90) en->tipo = ENEMIGO_RAPIDO;
        else             en->tipo = ENEMIGO_BASICO;
    } else if (nivel == 3) {
        if      (r < 30) en->tipo = ENEMIGO_ZIGZAG;
        else if (r < 55) en->tipo = ENEMIGO_TANQUE;
        else if (r < 80) en->tipo = ENEMIGO_RAPIDO;
        else             en->tipo = ENEMIGO_BASICO;
    } else if (nivel == 2) {
        en->tipo = (r < 40) ? ENEMIGO_RAPIDO : ENEMIGO_BASICO;
    } else {
        en->tipo = ENEMIGO_BASICO;
    }
    switch (en->tipo) {
        case ENEMIGO_RAPIDO:
            en->velX *= 1.8f; en->velY *= 1.8f;
            en->rect.w = 48.0f; en->rect.h = 48.0f; break;
        case ENEMIGO_TANQUE:
            en->velX *= 0.5f; en->velY *= 0.5f;
            en->rect.w = 80.0f; en->rect.h = 80.0f; en->vida = 3; break;
        case ENEMIGO_ZIGZAG:
            en->velX *= 0.9f; en->velY *= 0.9f; break;
        default: break;
    }
}

void moverEnemigo(Enemigo* en, const Jugador& jugador, int nivel, Juego* juego) {
    en->rect.x += en->velX;
    en->rect.y += en->velY;
    switch (en->tipo) {
        case ENEMIGO_ZIGZAG:
            en->anguloZigzag += 0.08f;
            if (fabsf(en->velX) > fabsf(en->velY))
                en->rect.y += sinf(en->anguloZigzag) * 4.0f;
            else
                en->rect.x += sinf(en->anguloZigzag) * 4.0f;
            break;
        case ENEMIGO_BOMBARDERO:
            en->timerBomba += 16.0f;
            if (en->timerBomba >= 2000.0f) {
                en->timerBomba = 0.0f;
                for (int s = 0; s < 2; s++) {
                    if (juego->enemigosActivos >= MAX_ENEMIGOS) break;
                    Enemigo* nuevo = &juego->enemigos[juego->enemigosActivos++];
                    generarEnemigo(nuevo, nivel);
                    nuevo->tipo   = ENEMIGO_BASICO;
                    nuevo->rect.w = (float)TAMANO_SPRITE;
                    nuevo->rect.h = (float)TAMANO_SPRITE;
                    nuevo->vida   = 1;
                }
            }
            break;
        case ENEMIGO_ESPEJO: {
            float targetX = (ANCHO_VENTANA - TAMANO_SPRITE) - jugador.rect.x;
            float targetY = (ALTO_VENTANA  - TAMANO_SPRITE) - jugador.rect.y;
            float ddx = targetX - en->rect.x, ddy = targetY - en->rect.y;
            float dist = sqrtf(ddx*ddx + ddy*ddy);
            if (dist > 1.0f) {
                en->rect.x += (ddx / dist) * 4.5f;
                en->rect.y += (ddy / dist) * 4.5f;
            }
            break;
        }
        default: break;
    }
}

// ============================================
// Floating texts
// ============================================
void renderizarFloatingTexts(Juego* juego) {
    for (int i = 0; i < MAX_FLOATING_TEXT; i++) {
        FloatingText& ft = juego->floatingTexts[i];
        if (!ft.activo) continue;
        float alpha = (float)ft.timer / FLOATING_TEXT_DURACION;
        char txt[16];
        SDL_snprintf(txt, sizeof(txt), "+%d", ft.valor);
        SDL_Color col = {
            (Uint8)(ft.colorR * 255), (Uint8)(ft.colorG * 255),
            (Uint8)(ft.colorB * 255), (Uint8)(alpha * 255)
        };
        TTF_Font* fnt = (juego->multiplicador >= 3.0f) ? juego->fuente : juego->fuentePequena;
        SDL_Surface* sup = TTF_RenderText_Solid(fnt, txt, 0, col);
        if (sup) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(juego->renderer, sup);
            if (tex) {
                SDL_SetTextureAlphaMod(tex, col.a);
                float escala = 1.0f + (juego->multiplicador - 1.0f) * 0.3f;
                int sw = (int)(sup->w * escala), sh = (int)(sup->h * escala);
                SDL_FRect dst = {ft.x - sw/2.0f, ft.y, (float)sw, (float)sh};
                SDL_RenderTexture(juego->renderer, tex, NULL, &dst);
                SDL_DestroyTexture(tex);
            }
            SDL_DestroySurface(sup);
        }
    }
}

// ============================================
// HUD combo - posiciones relativas a VW/VH
// ============================================
void renderizarHUDCombo(Juego* juego) {
    const int W = VW(juego), H = VH(juego);

    // Aviso "Llave proxima"
    if (!juego->llave.activa) {
        int nivel = nivelActual(juego->puntuacion);
        if (nivel < 5) {
            int umbralSig = 0;
            switch (nivel) {
                case 1: umbralSig = UMBRAL_NIVEL_2; break;
                case 2: umbralSig = UMBRAL_NIVEL_3; break;
                case 3: umbralSig = UMBRAL_NIVEL_4; break;
                case 4: umbralSig = UMBRAL_NIVEL_5; break;
            }
            int umbralLlave = umbralSig - 20;
            if (juego->puntuacion >= umbralLlave - 30 && juego->puntuacion < umbralSig) {
                int falta = umbralSig - juego->puntuacion;
                char txtLlave[48];
                SDL_snprintf(txtLlave, sizeof(txtLlave), "Llave en %d pts!", falta);
                SDL_Color dorado = {255, 200, 0, 255};
                renderizarTextoPequeno(juego, txtLlave,
                    W - (int)(W * 0.115f), (int)(H * 0.083f), dorado);
            }
        }
    }

    if (juego->combo < 3) return;

    char txtMult[24];
    SDL_Color color;
    if      (juego->combo >= 20) { SDL_snprintf(txtMult, sizeof(txtMult), "x5.0 COMBO!"); color = {200,  50, 255, 255}; }
    else if (juego->combo >= 10) { SDL_snprintf(txtMult, sizeof(txtMult), "x3.0 COMBO!"); color = {255,  50,  50, 255}; }
    else if (juego->combo >=  5) { SDL_snprintf(txtMult, sizeof(txtMult), "x2.0 COMBO");  color = {255, 140,   0, 255}; }
    else                         { SDL_snprintf(txtMult, sizeof(txtMult), "x1.5 COMBO");  color = {255, 220,   0, 255}; }
    if (juego->combo >= 10) {
        float p = 0.7f + 0.3f * sinf((float)SDL_GetTicks() * 0.015f);
        color.a = (Uint8)(p * 255);
    }
    renderizarTextoPequeno(juego, txtMult, W - (int)(W * 0.105f), (int)(H * 0.037f), color);
    char txtCombo[24];
    SDL_snprintf(txtCombo, sizeof(txtCombo), "racha: %d", juego->combo);
    SDL_Color gris = {160, 160, 160, 200};
    renderizarTextoPequeno(juego, txtCombo, W - (int)(W * 0.105f), (int)(H * 0.059f), gris);
}
