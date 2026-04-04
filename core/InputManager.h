#pragma once

#include "../utils/Types.h"

// ============================================
// Manejo de entrada (teclado + gamepad)
// ============================================
void manejarEventos(Juego* juego);
void actualizarJugador(Jugador* jugador, SDL_Gamepad* gamepad);
