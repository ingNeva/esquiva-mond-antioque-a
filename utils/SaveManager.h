#pragma once
#include "../utils/Types.h"
#include <cstdio>
#include <cstring>

// ============================================
// Estructura binaria guardada en saves/progreso.bin
// ============================================
struct ProgresoGuardado {
    bool nivelesDesbloqueados[MAX_NIVELES];
    int  ultimoNivelJugado;
    int  version;   // para compatibilidad futura
};

// ============================================
// Guardar progreso en disco
// ============================================
inline void guardarProgreso(const Juego* juego) {
    ProgresoGuardado pg = {};
    pg.version          = 1;
    pg.ultimoNivelJugado = juego->nivelActual;
    for (int i = 0; i < MAX_NIVELES; i++)
        pg.nivelesDesbloqueados[i] = juego->nivelesDesbloqueados[i];

    FILE* f = fopen("saves/progreso.bin", "wb");
    if (f) { fwrite(&pg, sizeof(pg), 1, f); fclose(f); }
}

// ============================================
// Cargar progreso desde disco
// Solo nivel 0 desbloqueado por defecto (primera vez)
// ============================================
inline void cargarProgreso(Juego* juego) {
    // Por defecto: todo bloqueado excepto nivel 1
    for (int i = 0; i < MAX_NIVELES; i++)
        juego->nivelesDesbloqueados[i] = false;
    juego->nivelesDesbloqueados[0] = true;

    FILE* f = fopen("saves/progreso.bin", "rb");
    if (f) {
        ProgresoGuardado pg = {};
        if (fread(&pg, sizeof(pg), 1, f) == 1 && pg.version == 1) {
            for (int i = 0; i < MAX_NIVELES; i++)
                juego->nivelesDesbloqueados[i] = pg.nivelesDesbloqueados[i];
            // Siempre garantizar nivel 1 desbloqueado
            juego->nivelesDesbloqueados[0] = true;
        }
        fclose(f);
    }
}

// ============================================
// Desbloquear el siguiente nivel y guardar
// Llamar cuando el jugador supera un nivel
// ============================================
inline void desbloquearSiguienteNivel(Juego* juego) {
    int siguiente = juego->nivelActual; // nivelActual es base-1, índice = nivelActual
    if (siguiente < MAX_NIVELES)
        juego->nivelesDesbloqueados[siguiente] = true;
    guardarProgreso(juego);
}
