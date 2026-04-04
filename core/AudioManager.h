#pragma once

#include "../utils/Types.h"

// ============================================
// Gestor de audio
// ============================================
bool        inicializarAudio(Juego* juego);
void        reproducirMusica(Juego* juego, EstadoPista pista);
void        toggleMusicaMute(Juego* juego);
void        ajustarVolumen(Juego* juego, int delta);
void        limpiarAudio(Juego* juego);
EstadoPista pistaSegunEstadoJuego(Juego* juego);
