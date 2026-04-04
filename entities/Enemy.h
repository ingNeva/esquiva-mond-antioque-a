#pragma once

#include "../utils/Types.h"

// ============================================
// Enemigos
// Nota: verificarColision vive en core/World.h
// para evitar dependencias cruzadas.
// ============================================
void inicializarEnemigos(Juego* juego);
void generarEnemigo(Enemigo* enemigo, int nivel);

// Mueve UN enemigo y aplica su comportamiento especial.
// No toma decisiones sobre colisiones ni puntuacion:
// eso es responsabilidad de World.
void moverEnemigo(Enemigo* en, const Jugador& jugador, int nivel, Juego* juego);

// Renderizado (llamado desde scenes/GameScene.cpp)
void renderizarFloatingTexts(Juego* juego);
void renderizarHUDCombo(Juego* juego);
