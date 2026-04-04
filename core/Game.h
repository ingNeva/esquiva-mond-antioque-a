#pragma once

#include "../utils/Types.h"

// ============================================
// Inicializacion SDL, texturas y fuente
// ============================================
bool inicializarSDL(Juego* juego);
bool cargarTexturas(Juego* juego);
bool cargarFuente(Juego* juego);
void limpiarRecursos(Juego* juego);
void reiniciarJuego(Juego* juego);

// ============================================
// Tamaño real de la ventana en tiempo de ejecucion
// Usar SIEMPRE en lugar de ANCHO_VENTANA/ALTO_VENTANA
// en el codigo de escenas/UI.
// ============================================
inline int VW(Juego* juego) {
    int w = ANCHO_VENTANA, h = ALTO_VENTANA;
    if (juego && juego->ventana) SDL_GetWindowSize(juego->ventana, &w, &h);
    return w;
}
inline int VH(Juego* juego) {
    int w = ANCHO_VENTANA, h = ALTO_VENTANA;
    if (juego && juego->ventana) SDL_GetWindowSize(juego->ventana, &w, &h);
    return h;
}

// ============================================
// Configuracion persistente (resolucion, audio…)
// ============================================
#define RUTA_CONFIG "saves/config.bin"
void guardarConfig(const Juego* juego);
void cargarConfig(Juego* juego);

// ============================================
// Utilidades de render de texto
// ============================================
void renderizarTexto(Juego* juego, const char* texto, int x, int y, SDL_Color color);
void renderizarTextoPequeno(Juego* juego, const char* texto, int x, int y, SDL_Color color);
void renderizarTop5(Juego* juego, int x, int y, int posicionResaltada);

// ============================================
// Nivel actual (global, usado en varios modulos)
// ============================================
int nivelActual(int puntuacion);
