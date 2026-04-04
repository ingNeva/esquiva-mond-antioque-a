#include "InputManager.h"
#include "AudioManager.h"
#include "../entities/Machete.h"

// ============================================
// Eventos durante la partida
// ============================================
void manejarEventos(Juego* juego) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }
        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_SPACE && juego->macheteEquipado) usarMachete(juego);
            if (e.key.key == SDLK_ESCAPE) juego->estado = ESTADO_PAUSADO;
            if (e.key.key == SDLK_M) toggleMusicaMute(juego);
            if (e.key.key == SDLK_EQUALS || e.key.key == SDLK_PLUS) ajustarVolumen(juego, 16);
            if (e.key.key == SDLK_MINUS) ajustarVolumen(juego, -16);
        }
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH && juego->macheteEquipado) usarMachete(juego);
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_START) juego->estado = ESTADO_PAUSADO;
        }
        if (e.type == SDL_EVENT_GAMEPAD_ADDED && !juego->gamepad)
            juego->gamepad = SDL_OpenGamepad(e.gdevice.which);
        if (e.type == SDL_EVENT_GAMEPAD_REMOVED && juego->gamepad) {
            SDL_CloseGamepad(juego->gamepad); juego->gamepad = nullptr;
        }
    }
}

// ============================================
// Actualizacion de posicion del jugador
// ============================================
void actualizarJugador(Jugador* jugador, SDL_Gamepad* gamepad) {
    const bool* teclas = SDL_GetKeyboardState(NULL);
    if (teclas[SDL_SCANCODE_W]) jugador->rect.y -= (float)jugador->velocidad;
    if (teclas[SDL_SCANCODE_S]) jugador->rect.y += (float)jugador->velocidad;
    if (teclas[SDL_SCANCODE_A]) jugador->rect.x -= (float)jugador->velocidad;
    if (teclas[SDL_SCANCODE_D]) jugador->rect.x += (float)jugador->velocidad;
    if (gamepad) {
        const int DZ = 8000;
        int ax = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
        int ay = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);
        if (ax >  DZ) jugador->rect.x += (float)jugador->velocidad;
        if (ax < -DZ) jugador->rect.x -= (float)jugador->velocidad;
        if (ay >  DZ) jugador->rect.y += (float)jugador->velocidad;
        if (ay < -DZ) jugador->rect.y -= (float)jugador->velocidad;
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP))    jugador->rect.y -= (float)jugador->velocidad;
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN))  jugador->rect.y += (float)jugador->velocidad;
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT))  jugador->rect.x -= (float)jugador->velocidad;
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT)) jugador->rect.x += (float)jugador->velocidad;
    }
    if (jugador->rect.x < 0.0f) jugador->rect.x = 0.0f;
    if (jugador->rect.x > ANCHO_VENTANA - TAMANO_SPRITE) jugador->rect.x = (float)(ANCHO_VENTANA - TAMANO_SPRITE);
    if (jugador->rect.y < 0.0f) jugador->rect.y = 0.0f;
    if (jugador->rect.y > ALTO_VENTANA - TAMANO_SPRITE)  jugador->rect.y = (float)(ALTO_VENTANA - TAMANO_SPRITE);
}
