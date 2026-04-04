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
// Tamaño real de la ventana en tiempo de ejecucion.
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
// Tamaños de fuente escalados a la resolucion actual.
// Base de diseño: 1080p → fuente 36px / 22px.
// ============================================
inline int tamanoFuente(Juego* juego) {
    return SDL_max(14, VH(juego) * 36 / 1080);
}
inline int tamanoFuentePequena(Juego* juego) {
    return SDL_max(10, VH(juego) * 22 / 1080);
}

// ============================================
// Recargar fuentes cuando cambia la resolucion.
// Llamar desde aplicarResolucion() y togglePantallaCompleta().
// ============================================
void recargarFuentes(Juego* juego);

// ============================================
// Configuracion persistente (resolucion, audio...)
// ============================================
#define RUTA_CONFIG "saves/config.bin"
void guardarConfig(const Juego* juego);
void cargarConfig(Juego* juego);

// ============================================
// Utilidades de render de texto
// ============================================
// Posicion absoluta (x, y en pixeles reales)
void renderizarTexto(Juego* juego, const char* texto, int x, int y, SDL_Color color);
void renderizarTextoPequeno(Juego* juego, const char* texto, int x, int y, SDL_Color color);

// Centrado horizontal automatico — solo pasar y
void renderizarTextoCentrado(Juego* juego, const char* texto, int y, SDL_Color color);
void renderizarTextoPequenoC(Juego* juego, const char* texto, int y, SDL_Color color);

void renderizarTop5(Juego* juego, int x, int y, int posicionResaltada);

// ============================================
// Nivel actual (global, usado en varios modulos)
// ============================================
int nivelActual(int puntuacion);
