#pragma once
#include "../utils/Types.h"

// Ciclo principal de física y colisiones
void mundoActualizar(Juego* juego);

// Inicializa las mecánicas especiales del nivel actual
// (zonas de riesgo, niebla, onda). Llamar al entrar a cada nivel.
void mundoIniciarMecanicasNivel(Juego* juego);

// Callbacks internos
void mundoOnEnemigoMuerto      (Juego* juego, int idx, float x, float y);
void mundoOnEnemigoEsquivado   (Juego* juego, int idx);
void mundoOnEsquiveCercano     (Juego* juego, Enemigo* en);
void mundoOnColisionJugador    (Juego* juego);
void mundoOnTrofeoRecogido     (Juego* juego);
void mundoOnPilarDestruido     (Juego* juego, int indicePilar);

// Renderizado auxiliar (barra de vida del boss)
void renderizarBarraVidaBoss   (Juego* juego);
void renderizarBarraCooldown   (Juego* juego);
void mostrarPuntuacionPantalla (Juego* juego);

// Floating texts
void agregarPuntos             (Juego* juego, int pts, float x, float y);
void actualizarFloatingTexts   (Juego* juego);
