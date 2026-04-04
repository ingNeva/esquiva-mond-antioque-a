#pragma once

// ============================================
// World: coordina la logica entre modulos
// que antes se llamaban mutuamente (Enemy <-> Machete,
// Enemy -> Boss, Enemy -> Llave).
//
// Regla: ningun modulo de entities/ incluye a otro.
// Todos llaman a World cuando necesitan
// interactuar con otro subsistema.
// ============================================

#include "../utils/Types.h"

// ── Ciclo de actualizacion completo ──────────
void mundoActualizar(Juego* juego);

// ── Colision helper (unica definicion) ───────
bool verificarColision(SDL_FRect* a, SDL_FRect* b);

// ── Notificaciones que los modulos emiten ────
// Llamadas desde Machete cuando mata un enemigo
void mundoOnEnemigoMuerto(Juego* juego, int indiceEnemigo, float x, float y);

// Llamada desde Machete cuando golpea un pilar
void mundoOnPilarDestruido(Juego* juego, int indicePilar);

// Llamada desde Enemy cuando sale de pantalla
void mundoOnEnemigoEsquivado(Juego* juego, int indiceEnemigo);

// Llamada desde Enemy cuando colisiona con jugador
void mundoOnColisionJugador(Juego* juego);

// Llamada desde Enemy cuando se recoge el trofeo
void mundoOnTrofeoRecogido(Juego* juego);
