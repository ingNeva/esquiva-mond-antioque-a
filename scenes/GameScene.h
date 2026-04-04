#pragma once

#include "../utils/Types.h"

// ============================================
// Escena principal de juego
// ============================================
void dibujarJuego(Juego* juego);
void renderizar(Juego* juego);
void iniciarTransicionNivel(Juego* juego, int nivelDestino);
void actualizarTransicionNivel(Juego* juego);
void renderizarTransicionNivel(Juego* juego);