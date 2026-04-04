#pragma once

#include "../utils/Types.h"

// ============================================
// Boss
// ============================================
void inicializarBoss(Juego* juego);
void actualizarBoss(Juego* juego);
void renderizarBarraVidaBoss(Juego* juego);
void spawnPilares(Juego* juego);
void dispararProyectil(Juego* juego);
void iniciarTransicionNivel(Juego* juego, int nivelNuevo);
void actualizarTransicionNivel(Juego* juego);
void renderizarTransicionNivel(Juego* juego);
