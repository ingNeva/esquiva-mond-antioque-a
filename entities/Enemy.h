#pragma once
#include "../utils/Types.h"

void inicializarEnemigos(Juego* juego);
void generarEnemigo(Enemigo* en, int nivel);
void generarEnemigoConJugador(Enemigo* en, int nivel, const Jugador* jugador);
void moverEnemigo(Enemigo* en, const Jugador& jugador, int nivel, Juego* juego);
void renderizarFloatingTexts(Juego* juego);
void renderizarHUDCombo(Juego* juego);
void actualizarFloatingTexts(Juego* juego);
