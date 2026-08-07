#include "Enemy.h"
#include "../core/Game.h"
#include "../utils/ScoreManager.h"
#include <cmath>
#include <cstring>
#include <cstdlib>

void inicializarEnemigos(Juego* juego) {
    juego->enemigosActivos = 1;
    juego->puntuacion      = 0;
    generarEnemigo(&juego->enemigos[0], 1);
}

static void orientarHaciaJugador(Enemigo* en, const Jugador* jugador, float speed) {
    float tx = jugador->rect.x + jugador->rect.w * 0.5f;
    float ty = jugador->rect.y + jugador->rect.h * 0.5f;
    float ex = en->rect.x + en->rect.w * 0.5f;
    float ey = en->rect.y + en->rect.h * 0.5f;
    float dx = tx - ex, dy = ty - ey;
    float dist = sqrtf(dx*dx + dy*dy);
    if (dist > 1.0f) {
        en->velX = (dx / dist) * speed;
        en->velY = (dy / dist) * speed;
    }
    en->apuntaAlJugador = true;
}

void generarEnemigo(Enemigo* en, int nivel) {
    generarEnemigoConJugador(en, nivel, nullptr);
}

void generarEnemigoConJugador(Enemigo* en, int nivel, const Jugador* jugador) {
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
    en->vida                  = 1;
    en->anguloZigzag          = 0.0f;
    en->timerBomba            = 0.0f;
    en->apuntaAlJugador       = false;
    en->esquiveCercanoContado = false;
    en->dentroBurbujaEsquive  = false;
    en->distMinAlcanzada      = 9999.0f;

    int r = rand() % 100;
    if (nivel >= 5) {
        if      (r < 35) en->tipo = ENEMIGO_ESPEJO;
        else if (r < 55) en->tipo = ENEMIGO_BOMBARDERO;
        else if (r < 75) en->tipo = ENEMIGO_ZIGZAG;
        else             en->tipo = ENEMIGO_RAPIDO;
    } else if (nivel == 4) {
        if      (r < 30) en->tipo = ENEMIGO_BOMBARDERO;
        else if (r < 55) en->tipo = ENEMIGO_ZIGZAG;
        else if (r < 75) en->tipo = ENEMIGO_ESPEJO;
        else if (r < 90) en->tipo = ENEMIGO_RAPIDO;
        else             en->tipo = ENEMIGO_BASICO;
    } else if (nivel == 3) {
        if      (r < 30) en->tipo = ENEMIGO_ZIGZAG;
        else if (r < 55) en->tipo = ENEMIGO_ESPEJO;
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
            en->rect.w = 48.0f; en->rect.h = 48.0f;
            break;
        case ENEMIGO_ESPEJO:
            en->rect.w = 72.0f; en->rect.h = 72.0f;
            break;
        case ENEMIGO_ZIGZAG:
            en->velX *= 0.9f; en->velY *= 0.9f; break;
        default: break;
    }

    if (jugador && nivel <= 3 && en->tipo != ENEMIGO_ESPEJO) {
        float speed = sqrtf(en->velX*en->velX + en->velY*en->velY);
        orientarHaciaJugador(en, jugador, speed);
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
            float screenW = (float)VW(juego);
            float screenH = (float)VH(juego);
            float targetX = (screenW - TAMANO_SPRITE) - jugador.rect.x;
            float targetY = (screenH - TAMANO_SPRITE) - jugador.rect.y;
            float ddx = targetX - en->rect.x;
            float ddy = targetY - en->rect.y;
            float dist = sqrtf(ddx*ddx + ddy*ddy);
            if (dist > 1.0f) {
                float vel = (nivel >= 5) ? 4.5f
                          : (nivel == 4) ? 3.2f
                                         : 2.2f;
                en->rect.x += (ddx / dist) * vel;
                en->rect.y += (ddy / dist) * vel;
            }
            break;
        }
        default: break;
    }
}

// ============================================
// Floating texts — puntos, racha y frases
// ============================================
void renderizarFloatingTexts(Juego* juego) {
    for (int i = 0; i < MAX_FLOATING_TEXT; i++) {
        FloatingText& ft = juego->floatingTexts[i];
        if (!ft.activo) continue;

        float alpha = (float)ft.timer / (float)FLOATING_TEXT_DURACION;
        if (alpha > 1.0f) alpha = 1.0f;  // frases duran más, clampear

        SDL_Color col = {
            (Uint8)(ft.colorR * 255),
            (Uint8)(ft.colorG * 255),
            (Uint8)(ft.colorB * 255),
            (Uint8)(alpha * 255)
        };

        const char* txt_ptr = nullptr;
        char buf[16];

        if (ft.frase[0] != '\0') {
            // Es una frase colombiana — siempre fuente grande
            txt_ptr = ft.frase;
        } else if (ft.valor < 0) {
            // Es racha — mostrar "racha X"
            SDL_snprintf(buf, sizeof(buf), "racha %d", -ft.valor);
            txt_ptr = buf;
        } else {
            // Es puntuación normal
            SDL_snprintf(buf, sizeof(buf), "+%d", ft.valor);
            txt_ptr = buf;
        }

        // Fuente: frases y combo alto usan fuente grande; racha y puntos normales pequeña
        TTF_Font* fnt;
        float escala;
        if (ft.frase[0] != '\0') {
            fnt    = juego->fuentePequena;
            escala = 1.2f;
        } else if (ft.valor < 0) {
            // racha
            fnt    = juego->fuentePequena;
            escala = 1.0f;
        } else {
            // puntos — escala base 1.4, crece con combo alto
            fnt    = juego->fuente;
            escala = 1.4f + (juego->multiplicador - 1.0f) * 0.35f;
        }

        SDL_Surface* sup = TTF_RenderText_Solid(fnt, txt_ptr, 0, col);
        if (sup) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(juego->renderer, sup);
            if (tex) {
                SDL_SetTextureAlphaMod(tex, col.a);
                int sw = (int)(sup->w * escala);
                int sh = (int)(sup->h * escala);
                SDL_FRect dst = {ft.x - sw / 2.0f, ft.y, (float)sw, (float)sh};
                SDL_RenderTexture(juego->renderer, tex, NULL, &dst);
                SDL_DestroyTexture(tex);
            }
            SDL_DestroySurface(sup);
        }
    }
}

// ============================================
// HUD combo
// ============================================
void renderizarHUDCombo(Juego* juego) {
    const int W = VW(juego), H = VH(juego);

    if (!juego->llave.activa && juego->nivelActual < 5) {
        int umbral = 0;
        switch (juego->nivelActual) {
            case 1: umbral = PUNTOS_LLAVE_NIVEL_1; break;
            case 2: umbral = PUNTOS_LLAVE_NIVEL_2; break;
            case 3: umbral = PUNTOS_LLAVE_NIVEL_3; break;
            case 4: umbral = PUNTOS_LLAVE_NIVEL_4; break;
        }
        int falta = umbral - juego->puntosEnNivel;
        if (falta > 0 && falta <= 30) {
            char txtLlave[48];
            SDL_snprintf(txtLlave, sizeof(txtLlave), "Llave en %d pts!", falta);
            SDL_Color dorado = {255, 200, 0, 255};
            renderizarTextoPequeno(juego, txtLlave,
                W - (int)(W * 0.115f), (int)(H * 0.083f), dorado);
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
