#pragma once

#include "../utils/Types.h"

// ============================================
// Sistema de puntajes
// ============================================
void crearDirectorioSaves();
void cargarPuntajes(TablaPuntajes* tabla);
void guardarPuntajes(const TablaPuntajes* tabla);
bool calificaParaTop5(const TablaPuntajes* tabla, int puntuacion);
int  insertarPuntaje(TablaPuntajes* tabla, const char* nombre, int puntuacion);
void agregarPuntos(Juego* juego, int base, float x, float y);
void actualizarFloatingTexts(Juego* juego);
