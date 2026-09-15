#pragma once
#include "../utils/Types.h"

void inicializarEnemigos(Juego* juego);
void generarEnemigo(Enemigo* en, int nivel);
void generarEnemigoConJugador(Enemigo* en, int nivel, const Jugador* jugador);
void moverEnemigo(Enemigo* en, const Jugador& jugador, int nivel, Juego* juego);

// ENEMIGO_TANQUE: lo marca "explotando" (inerte, sin colisionar) en vez de
// regenerarlo de inmediato. moverEnemigo() se encarga de esperar
// TANQUE_TIEMPO_EXPLOSION y luego liberar 2 ENEMIGO_BASICO en su posicion.
void iniciarExplosionTanque(Juego* juego, int idx);
void renderizarFloatingTexts(Juego* juego);
void renderizarHUDCombo(Juego* juego);
void actualizarFloatingTexts(Juego* juego);
