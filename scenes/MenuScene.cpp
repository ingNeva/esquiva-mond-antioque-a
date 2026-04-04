#include "MenuScene.h"
#include "../core/Game.h"
#include "../core/AudioManager.h"
#include "../scenes/CountdownScene.h"
#include <string>

void renderizarMenu(Juego* juego) {
    SDL_RenderClear(juego->renderer);
    const int W = VW(juego), H = VH(juego);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color gris     = {130, 130, 130, 255};

    // Titulo: 15% desde arriba, 5% desde la izquierda
    renderizarTexto(juego, "ESQUIVAR BOTELLAS", (int)(W * 0.05f), (int)(H * 0.15f), amarillo);

    const char* opciones[]  = {"JUGAR", "SELECCIONAR NIVEL", "INSTRUCCIONES", "OPCIONES", "SALIR"};
    const int totalOpciones = 5;
    // Opciones: empieza al 30% vertical, espaciado 8% de altura
    const int inicioY   = (int)(H * 0.30f);
    const int espaciado = (int)(H * 0.08f);

    for (int i = 0; i < totalOpciones; i++) {
        SDL_Color color = (juego->opcionMenuSeleccionada == i) ? amarillo : blanco;
        std::string linea = (juego->opcionMenuSeleccionada == i)
            ? std::string("> ") + opciones[i]
            : std::string("  ") + opciones[i];
        renderizarTexto(juego, linea.c_str(), (int)(W * 0.05f), inicioY + i * espaciado, color);
    }

    // Pie de pagina — bien pegado al borde inferior
    renderizarTextoPequeno(juego,
        "Flechas/DPad: navegar    Enter/Cruz: seleccionar",
        (int)(W * 0.05f), H - (int)(H * 0.05f), gris);

    SDL_Color colorAudio = juego->musicaActiva
        ? (SDL_Color){80, 255, 120, 255}
        : (SDL_Color){180, 180, 180, 255};
    char textoAudio[48];
    SDL_snprintf(textoAudio, sizeof(textoAudio),
        juego->musicaActiva ? "M: Vol %d%%" : "M: SIN AUDIO",
        juego->volumenMusica * 100 / 128);
    renderizarTextoPequeno(juego, textoAudio, (int)(W * 0.05f), H - (int)(H * 0.09f), colorAudio);

    char txtRes[32];
    SDL_snprintf(txtRes, sizeof(txtRes), "Res: %s%s",
        RESOLUCIONES[juego->resolucionSeleccionada].etiqueta,
        juego->pantallaCompleta ? " [FS]" : "");
    renderizarTextoPequeno(juego, txtRes, (int)(W * 0.05f), H - (int)(H * 0.13f), gris);

    // Linea divisoria central
    SDL_SetRenderDrawColor(juego->renderer, 70, 70, 70, 255);
    SDL_RenderLine(juego->renderer,
        (float)(W / 2), (float)(H * 0.07f),
        (float)(W / 2), (float)(H * 0.93f));

    // Top5 en mitad derecha
    renderizarTop5(juego, (int)(W * 0.53f), (int)(H * 0.09f), -1);

    SDL_RenderPresent(juego->renderer);
}

void manejarEventosMenu(Juego* juego) {
    const int totalOpciones = 5;
    const int H = VH(juego);
    const int inicioY   = (int)(H * 0.30f);
    const int espaciado = (int)(H * 0.08f);
    const int altoFila  = espaciado - 4;

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }
        if (e.type == SDL_EVENT_KEY_DOWN) {
            switch (e.key.key) {
                case SDLK_UP:
                    juego->opcionMenuSeleccionada =
                        (juego->opcionMenuSeleccionada - 1 + totalOpciones) % totalOpciones; break;
                case SDLK_DOWN:
                    juego->opcionMenuSeleccionada =
                        (juego->opcionMenuSeleccionada + 1) % totalOpciones; break;
                case SDLK_RETURN: case SDLK_KP_ENTER:
                    switch (juego->opcionMenuSeleccionada) {
                        case 0: iniciarCuentaRegresiva(juego);        break;
                        case 1:
                            // Sincronizar cursor al nivel actual
                            juego->opcionLevelSelectSeleccionada = juego->nivelActual - 1;
                            if (juego->opcionLevelSelectSeleccionada < 0)
                                juego->opcionLevelSelectSeleccionada = 0;
                            juego->estado = ESTADO_SELECCION_NIVEL;
                            break;
                        case 2: juego->estado = ESTADO_INSTRUCCIONES; break;
                        case 3: juego->estado = ESTADO_OPCIONES;      break;
                        case 4: juego->ejecutando = false;            break;
                    }
                    break;
                case SDLK_M:    toggleMusicaMute(juego); break;
                case SDLK_EQUALS: case SDLK_PLUS: ajustarVolumen(juego,  16); break;
                case SDLK_MINUS:                  ajustarVolumen(juego, -16); break;
                default: break;
            }
        }
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            switch (e.gbutton.button) {
                case SDL_GAMEPAD_BUTTON_DPAD_UP:
                    juego->opcionMenuSeleccionada =
                        (juego->opcionMenuSeleccionada - 1 + totalOpciones) % totalOpciones; break;
                case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
                    juego->opcionMenuSeleccionada =
                        (juego->opcionMenuSeleccionada + 1) % totalOpciones; break;
                case SDL_GAMEPAD_BUTTON_SOUTH:
                    switch (juego->opcionMenuSeleccionada) {
                        case 0: iniciarCuentaRegresiva(juego);        break;
                        case 1:
                            juego->opcionLevelSelectSeleccionada = juego->nivelActual - 1;
                            if (juego->opcionLevelSelectSeleccionada < 0)
                                juego->opcionLevelSelectSeleccionada = 0;
                            juego->estado = ESTADO_SELECCION_NIVEL;
                            break;
                        case 2: juego->estado = ESTADO_INSTRUCCIONES; break;
                        case 3: juego->estado = ESTADO_OPCIONES;      break;
                        case 4: juego->ejecutando = false;            break;
                    }
                    break;
                default: break;
            }
        }
        if (e.type == SDL_EVENT_GAMEPAD_ADDED && !juego->gamepad)
            juego->gamepad = SDL_OpenGamepad(e.gdevice.which);
        if (e.type == SDL_EVENT_GAMEPAD_REMOVED && juego->gamepad) {
            SDL_CloseGamepad(juego->gamepad); juego->gamepad = nullptr;
        }
        if (e.type == SDL_EVENT_MOUSE_MOTION) {
            float my = e.motion.y;
            for (int i = 0; i < totalOpciones; i++) {
                float fy = (float)(inicioY + i * espaciado);
                if (my >= fy && my <= fy + altoFila) juego->opcionMenuSeleccionada = i;
            }
        }
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
            float my = e.button.y;
            for (int i = 0; i < totalOpciones; i++) {
                float fy = (float)(inicioY + i * espaciado);
                if (my >= fy && my <= fy + altoFila) {
                    switch (i) {
                        case 0: iniciarCuentaRegresiva(juego);        break;
                        case 1:
                            juego->opcionLevelSelectSeleccionada = juego->nivelActual - 1;
                            if (juego->opcionLevelSelectSeleccionada < 0)
                                juego->opcionLevelSelectSeleccionada = 0;
                            juego->estado = ESTADO_SELECCION_NIVEL;
                            break;
                        case 2: juego->estado = ESTADO_INSTRUCCIONES; break;
                        case 3: juego->estado = ESTADO_OPCIONES;      break;
                        case 4: juego->ejecutando = false;            break;
                    }
                }
            }
        }
        if (e.type == SDL_EVENT_WINDOW_RESIZED) {
            recargarFuentes(juego);
        }
    }
}
