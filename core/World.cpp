#include "World.h"
#include "Game.h"
#include "../utils/ScoreManager.h"
#include "../entities/Player.h"
#include "../entities/Machete.h"
#include "../entities/Enemy.h"
#include "../entities/Boss.h"
#include "../entities/Llave.h"
#include <cmath>
#include <cstdlib>
// ============================================
// Hitbox reducida del jugador
// ============================================
static SDL_FRect hitboxJugador(const Jugador* j) {
    // El sprite de 64x64 tiene sombrero arriba y espacio vacio a los lados.
    // Hitbox ajustada al cuerpo visible (torso + piernas), sin sombrero.
    //
    //  margenX  = 30% a cada lado  → ancho hitbox ≈ 40% del sprite (~25px)
    //  margenTop = 35% arriba      → recorta el sombrero
    //  margenBot = 5%  abajo       → deja los pies
    //
    const float margenX   = j->rect.w * 0.30f;
    const float margenTop = j->rect.h * 0.35f;
    const float margenBot = j->rect.h * 0.05f;
    return {
        j->rect.x + margenX,
        j->rect.y + margenTop,
        j->rect.w - margenX * 2.0f,
        j->rect.h - margenTop - margenBot
    };
}

// ============================================
// Colision AABB  (unica definicion en el proyecto)
// ============================================
bool verificarColision(SDL_FRect* a, SDL_FRect* b) {
    return SDL_HasRectIntersectionFloat(a, b);
}

// ============================================
// Mecánicas especiales — inicialización
// Llamar desde reiniciarJuego() al cambiar de nivel
// ============================================
void mundoIniciarMecanicasNivel(Juego* juego) {
    const float W = (float)VW(juego);
    const float H = (float)VH(juego);
    const float escX = W / 1920.0f;
    const float escY = H / 1080.0f;
    const float zW   = ZONA_RIESGO_W * escX;
    const float zH   = ZONA_RIESGO_H * escY;

    // — Limpiar estado de mecánicas anteriores —
    juego->zonasRiesgoCount  = 0;
    juego->nieblaActiva      = false;
    juego->nieblaSiguiente   = 0;
    juego->ondaActiva        = false;
    juego->ondaSiguiente     = 0;
    juego->ondaRadio         = 0.0f;
    juego->jugadorEmpujonX   = 0.0f;
    juego->jugadorEmpujonY   = 0.0f;
    juego->jugadorEmpujonFin = 0;

    if (juego->nivelActual == 2) {
        // Tres zonas fijas: esquina superior-izquierda, centro-derecha, inferior-centro
        juego->zonasRiesgoCount = ZONA_RIESGO_COUNT;
        juego->zonasRiesgo[0] = { W * 0.05f,  H * 0.08f,  zW, zH };
        juego->zonasRiesgo[1] = { W * 0.72f,  H * 0.38f,  zW, zH };
        juego->zonasRiesgo[2] = { W * 0.38f,  H * 0.74f,  zW, zH };
    }
    else if (juego->nivelActual == 3) {
        // Primera niebla a los 4 segundos de entrar al nivel
        juego->nieblaSiguiente = SDL_GetTicks() + 4000;
    }
    else if (juego->nivelActual == 4) {
        // Primera onda a los 3 segundos de entrar al nivel
        juego->ondaSiguiente = SDL_GetTicks() + 3000;
    }
}

// ============================================
// Mecánicas especiales — actualización por tick
// Llamar desde mundoActualizar() antes del loop de enemigos
// ============================================
static void actualizarMecanicasNivel(Juego* juego) {
    const Uint64 ahora = SDL_GetTicks();
    const float  W     = (float)VW(juego);
    const float  H     = (float)VH(juego);

    // ── Nivel 2: colisión con zonas de riesgo ───────────────────────
    if (juego->nivelActual == 2 && juego->zonasRiesgoCount > 0) {
        SDL_FRect hj = {
            juego->jugador.rect.x + juego->jugador.rect.w * 0.30f,
            juego->jugador.rect.y + juego->jugador.rect.h * 0.35f,
            juego->jugador.rect.w * 0.40f,
            juego->jugador.rect.h * 0.60f
        };
        for (int z = 0; z < juego->zonasRiesgoCount; z++) {
            if (SDL_HasRectIntersectionFloat(&hj, &juego->zonasRiesgo[z])) {
                mundoOnColisionJugador(juego);
                return;
            }
        }
    }

    // ── Nivel 3: ciclo de niebla ─────────────────────────────────────
    if (juego->nivelActual == 3) {
        if (!juego->nieblaActiva && ahora >= juego->nieblaSiguiente) {
            juego->nieblaActiva    = true;
            juego->nieblaFin       = ahora + NIEBLA_DURACION_MS;
        }
        if (juego->nieblaActiva && ahora >= juego->nieblaFin) {
            juego->nieblaActiva    = false;
            juego->nieblaSiguiente = ahora + NIEBLA_INTERVALO_MS;
        }
    }

    // ── Nivel 4: onda expansiva + empujón ───────────────────────────
    if (juego->nivelActual == 4) {
        // Disparar nueva onda
        if (!juego->ondaActiva && ahora >= juego->ondaSiguiente) {
            juego->ondaActiva  = true;
            juego->ondaInicio  = ahora;
            juego->ondaRadio   = 0.0f;
        }
        // Actualizar onda activa
        if (juego->ondaActiva) {
            juego->ondaRadio += ONDA_VELOCIDAD;

            // Detectar si la onda toca al jugador
            float pcx = juego->jugador.rect.x + juego->jugador.rect.w * 0.5f;
            float pcy = juego->jugador.rect.y + juego->jugador.rect.h * 0.5f;
            float cx  = W * 0.5f;
            float cy  = H * 0.5f;
            float dx  = pcx - cx;
            float dy  = pcy - cy;
            float dist = sqrtf(dx*dx + dy*dy);

            // El jugador es golpeado si está dentro del grosor de la onda
            if (dist > 1.0f &&
                dist >= juego->ondaRadio - ONDA_GROSOR * 2.0f &&
                dist <= juego->ondaRadio + ONDA_GROSOR * 2.0f &&
                ahora >= juego->jugadorEmpujonFin)  // no acumular empujones
            {
                // Empujar radialmente hacia afuera desde el centro
                float nx = dx / dist;
                float ny = dy / dist;
                juego->jugadorEmpujonX   = nx * ONDA_EMPUJON_FUERZA;
                juego->jugadorEmpujonY   = ny * ONDA_EMPUJON_FUERZA;
                juego->jugadorEmpujonFin = ahora + ONDA_EMPUJON_DURACION;
            }

            // Onda llegó al borde → resetear
            if (juego->ondaRadio >= ONDA_RADIO_MAX) {
                juego->ondaActiva    = false;
                juego->ondaSiguiente = ahora + ONDA_INTERVALO_MS;
            }
        }

        // Aplicar empujón gradual al jugador
        if (ahora < juego->jugadorEmpujonFin) {
            float progreso   = 1.0f - (float)(juego->jugadorEmpujonFin - ahora)
                                    / (float)ONDA_EMPUJON_DURACION;
            float factor     = 1.0f - progreso;  // desacelera con el tiempo
            float deltaX     = juego->jugadorEmpujonX * factor * 0.016f;
            float deltaY     = juego->jugadorEmpujonY * factor * 0.016f;
            juego->jugador.rect.x = SDL_clamp(
                juego->jugador.rect.x + deltaX, 0.0f, W - juego->jugador.rect.w);
            juego->jugador.rect.y = SDL_clamp(
                juego->jugador.rect.y + deltaY, 0.0f, H - juego->jugador.rect.h);
        }
    }
}

// ============================================
// Ciclo de actualizacion principal
// ============================================
void mundoActualizar(Juego* juego) {
    int nivel = juego->nivelActual;

    // Hitbox reducida — declarada una sola vez al inicio
    SDL_FRect hj = hitboxJugador(&juego->jugador);

    // ── Mecánicas especiales por nivel ───────────────────────────────
    actualizarMecanicasNivel(juego);
    if (!juego->ejecutando) return;  // muerte por zona de riesgo

    // ── Spawn de enemigo extra al subir de nivel ──
    if (juego->nivelActual > juego->ultimoNivelDificultad
        && juego->enemigosActivos < MAX_ENEMIGOS) {
        // En niveles 1-3 el nuevo enemigo apunta directo al jugador
        generarEnemigoConJugador(&juego->enemigos[juego->enemigosActivos],
            juego->nivelActual, &juego->jugador);
        juego->enemigosActivos++;
        juego->ultimoNivelDificultad = juego->nivelActual;
    }

    // ── Aparicion del machete en nivel 4 ─────────
    if (juego->nivelActual >= 4
    && !juego->macheteAparecido && !juego->machete.recogido) {
        aparecerMachete(juego);
        juego->macheteAparecido = true;
    }

    // ── Movimiento y logica de cada enemigo ──────
    for (int i = 0; i < juego->enemigosActivos; i++) {
        Enemigo* en = &juego->enemigos[i];
        moverEnemigo(en, juego->jugador, nivel, juego);

        const float screenW = (float)VW(juego);
        const float screenH = (float)VH(juego);
        if (en->rect.x < -80 || en->rect.x > screenW + 20 ||
            en->rect.y < -80 || en->rect.y > screenH + 20) {
            // Calcular si pasó cerca del jugador (esquive cercano)
            float ecx = en->rect.x + en->rect.w * 0.5f;
            float ecy = en->rect.y + en->rect.h * 0.5f;
            float pcx = juego->jugador.rect.x + juego->jugador.rect.w * 0.5f;
            float pcy = juego->jugador.rect.y + juego->jugador.rect.h * 0.5f;
            // Usamos la posicion mas cercana que tuvo al cruzar (approx: borde)
            // Si venia apuntando al jugador y salio cerca → esquive activo
            if (en->apuntaAlJugador) {
                // Distancia minima estimada al cruzar el borde opuesto
                float dx = ecx - pcx, dy = ecy - pcy;
                float dist = sqrtf(dx*dx + dy*dy);
                if (dist < 180.0f) {
                    mundoOnEsquiveCercano(juego, en);
                }
            }
            mundoOnEnemigoEsquivado(juego, i);
            continue;
        }

        // Deteccion de paso cercano mientras aun esta en pantalla
        {
            float ecx = en->rect.x + en->rect.w * 0.5f;
            float ecy = en->rect.y + en->rect.h * 0.5f;
            float pcx = juego->jugador.rect.x + juego->jugador.rect.w * 0.5f;
            float pcy = juego->jugador.rect.y + juego->jugador.rect.h * 0.5f;
            float dx = ecx - pcx, dy = ecy - pcy;
            float dist = sqrtf(dx*dx + dy*dy);
            // Si apuntaba al jugador, paso muy cerca y aun no fue contado
            if (en->apuntaAlJugador && dist < 55.0f && !en->esquiveCercanoContado) {
                en->esquiveCercanoContado = true;
                mundoOnEsquiveCercano(juego, en);
            }
        }

        // Colision con jugador -> game over
        if (verificarColision(&hj, &en->rect)) {
            mundoOnColisionJugador(juego);
            return;
        }
    }

    // ── Recogida de machete ───────────────────────
    if (!juego->machete.recogido &&
        verificarColision(&hj, &juego->machete.rect)) {
        juego->machete.recogido = true;
        juego->macheteEquipado  = true;
        juego->machete.rect.w   = 48.0f;
        juego->machete.rect.h   = 48.0f;
        SDL_Log("Machete equipado!");
    }
    if (juego->machete.activo) juego->machete.activo = false;

    // ── Trofeo del boss ───────────────────────────
    if (juego->trofeoActivo &&
        verificarColision(&hj, &juego->trofeoRect)) {
        mundoOnTrofeoRecogido(juego);
        return;
    }

    // ── Llave de nivel ────────────────────────────
    actualizarLlave(juego);

    // ── Boss (nivel 5) ────────────────────────────
    if (juego->nivelActual >= 5) actualizarBoss(juego);

    // ── Floating texts ────────────────────────────
    actualizarFloatingTexts(juego);

// ── DEBUG: descomentar #define para ver la hitbox en pantalla ──
// #define DEBUG_HITBOX
#ifdef DEBUG_HITBOX
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(juego->renderer, 255, 0, 0, 120);
    SDL_RenderFillRect(juego->renderer, &hj);
    SDL_SetRenderDrawColor(juego->renderer, 255, 0, 0, 255);
    SDL_RenderRect(juego->renderer, &hj);
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);
#endif
}

// ============================================
// Callbacks / notificaciones
// ============================================

void mundoOnEnemigoMuerto(Juego* juego, int idx, float x, float y) {
    Enemigo* en = &juego->enemigos[idx];
    int pts;
    switch (en->tipo) {
        case ENEMIGO_ESPEJO:     pts = 10;               break;  // reemplaza al tanque
        case ENEMIGO_BOMBARDERO: pts = 8;                break;
        case ENEMIGO_ZIGZAG:     pts = 6;                break;
        case ENEMIGO_RAPIDO:     pts = 4;                break;
        default:                 pts = PTS_MATAR_NORMAL; break;
    }
    agregarPuntos(juego, pts, x, y);
    generarEnemigoConJugador(en, juego->nivelActual, &juego->jugador);
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
        const float bossX = (float)(VW(juego) / 2 - BOSS_TAMANO / 2);
        const float bossY = (float)(VH(juego) / 2 - BOSS_TAMANO / 2);
        juego->estadoBoss        = BOSS_MUERTO;
        juego->trofeoActivo      = true;
        juego->trofeoRect.x      = bossX;
        juego->trofeoRect.y      = bossY;
        juego->trofeoRect.w      = 64.0f;
        juego->trofeoRect.h      = 64.0f;
        agregarPuntos(juego, PTS_ESQUIVAR_BOSS, bossX, bossY);
    }
}

void mundoOnEnemigoEsquivado(Juego* juego, int idx) {
    Enemigo* en = &juego->enemigos[idx];
    int pts;
    switch (en->tipo) {
        case ENEMIGO_ESPEJO:     pts = 8; break;
        case ENEMIGO_BOMBARDERO: pts = 6; break;
        case ENEMIGO_ZIGZAG:     pts = 5; break;
        case ENEMIGO_RAPIDO:     pts = 4; break;
        default:                 pts = PTS_ESQUIVAR_NORMAL; break;
    }
    agregarPuntos(juego, pts, en->rect.x + en->rect.w / 2.0f, en->rect.y);
    generarEnemigoConJugador(en, juego->nivelActual, &juego->jugador);
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