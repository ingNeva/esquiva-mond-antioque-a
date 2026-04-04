#include "World.h"
#include "Game.h"
#include "../utils/ScoreManager.h"
#include "../entities/Player.h"
#include "../entities/Machete.h"
#include "../entities/Enemy.h"
#include "../entities/Boss.h"
#include "../entities/Llave.h"

// ============================================
// Colision AABB  (unica definicion en el proyecto)
// ============================================
bool verificarColision(SDL_FRect* a, SDL_FRect* b) {
    return SDL_HasRectIntersectionFloat(a, b);
}

// ============================================
// Ciclo de actualizacion principal
// Reemplaza la logica que antes vivia dispersa
// entre actualizarEnemigos(), actualizarBoss(),
// actualizarLlave() y verificarRecogidaMachete()
// ============================================
void mundoActualizar(Juego* juego) {
    int nivel = nivelActual(juego->puntuacion);

    // ── Spawn de enemigo extra al subir de nivel ──
    if (nivel > juego->ultimoNivelDificultad && juego->enemigosActivos < MAX_ENEMIGOS) {
        generarEnemigo(&juego->enemigos[juego->enemigosActivos], nivel);
        juego->enemigosActivos++;
        if (nivel == 4 || nivel == 5) {
            juego->ultimoNivelDificultad = nivel;
            iniciarTransicionNivel(juego, nivel);
            return;
        }
        juego->ultimoNivelDificultad = nivel;
    }

    // ── Aparicion del machete en nivel 4 ─────────
    if (juego->puntuacion >= UMBRAL_NIVEL_4
        && !juego->macheteAparecido && !juego->machete.recogido) {
        aparecerMachete(juego);
        juego->macheteAparecido = true;
    }

    // ── Movimiento y logica de cada enemigo ──────
    for (int i = 0; i < juego->enemigosActivos; i++) {
        Enemigo* en = &juego->enemigos[i];
        moverEnemigo(en, juego->jugador, nivel, juego);

        // Sale de pantalla -> esquivado
        if (en->rect.x < -80 || en->rect.x > ANCHO_VENTANA + 20 ||
            en->rect.y < -80 || en->rect.y > ALTO_VENTANA  + 20) {
            mundoOnEnemigoEsquivado(juego, i);
            continue;
        }

        // Colision con jugador -> game over
        if (verificarColision(&juego->jugador.rect, &en->rect)) {
            mundoOnColisionJugador(juego);
            return;
        }
    }

    // ── Recogida de machete ───────────────────────
    if (!juego->machete.recogido &&
        verificarColision(&juego->jugador.rect, &juego->machete.rect)) {
        juego->machete.recogido = true;
        juego->macheteEquipado  = true;
        juego->machete.rect.w   = 48.0f;
        juego->machete.rect.h   = 48.0f;
        SDL_Log("Machete equipado!");
    }
    if (juego->machete.activo) juego->machete.activo = false;

    // ── Trofeo del boss ───────────────────────────
    if (juego->trofeoActivo &&
        verificarColision(&juego->jugador.rect, &juego->trofeoRect)) {
        mundoOnTrofeoRecogido(juego);
        return;
    }

    // ── Llave de nivel ────────────────────────────
    actualizarLlave(juego);

    // ── Boss (nivel 5) ────────────────────────────
    if (nivel >= 5) actualizarBoss(juego);

    // ── Floating texts ────────────────────────────
    actualizarFloatingTexts(juego);
}

// ============================================
// Callbacks / notificaciones
// ============================================

void mundoOnEnemigoMuerto(Juego* juego, int idx, float x, float y) {
    Enemigo* en = &juego->enemigos[idx];
    int pts;
    switch (en->tipo) {
        case ENEMIGO_TANQUE:     pts = PTS_MATAR_TANQUE; break;
        case ENEMIGO_BOMBARDERO: pts = 8;                break;
        case ENEMIGO_ESPEJO:     pts = 8;                break;
        case ENEMIGO_ZIGZAG:     pts = 6;                break;
        case ENEMIGO_RAPIDO:     pts = 4;                break;
        default:                 pts = PTS_MATAR_NORMAL; break;
    }
    agregarPuntos(juego, pts, x, y);
    generarEnemigo(en, nivelActual(juego->puntuacion));
}

void mundoOnPilarDestruido(Juego* juego, int indicePilar) {
    if (indicePilar < 0 || indicePilar >= MAX_PILARES) return;
    Pilar* p = &juego->pilares[indicePilar];
    if (!p->activo) return;
    p->activo = false;
    juego->pilaresActivos--;
    juego->bossHP--;
    agregarPuntos(juego, 25, p->rect.x + 24, p->rect.y);
    if (juego->bossHP <= 2) juego->estadoBoss = BOSS_ENFURECIDO;
    if (juego->bossHP <= 0) {
        juego->estadoBoss   = BOSS_MUERTO;
        juego->trofeoActivo = true;
        juego->trofeoRect   = {(float)BOSS_X, (float)BOSS_Y, 64.0f, 64.0f};
        agregarPuntos(juego, PTS_ESQUIVAR_BOSS, (float)BOSS_X, (float)BOSS_Y);
    }
}

void mundoOnEnemigoEsquivado(Juego* juego, int idx) {
    Enemigo* en = &juego->enemigos[idx];
    int pts;
    switch (en->tipo) {
        case ENEMIGO_TANQUE:     pts = 8; break;
        case ENEMIGO_BOMBARDERO: pts = 6; break;
        case ENEMIGO_ESPEJO:     pts = 8; break;
        case ENEMIGO_ZIGZAG:     pts = 5; break;
        case ENEMIGO_RAPIDO:     pts = 4; break;
        default:                 pts = PTS_ESQUIVAR_NORMAL; break;
    }
    agregarPuntos(juego, pts, en->rect.x + en->rect.w / 2.0f, en->rect.y);
    generarEnemigo(en, nivelActual(juego->puntuacion));
}

void mundoOnColisionJugador(Juego* juego) {
    juego->combo         = 0;
    juego->multiplicador = 1.0f;
    juego->ejecutando    = false;
}

void mundoOnTrofeoRecogido(Juego* juego) {
    juego->trofeoActivo = false;
    agregarPuntos(juego, PTS_TROFEO_BONUS, juego->trofeoRect.x, juego->trofeoRect.y);
    juego->estado = ESTADO_VICTORIA;
    if (juego->musicaActiva && juego->musicaVictoria) {
        MIX_StopTrack(juego->trackMusica, 0);
        MIX_SetTrackAudio(juego->trackMusica, juego->musicaVictoria);
        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, 0);
        MIX_PlayTrack(juego->trackMusica, props);
        SDL_DestroyProperties(props);
        juego->pistaSonando = PISTA_NINGUNA;
    }
}
