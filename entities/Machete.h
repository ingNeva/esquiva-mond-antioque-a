#pragma once

#include "../utils/Types.h"

// ============================================
// Machete
// ============================================
void  inicializarMachete(Juego* juego);
void  aparecerMachete(Juego* juego);
void  verificarRecogidaMachete(Juego* juego);
void  usarMachete(Juego* juego);
float calcularProgresoCooldown(Juego* juego);
void  renderizarBarraCooldown(Juego* juego);
void  actualizarPosicionMacheteEquipado(Juego* juego);
void  actualizarAnimacionAtaque(Juego* juego);
void  calcularPosicionMacheteGirando(Juego* juego, float* posX, float* posY);
