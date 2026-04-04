#include "OptionsScene.h"
#include "../core/Game.h"
#include "../core/AudioManager.h"
#include "../utils/ScoreManager.h"

void aplicarResolucion(Juego* juego) {
    int w = RESOLUCIONES[juego->resolucionSeleccionada].ancho;
    int h = RESOLUCIONES[juego->resolucionSeleccionada].alto;
    SDL_SetWindowSize(juego->ventana, w, h);
    SDL_SetWindowPosition(juego->ventana, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void togglePantallaCompleta(Juego* juego) {
    juego->pantallaCompleta = !juego->pantallaCompleta;
    if (juego->pantallaCompleta) {
        SDL_SetWindowFullscreen(juego->ventana, true);
    } else {
        SDL_SetWindowFullscreen(juego->ventana, false);
        aplicarResolucion(juego);
    }
}

void renderizarOpciones(Juego* juego) {
    SDL_RenderClear(juego->renderer);
    const int W = VW(juego), H = VH(juego);
    const int cx     = W / 2;
    // Columnas relativas
    const int labelX = cx - (int)(W * 0.22f);
    const int valorX = cx + (int)(W * 0.02f);
    // Filas relativas: inicio al 15% de H, paso 8%
    const int inicioY = (int)(H * 0.15f);
    const int paso    = (int)(H * 0.08f);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color gris     = {130, 130, 130, 255};
    SDL_Color verde    = { 80, 255, 120, 255};
    SDL_Color apagado  = {100, 100, 100, 255};

    renderizarTexto(juego, "OPCIONES", cx - (int)(W*0.06f), (int)(H*0.06f), amarillo);

    SDL_Color seccion = {180, 160, 50, 255};
    renderizarTextoPequeno(juego, "AUDIO",    labelX, inicioY - (int)(H*0.03f), seccion);
    renderizarTextoPequeno(juego, "PANTALLA", labelX, inicioY + 2*paso - (int)(H*0.03f), seccion);

    const char* nombres[] = {"Musica", "Volumen", "Resolucion", "Pantalla completa"};
    const int totalOpciones = 4;

    for (int i = 0; i < totalOpciones; i++) {
        bool seleccionada = (juego->opcionOpcionesSeleccionada == i);
        SDL_Color colorNombre = seleccionada ? amarillo : blanco;
        char linea[64];
        SDL_snprintf(linea, sizeof(linea), "%s%s", seleccionada ? "> " : "  ", nombres[i]);
        renderizarTexto(juego, linea, labelX, inicioY + i * paso, colorNombre);

        char valor[64] = {};
        SDL_Color colorValor = blanco;
        switch (i) {
            case 0:
                SDL_snprintf(valor, sizeof(valor), juego->musicaActiva ? "[ ON ]" : "[ OFF ]");
                colorValor = juego->musicaActiva ? verde : apagado; break;
            case 1:
                SDL_snprintf(valor, sizeof(valor), "%3d%%", juego->volumenMusica * 100 / 128);
                colorValor = (juego->volumenMusica > 0) ? blanco : apagado; break;
            case 2:
                SDL_snprintf(valor, sizeof(valor), "< %s >",
                    RESOLUCIONES[juego->resolucionSeleccionada].etiqueta);
                colorValor = amarillo; break;
            case 3:
                SDL_snprintf(valor, sizeof(valor), juego->pantallaCompleta ? "[ ON ]" : "[ OFF ]");
                colorValor = juego->pantallaCompleta ? verde : apagado; break;
        }
        renderizarTexto(juego, valor, valorX, inicioY + i * paso, colorValor);

        // Barra grafica de volumen
        if (i == 1) {
            const int barX = valorX + (int)(W * 0.06f);
            const int barY = inicioY + i * paso + (int)(H * 0.01f);
            const int barW = (int)(W * 0.14f), barH = (int)(H * 0.014f);
            float frac = juego->volumenMusica / 128.0f;
            SDL_SetRenderDrawColor(juego->renderer, 40, 40, 40, 255);
            SDL_FRect bg = {(float)barX, (float)barY, (float)barW, (float)barH};
            SDL_RenderFillRect(juego->renderer, &bg);
            Uint8 br = (Uint8)(255 * (1.0f - frac));
            Uint8 bg2 = (Uint8)(200 * frac);
            SDL_SetRenderDrawColor(juego->renderer, br, bg2, 0, 255);
            SDL_FRect fill = {(float)barX, (float)barY, barW * frac, (float)barH};
            SDL_RenderFillRect(juego->renderer, &fill);
            SDL_SetRenderDrawColor(juego->renderer, 100, 100, 100, 255);
            SDL_RenderRect(juego->renderer, &bg);
        }
    }

    int sepY = inicioY + totalOpciones * paso + (int)(H * 0.01f);
    SDL_SetRenderDrawColor(juego->renderer, 60, 60, 60, 255);
    SDL_RenderLine(juego->renderer,
        (float)(cx - (int)(W*0.22f)), (float)sepY,
        (float)(cx + (int)(W*0.22f)), (float)sepY);
    renderizarTextoPequeno(juego,
        "Arriba/Abajo: navegar     Izq/Der o +/-: cambiar valor     F: fullscreen",
        cx - (int)(W*0.21f), sepY + (int)(H*0.02f), gris);
    renderizarTextoPequeno(juego, "ESC / Circulo: volver al menu",
        cx - (int)(W*0.08f), sepY + (int)(H*0.05f), gris);
    if (juego->pantallaCompleta)
        renderizarTextoPequeno(juego, "En pantalla completa la resolucion la fija el SO",
            cx - (int)(W*0.15f), sepY + (int)(H*0.08f), apagado);
    SDL_RenderPresent(juego->renderer);

    // ── Eventos ──────────────────────────────────
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }
        if (e.type == SDL_EVENT_KEY_DOWN) {
            switch (e.key.key) {
                case SDLK_UP:
                    juego->opcionOpcionesSeleccionada =
                        (juego->opcionOpcionesSeleccionada - 1 + totalOpciones) % totalOpciones; break;
                case SDLK_DOWN:
                    juego->opcionOpcionesSeleccionada =
                        (juego->opcionOpcionesSeleccionada + 1) % totalOpciones; break;
                case SDLK_LEFT: case SDLK_MINUS:
                    switch (juego->opcionOpcionesSeleccionada) {
                        case 0: toggleMusicaMute(juego); break;
                        case 1: ajustarVolumen(juego, -8); break;
                        case 2:
                            juego->resolucionSeleccionada =
                                (juego->resolucionSeleccionada - 1 + NUM_RESOLUCIONES) % NUM_RESOLUCIONES;
                            if (!juego->pantallaCompleta) aplicarResolucion(juego);
                            break;
                        case 3: togglePantallaCompleta(juego); break;
                    }
                    guardarConfig(juego);
                    break;
                case SDLK_RIGHT: case SDLK_EQUALS: case SDLK_PLUS:
                    switch (juego->opcionOpcionesSeleccionada) {
                        case 0: toggleMusicaMute(juego); break;
                        case 1: ajustarVolumen(juego, 8); break;
                        case 2:
                            juego->resolucionSeleccionada =
                                (juego->resolucionSeleccionada + 1) % NUM_RESOLUCIONES;
                            if (!juego->pantallaCompleta) aplicarResolucion(juego);
                            break;
                        case 3: togglePantallaCompleta(juego); break;
                    }
                    guardarConfig(juego);
                    break;
                case SDLK_RETURN: case SDLK_KP_ENTER:
                    switch (juego->opcionOpcionesSeleccionada) {
                        case 0: toggleMusicaMute(juego);       break;
                        case 3: togglePantallaCompleta(juego); break;
                        default: break;
                    }
                    guardarConfig(juego);
                    break;
                case SDLK_F: togglePantallaCompleta(juego); guardarConfig(juego); break;
                case SDLK_M: toggleMusicaMute(juego);       guardarConfig(juego); break;
                case SDLK_ESCAPE: case SDLK_BACKSPACE:
                    juego->estado = ESTADO_MENU; break;
                default: break;
            }
        }
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            switch (e.gbutton.button) {
                case SDL_GAMEPAD_BUTTON_DPAD_UP:
                    juego->opcionOpcionesSeleccionada =
                        (juego->opcionOpcionesSeleccionada - 1 + totalOpciones) % totalOpciones; break;
                case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
                    juego->opcionOpcionesSeleccionada =
                        (juego->opcionOpcionesSeleccionada + 1) % totalOpciones; break;
                case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
                    switch (juego->opcionOpcionesSeleccionada) {
                        case 0: toggleMusicaMute(juego);   break;
                        case 1: ajustarVolumen(juego, -8); break;
                        case 2:
                            juego->resolucionSeleccionada =
                                (juego->resolucionSeleccionada - 1 + NUM_RESOLUCIONES) % NUM_RESOLUCIONES;
                            if (!juego->pantallaCompleta) aplicarResolucion(juego);
                            break;
                        case 3: togglePantallaCompleta(juego); break;
                    }
                    guardarConfig(juego);
                    break;
                case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
                    switch (juego->opcionOpcionesSeleccionada) {
                        case 0: toggleMusicaMute(juego);  break;
                        case 1: ajustarVolumen(juego, 8); break;
                        case 2:
                            juego->resolucionSeleccionada =
                                (juego->resolucionSeleccionada + 1) % NUM_RESOLUCIONES;
                            if (!juego->pantallaCompleta) aplicarResolucion(juego);
                            break;
                        case 3: togglePantallaCompleta(juego); break;
                    }
                    guardarConfig(juego);
                    break;
                case SDL_GAMEPAD_BUTTON_EAST:
                    juego->estado = ESTADO_MENU; break;
                default: break;
            }
        }
    }
}
