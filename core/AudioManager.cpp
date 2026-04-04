#include "AudioManager.h"

// ============================================
// Inicializacion
// ============================================
bool inicializarAudio(Juego* juego) {
    if (!MIX_Init()) {
        SDL_Log("MIX_Init fallo: %s", SDL_GetError());
        juego->musicaActiva = false;
        juego->pistaSonando = PISTA_NINGUNA;
        return true;
    }
    juego->mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!juego->mixer) {
        SDL_Log("Sin audio: %s", SDL_GetError());
        juego->musicaActiva = false;
        juego->pistaSonando = PISTA_NINGUNA;
        return true;
    }
    juego->trackMusica      = MIX_CreateTrack(juego->mixer);
    juego->musicaMenu       = MIX_LoadAudio(juego->mixer, RUTA_MUSICA_MENU,       false);
    juego->musicaPausa      = MIX_LoadAudio(juego->mixer, RUTA_MUSICA_PAUSA,      false);
    juego->musicaNiveles123 = MIX_LoadAudio(juego->mixer, RUTA_MUSICA_NIVELES123, false);
    juego->musicaNivel4     = MIX_LoadAudio(juego->mixer, RUTA_MUSICA_NIVEL4,     false);
    juego->musicaNivel5     = MIX_LoadAudio(juego->mixer, RUTA_MUSICA_NIVEL5,     false);
    juego->musicaGameOver   = MIX_LoadAudio(juego->mixer, RUTA_MUSICA_GAMEOVER,   false);
    juego->musicaActiva  = true;
    juego->volumenMusica = VOLUMEN_MUSICA_DEFAULT;
    juego->pistaSonando  = PISTA_NINGUNA;
    MIX_SetTrackGain(juego->trackMusica, juego->volumenMusica / 128.0f);
    return true;
}

// ============================================
// Logica de pista segun estado
// ============================================
EstadoPista pistaSegunEstadoJuego(Juego* juego) {
    switch (juego->estado) {
        case ESTADO_MENU:
        case ESTADO_INSTRUCCIONES:
        case ESTADO_INGRESANDO_NOMBRE:
        case ESTADO_OPCIONES:
            return PISTA_MENU;
        case ESTADO_PAUSADO:
            return PISTA_PAUSA;
        case ESTADO_GAME_OVER:
            if (juego->gameOverReproducido) return PISTA_MENU;
            return PISTA_GAMEOVER;
        case ESTADO_JUGANDO: {
            if (juego->nivelActual <= 3) return PISTA_NIVELES123;
            if (juego->nivelActual == 4) {
            if (!juego->nivel4Reproducido) return PISTA_NIVEL4;
        return PISTA_NIVEL5;
    }
    return PISTA_NIVEL5;
}
        case ESTADO_TRANSICION_NIVEL:
            return PISTA_NINGUNA;
        case ESTADO_CUENTA_REGRESIVA:
            return PISTA_MENU;
        case ESTADO_VICTORIA:
            return PISTA_NINGUNA;
        default:
            return PISTA_MENU;
    }
}

// ============================================
// Reproduccion
// ============================================
void reproducirMusica(Juego* juego, EstadoPista pista) {
    if (!juego->musicaActiva) return;
    if (juego->pistaSonando == pista) return;

    MIX_Audio* objetivo  = nullptr;
    bool       loopInfinito = true;
    switch (pista) {
        case PISTA_MENU:       objetivo = juego->musicaMenu;       loopInfinito = true;  break;
        case PISTA_PAUSA:      objetivo = juego->musicaPausa;      loopInfinito = true;  break;
        case PISTA_NIVELES123: objetivo = juego->musicaNiveles123; loopInfinito = true;  break;
        case PISTA_NIVEL4:     objetivo = juego->musicaNivel4;     loopInfinito = false; break;
        case PISTA_NIVEL5:     objetivo = juego->musicaNivel5;     loopInfinito = true;  break;
        case PISTA_GAMEOVER:   objetivo = juego->musicaGameOver;   loopInfinito = false; break;
        default: return;
    }
    if (!objetivo) {
        juego->pistaSonando = pista;
        if (pista == PISTA_NIVEL4)   juego->nivel4Reproducido  = true;
        if (pista == PISTA_GAMEOVER) juego->gameOverReproducido = true;
        return;
    }
    MIX_StopTrack(juego->trackMusica, 0);
    MIX_SetTrackAudio(juego->trackMusica, objetivo);
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, loopInfinito ? -1 : 0);
    MIX_PlayTrack(juego->trackMusica, props);
    SDL_DestroyProperties(props);
    juego->pistaSonando = pista;
    if (pista == PISTA_NIVEL4)   juego->nivel4Reproducido  = true;
    if (pista == PISTA_GAMEOVER) juego->gameOverReproducido = true;
}

// ============================================
// Controles de volumen
// ============================================
void toggleMusicaMute(Juego* juego) {
    juego->musicaActiva = !juego->musicaActiva;
    if (juego->musicaActiva) {
        MIX_SetTrackGain(juego->trackMusica, juego->volumenMusica / 128.0f);
        juego->pistaSonando = PISTA_NINGUNA;
    } else {
        MIX_StopTrack(juego->trackMusica, 0);
        juego->pistaSonando = PISTA_NINGUNA;
    }
}

void ajustarVolumen(Juego* juego, int delta) {
    juego->volumenMusica = SDL_clamp(juego->volumenMusica + delta, 0, 128);
    if (juego->musicaActiva)
        MIX_SetTrackGain(juego->trackMusica, juego->volumenMusica / 128.0f);
}

// ============================================
// Limpieza
// ============================================
void limpiarAudio(Juego* juego) {
    if (juego->trackMusica)      MIX_DestroyTrack(juego->trackMusica);
    if (juego->musicaMenu)       MIX_DestroyAudio(juego->musicaMenu);
    if (juego->musicaPausa)      MIX_DestroyAudio(juego->musicaPausa);
    if (juego->musicaNiveles123) MIX_DestroyAudio(juego->musicaNiveles123);
    if (juego->musicaNivel4)     MIX_DestroyAudio(juego->musicaNivel4);
    if (juego->musicaNivel5)     MIX_DestroyAudio(juego->musicaNivel5);
    if (juego->musicaGameOver)   MIX_DestroyAudio(juego->musicaGameOver);
    if (juego->mixer)            MIX_DestroyMixer(juego->mixer);
    MIX_Quit();
}
