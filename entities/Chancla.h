#pragma once
#include "../utils/Types.h"

// ============================================
// Chancla — arma bumerán
// Se lanza en la dirección que mira el jugador,
// viaja hasta MAX_DISTANCIA, luego regresa.
// Destruye enemigos a su paso (ida y vuelta).
// ============================================
void inicializarChancla(Juego* juego);
void lanzarChancla(Juego* juego);
void actualizarChancla(Juego* juego);
void renderizarChancla(Juego* juego);
void renderizarBarraCooldownChancla(Juego* juego);
