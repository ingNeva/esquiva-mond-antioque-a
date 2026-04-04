#include "CountdownScene.h"
#include "../core/Game.h"
#include "../entities/Player.h"
#include "../entities/Enemy.h"
#include "../entities/Machete.h"
#include <cmath>

// ============================================
// iniciarCuentaRegresiva  (sin cambios)
// Ahora llama a iniciarIntro en lugar de poner
// el estado directamente a CUENTA_REGRESIVA
// ============================================
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
    juego->pistaSonando  = PISTA_NINGUNA;

    // Arrancamos la intro en lugar de la cuenta regresiva directa
    iniciarIntro(juego);
}

// ============================================
// renderizarCuentaRegresiva  (sin cambios)
// ============================================
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

// ============================================
// renderizarInstrucciones  (sin cambios)
// ============================================
void renderizarInstrucciones(Juego* juego) {
    SDL_RenderClear(juego->renderer);
    const int W = VW(juego), H = VH(juego);
    const int cx = W / 2;
    const int col = (int)(W * 0.10f);
    const float dY = H / 22.0f;

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


// ============================================================
//  INTRO CINEMÁTICA
//  Fases (tiempo acumulado desde juego->inicioIntro):
//
//  [0 … CAMINAR_MS)    Jugador entra desde el borde izquierdo
//                      caminando hasta el centro de la pantalla.
//                      El machete brilla en el centro esperando.
//
//  [CAMINAR … +RECOGER) Jugador llega al centro.
//                      Efecto de destello al recoger el machete.
//                      Machete desaparece del suelo.
//
//  [CAMINAR+RECOGER … +TEXTO) Se muestra el mensaje:
//                      "¡Destruye las botellas y esquívalas!"
//                      con fade-in/fade-out.
//
//  Al terminar → ESTADO_JUGANDO (con machete ya equipado)
// ============================================================

void iniciarIntro(Juego* juego) {
    const int W = VW(juego), H = VH(juego);

    // Jugador empieza en el borde izquierdo, centrado verticalmente
    juego->jugador.rect.x = -(float)TAMANO_SPRITE;   // fuera de pantalla, entra desde la izq
    juego->jugador.rect.y = (float)(H - TAMANO_SPRITE) / 2.0f;
    juego->jugador.direccion  = DIR_DERECHA;
    juego->jugador.frameAnim  = 0;
    juego->jugador.enMovimiento = true;
    juego->jugador.ultimoFrame  = SDL_GetTicks();

    // Machete aparece en el centro exacto de la pantalla
    juego->machete.rect.x = (float)(W - TAMANO_SPRITE) / 2.0f;
    juego->machete.rect.y = (float)(H - TAMANO_SPRITE) / 2.0f;
    juego->machete.recogido   = false;
    juego->machete.activo     = false;
    juego->macheteEquipado    = false;
    juego->macheteAparecido   = true;   // ya lo pusimos nosotros

    juego->inicioCuentaRegresiva = SDL_GetTicks(); // reutilizamos el campo como timer
    juego->estado = ESTADO_INTRO;
}

// Dibuja el jugador con su spritesheet de caminar
static void dibujarJugadorIntro(Juego* juego) {
    SDL_Texture* tex = juego->texPlayerRight;   // siempre camina a la derecha en la intro
    if (!tex) tex = juego->texJugador;
    if (!tex) return;

    SDL_FRect src = { (float)(juego->jugador.frameAnim * 64), 0.0f, 64.0f, 64.0f };
    SDL_RenderTexture(juego->renderer, tex, &src, &juego->jugador.rect);
}

void renderizarIntro(Juego* juego) {
    // ── consumir eventos (ESC cancela la intro y va directo a jugar) ──
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT)  { juego->ejecutando = false; return; }
        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_ESCAPE) { juego->estado = ESTADO_MENU; return; }
            // Cualquier otra tecla salta la intro
            juego->macheteEquipado = true;
            juego->machete.recogido = true;
            juego->jugador.rect.x = (float)(VW(juego) - TAMANO_SPRITE) / 2.0f;
            juego->jugador.rect.y = (float)(VH(juego) - TAMANO_SPRITE) / 2.0f;
            juego->jugador.enMovimiento = false;
            juego->jugador.frameAnim    = 0;
            juego->estado = ESTADO_JUGANDO;
            return;
        }
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            // Botón sur también salta la intro
            juego->macheteEquipado = true;
            juego->machete.recogido = true;
            juego->jugador.rect.x = (float)(VW(juego) - TAMANO_SPRITE) / 2.0f;
            juego->jugador.rect.y = (float)(VH(juego) - TAMANO_SPRITE) / 2.0f;
            juego->jugador.enMovimiento = false;
            juego->jugador.frameAnim    = 0;
            juego->estado = ESTADO_JUGANDO;
            return;
        }
    }

    const int    W       = VW(juego);
    const int    H       = VH(juego);
    const float  cx      = (float)(W - TAMANO_SPRITE) / 2.0f;  // X destino del jugador
    const float  cy      = (float)(H - TAMANO_SPRITE) / 2.0f;  // Y destino del jugador
    const Uint64 elapsed = SDL_GetTicks() - juego->inicioCuentaRegresiva;

    // ── Fondo ──────────────────────────────────────────────────────────
    if (juego->texFondos[0])
        SDL_RenderTexture(juego->renderer, juego->texFondos[0], NULL, NULL);

    // ── FASE 1: caminar hasta el centro ───────────────────────────────
    if (elapsed < INTRO_DURACION_CAMINAR_MS) {
        float t = (float)elapsed / (float)INTRO_DURACION_CAMINAR_MS;  // 0.0 → 1.0
        // Interpolación suave (ease-out)
        float smooth = 1.0f - (1.0f - t) * (1.0f - t);

        // El jugador parte desde x=-TAMANO_SPRITE hasta cx
        float startX = -(float)TAMANO_SPRITE;
        juego->jugador.rect.x = startX + (cx - startX) * smooth;
        juego->jugador.rect.y = cy;
        juego->jugador.enMovimiento = true;

        // Avanzar frame de caminata
        actualizarAnimacionJugador(&juego->jugador);

        // Machete pulsando en el suelo (brillo senoidal)
        float pulso = 0.7f + 0.3f * sinf((float)elapsed * 0.008f);
        SDL_SetTextureColorMod(juego->texMachete,
            255, (Uint8)(220 * pulso), (Uint8)(50 * pulso));
        SDL_RenderTexture(juego->renderer, juego->texMachete, NULL, &juego->machete.rect);
        SDL_SetTextureColorMod(juego->texMachete, 255, 255, 255);

        // Jugador
        dibujarJugadorIntro(juego);
    }
    // ── FASE 2: destello al recoger ───────────────────────────────────
    else if (elapsed < INTRO_DURACION_CAMINAR_MS + INTRO_DURACION_RECOGER_MS) {
        Uint64 tRecoger = elapsed - INTRO_DURACION_CAMINAR_MS;
        float  t        = (float)tRecoger / (float)INTRO_DURACION_RECOGER_MS; // 0→1

        // Jugador quieto en el centro, mirando a la derecha
        juego->jugador.rect.x   = cx;
        juego->jugador.rect.y   = cy;
        juego->jugador.enMovimiento = false;
        juego->jugador.frameAnim    = 0;

        // Primera mitad: machete sube hacia el jugador (escala crece y se desvanece)
        if (t < 0.5f) {
            float esc    = 1.0f + t * 2.0f;          // 1x → 2x
            Uint8 alpha  = (Uint8)(255 * (1.0f - t * 2.0f));
            int   sz     = (int)(TAMANO_SPRITE * esc);
            SDL_FRect mRect = {
                juego->machete.rect.x - (sz - TAMANO_SPRITE) * 0.5f,
                juego->machete.rect.y - (sz - TAMANO_SPRITE) * 0.5f,
                (float)sz, (float)sz
            };
            SDL_SetTextureAlphaMod(juego->texMachete, alpha);
            SDL_RenderTexture(juego->renderer, juego->texMachete, NULL, &mRect);
            SDL_SetTextureAlphaMod(juego->texMachete, 255);
        }
        // else: machete ya recogido, no se dibuja

        // Destello blanco que se expande y desvanece
        Uint8 flashAlpha = (Uint8)(180 * (1.0f - t));
        SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(juego->renderer, 255, 240, 120, flashAlpha);
        float flashR = t * 120.0f;
        SDL_FRect flash = {
            cx + TAMANO_SPRITE * 0.5f - flashR,
            cy + TAMANO_SPRITE * 0.5f - flashR,
            flashR * 2.0f, flashR * 2.0f
        };
        SDL_RenderFillRect(juego->renderer, &flash);
        SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);

        dibujarJugadorIntro(juego);

        // Al terminar la fase marcamos el machete como equipado
        if (t >= 0.99f) {
            juego->machete.recogido  = true;
            juego->macheteEquipado   = true;
            // Reposicionar el machete al hombro del jugador
            juego->machete.rect.x = juego->jugador.rect.x + OFFSET_MACHETE_X;
            juego->machete.rect.y = juego->jugador.rect.y + OFFSET_MACHETE_Y;
        }
    }
    // ── FASE 3: texto y machete equipado ─────────────────────────────
    else if (elapsed < INTRO_DURACION_TOTAL_MS) {
        Uint64 tTexto = elapsed - INTRO_DURACION_CAMINAR_MS - INTRO_DURACION_RECOGER_MS;
        float  t      = (float)tTexto / (float)INTRO_DURACION_TEXTO_MS;  // 0→1

        juego->machete.recogido = true;
        juego->macheteEquipado  = true;
        juego->jugador.rect.x   = cx;
        juego->jugador.rect.y   = cy;
        juego->jugador.enMovimiento = false;
        juego->jugador.frameAnim    = 0;

        // Machete en reposo al hombro
        juego->machete.rect.x = juego->jugador.rect.x + OFFSET_MACHETE_X;
        juego->machete.rect.y = juego->jugador.rect.y + OFFSET_MACHETE_Y;
        SDL_RenderTexture(juego->renderer, juego->texMachete, NULL, &juego->machete.rect);

        dibujarJugadorIntro(juego);

        // Texto con fade-in (0→0.3) estable (0.3→0.7) fade-out (0.7→1.0)
        Uint8 alpha;
        if      (t < 0.3f) alpha = (Uint8)(255 * (t / 0.3f));
        else if (t < 0.7f) alpha = 255;
        else               alpha = (Uint8)(255 * (1.0f - (t - 0.7f) / 0.3f));

        SDL_Color amarillo= {255, 220,  0,  alpha};
        SDL_Color blanco  = {255, 255, 255, alpha};

        const char* linea1 = "jDestruye las botellas";
        const char* linea2 = "y esquivalas!";

        int textY1 = (int)(H * 0.72f);
        int textY2 = (int)(H * 0.72f) + (int)(H * 0.07f);
        int textX  = W / 2;

        // Sombra (offset +2,+2)
        SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);

        // Texto principal con renderizarTexto del juego
        // Usamos SDL_SetTextureAlphaMod sobre las texturas temporales de texto
        // Para simplificar, dibujamos con TTF directamente con el alpha calculado
        SDL_Surface* s1 = TTF_RenderText_Solid(juego->fuente, linea1, 0, amarillo);
        SDL_Surface* s2 = TTF_RenderText_Solid(juego->fuente, linea2, 0, blanco);

        if (s1) {
            SDL_Texture* t1 = SDL_CreateTextureFromSurface(juego->renderer, s1);
            if (t1) {
                SDL_SetTextureAlphaMod(t1, alpha);
                int tw1 = s1->w, th1 = s1->h;
                // Sombra
                SDL_FRect dstS1 = {(float)(textX - tw1/2 + 2), (float)(textY1 + 2), (float)tw1, (float)th1};
                SDL_SetTextureColorMod(t1, 0, 0, 0);
                SDL_SetTextureAlphaMod(t1, (Uint8)(alpha * 0.5f));
                SDL_RenderTexture(juego->renderer, t1, NULL, &dstS1);
                // Texto real
                SDL_FRect dst1 = {(float)(textX - tw1/2), (float)textY1, (float)tw1, (float)th1};
                SDL_SetTextureColorMod(t1, 255, 220, 0);
                SDL_SetTextureAlphaMod(t1, alpha);
                SDL_RenderTexture(juego->renderer, t1, NULL, &dst1);
                SDL_DestroyTexture(t1);
            }
            SDL_DestroySurface(s1);
        }
        if (s2) {
            SDL_Texture* t2 = SDL_CreateTextureFromSurface(juego->renderer, s2);
            if (t2) {
                SDL_SetTextureAlphaMod(t2, alpha);
                int tw2 = s2->w, th2 = s2->h;
                SDL_FRect dstS2 = {(float)(textX - tw2/2 + 2), (float)(textY2 + 2), (float)tw2, (float)th2};
                SDL_SetTextureColorMod(t2, 0, 0, 0);
                SDL_SetTextureAlphaMod(t2, (Uint8)(alpha * 0.5f));
                SDL_RenderTexture(juego->renderer, t2, NULL, &dstS2);
                SDL_FRect dst2 = {(float)(textX - tw2/2), (float)textY2, (float)tw2, (float)th2};
                SDL_SetTextureColorMod(t2, 255, 255, 255);
                SDL_SetTextureAlphaMod(t2, alpha);
                SDL_RenderTexture(juego->renderer, t2, NULL, &dst2);
                SDL_DestroyTexture(t2);
            }
            SDL_DestroySurface(s2);
        }

        SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);
    }
    // ── FIN: pasar al juego ───────────────────────────────────────────
    else {
        juego->machete.recogido = true;
        juego->macheteEquipado  = true;
        juego->jugador.rect.x   = cx;
        juego->jugador.rect.y   = cy;
        juego->jugador.enMovimiento = false;
        juego->jugador.frameAnim    = 0;
        juego->machete.rect.x = juego->jugador.rect.x + OFFSET_MACHETE_X;
        juego->machete.rect.y = juego->jugador.rect.y + OFFSET_MACHETE_Y;
        juego->estado = ESTADO_JUGANDO;
    }

    SDL_RenderPresent(juego->renderer);
}
