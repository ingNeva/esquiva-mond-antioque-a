#include "Game.h"
#include "AudioManager.h"
#include "../utils/ScoreManager.h"
#include "../entities/Player.h"
#include "../entities/Enemy.h"
#include "../entities/Machete.h"
#include "../scenes/CountdownScene.h"

#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>

// ============================================
// Configuracion persistente
// ============================================
struct ConfigGuardada {
    int  magic;                // 0xEB0C para validar
    int  resolucionSeleccionada;
    bool pantallaCompleta;
    bool musicaActiva;
    int  volumenMusica;
};
static const int CONFIG_MAGIC = 0xEB0C;

void guardarConfig(const Juego* juego) {
    crearDirectorioSaves();
    FILE* f = fopen(RUTA_CONFIG, "wb");
    if (!f) return;
    ConfigGuardada cfg;
    cfg.magic                  = CONFIG_MAGIC;
    cfg.resolucionSeleccionada = juego->resolucionSeleccionada;
    cfg.pantallaCompleta       = juego->pantallaCompleta;
    cfg.musicaActiva           = juego->musicaActiva;
    cfg.volumenMusica          = juego->volumenMusica;
    fwrite(&cfg, sizeof(cfg), 1, f);
    fclose(f);
}

void cargarConfig(Juego* juego) {
    FILE* f = fopen(RUTA_CONFIG, "rb");
    if (!f) {
        // Primera ejecucion: pantalla completa por defecto
        juego->pantallaCompleta       = true;
        juego->resolucionSeleccionada = 12; // 1920x1080
        juego->musicaActiva           = true;
        juego->volumenMusica          = VOLUMEN_MUSICA_DEFAULT;
        return;
    }
    ConfigGuardada cfg = {};
    fread(&cfg, sizeof(cfg), 1, f);
    fclose(f);
    if (cfg.magic != CONFIG_MAGIC) {
        juego->pantallaCompleta       = true;
        juego->resolucionSeleccionada = 12;
        juego->musicaActiva           = true;
        juego->volumenMusica          = VOLUMEN_MUSICA_DEFAULT;
        return;
    }
    juego->resolucionSeleccionada = SDL_clamp(cfg.resolucionSeleccionada, 0, NUM_RESOLUCIONES - 1);
    juego->pantallaCompleta       = cfg.pantallaCompleta;
    juego->musicaActiva           = cfg.musicaActiva;
    juego->volumenMusica          = SDL_clamp(cfg.volumenMusica, 0, 128);
}

// ============================================
// Nivel actual (por puntuacion — solo para compatibilidad legacy)
// ============================================
int nivelActual(int puntuacion) {
    if (puntuacion >= UMBRAL_NIVEL_5) return 5;
    if (puntuacion >= UMBRAL_NIVEL_4) return 4;
    if (puntuacion >= UMBRAL_NIVEL_3) return 3;
    if (puntuacion >= UMBRAL_NIVEL_2) return 2;
    return 1;
}

// ============================================
// Inicializacion SDL
// ============================================
bool inicializarSDL(Juego* juego) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
        SDL_Log("Error SDL: %s", SDL_GetError());
        return false;
    }
    juego->ventana = SDL_CreateWindow("Esquivar Botellas", ANCHO_VENTANA, ALTO_VENTANA, 0);
    if (!juego->ventana) { SDL_Log("Error ventana: %s", SDL_GetError()); return false; }

    // Aplicar resolucion y modo de pantalla guardados
    if (juego->pantallaCompleta) {
        SDL_SetWindowFullscreen(juego->ventana, true);
    } else {
        int w = RESOLUCIONES[juego->resolucionSeleccionada].ancho;
        int h = RESOLUCIONES[juego->resolucionSeleccionada].alto;
        SDL_SetWindowSize(juego->ventana, w, h);
        SDL_SetWindowPosition(juego->ventana, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
    juego->renderer = SDL_CreateRenderer(juego->ventana, NULL);
    if (!juego->renderer) { SDL_Log("Error renderer: %s", SDL_GetError()); return false; }
    if (!TTF_Init()) { SDL_Log("Error TTF: %s", SDL_GetError()); return false; }
    inicializarAudio(juego);
    juego->gamepad = nullptr;
    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    if (ids) {
        for (int i = 0; i < count; i++) {
            if (SDL_IsGamepad(ids[i])) {
                juego->gamepad = SDL_OpenGamepad(ids[i]);
                if (juego->gamepad) { SDL_Log("Gamepad: %s", SDL_GetGamepadName(juego->gamepad)); break; }
            }
        }
        SDL_free(ids);
    }
    return true;
}

// ============================================
// Carga de texturas
// ============================================
bool cargarTexturas(Juego* juego) {
    const char* rutasFondos[5] = {
        RUTA_FONDO_NIVEL1, RUTA_FONDO_NIVEL2, RUTA_FONDO_NIVEL3,
        RUTA_FONDO_NIVEL4, RUTA_FONDO_NIVEL5
    };
    for (int i = 0; i < 5; i++) {
        juego->texFondos[i] = IMG_LoadTexture(juego->renderer, rutasFondos[i]);
        if (!juego->texFondos[i])
            SDL_Log("Advertencia: no se cargo fondo nivel %d (%s): %s", i+1, rutasFondos[i], SDL_GetError());
    }
    juego->texPlayerRight = IMG_LoadTexture(juego->renderer, "imagenes/player_walk_right.png");
    juego->texPlayerLeft  = IMG_LoadTexture(juego->renderer, "imagenes/player_walk_left.png");
    juego->texPlayerDown  = IMG_LoadTexture(juego->renderer, "imagenes/player_walk_down.png");
    juego->texPlayerUp    = IMG_LoadTexture(juego->renderer, "imagenes/player_walk_up.png");
    juego->texJugador     = juego->texPlayerDown;
    juego->texEnemigo = IMG_LoadTexture(juego->renderer, "imagenes/enemy.png");
    juego->texMachete = IMG_LoadTexture(juego->renderer, "imagenes/machete.png");

    auto loadFB = [&](const char* ruta) -> SDL_Texture* {
        SDL_Texture* t = IMG_LoadTexture(juego->renderer, ruta);
        if (!t) SDL_Log("Tex opcional no encontrada: %s", ruta);
        return t;
    };
    juego->texEnemigoRapido     = loadFB("imagenes/enemy_mini.png");
    juego->texEnemigoTanque     = loadFB("imagenes/enemy_boss.png");
    juego->texEnemigoZigzag     = loadFB("imagenes/enemy_azul.png");
    juego->texEnemigoBombardero = loadFB("imagenes/enemy_roja.png");
    juego->texEnemigoEspejo     = loadFB("imagenes/enemy_maxima.png");
    if (!juego->texEnemigoRapido)     juego->texEnemigoRapido     = juego->texEnemigo;
    if (!juego->texEnemigoTanque)     juego->texEnemigoTanque     = juego->texEnemigo;
    if (!juego->texEnemigoZigzag)     juego->texEnemigoZigzag     = juego->texEnemigo;
    if (!juego->texEnemigoBombardero) juego->texEnemigoBombardero = juego->texEnemigo;
    if (!juego->texEnemigoEspejo)     juego->texEnemigoEspejo     = juego->texEnemigo;

    juego->texBoss   = loadFB("imagenes/enemy_boss.png");
    juego->texTrofeo = loadFB("imagenes/trofeo.png");
    juego->texPilar  = loadFB("imagenes/pilar.png");
    if (!juego->texBoss)   juego->texBoss   = juego->texEnemigo;
    if (!juego->texTrofeo) juego->texTrofeo = juego->texEnemigo;
    if (!juego->texPilar)  juego->texPilar  = juego->texEnemigo;

    juego->texLlave = loadFB("imagenes/llave.png");
    if (!juego->texLlave) juego->texLlave = juego->texEnemigo;

    if (juego->mixer)
        juego->musicaVictoria = MIX_LoadAudio(juego->mixer, "musica/victoria.wav", false);

    return (juego->texJugador && juego->texEnemigo && juego->texMachete);
}

// ============================================
// Carga de fuente — tamaño escalado a la resolucion actual
// ============================================
bool cargarFuente(Juego* juego) {
    int sz  = tamanoFuente(juego);
    int szP = tamanoFuentePequena(juego);

    juego->fuente = TTF_OpenFont("Arial Black.ttf", sz);
    if (!juego->fuente) { SDL_Log("Error fuente: %s", SDL_GetError()); return false; }

    juego->fuentePequena = TTF_OpenFont("Arial Black.ttf", szP);
    if (!juego->fuentePequena) juego->fuentePequena = juego->fuente;

    return true;
}

// ============================================
// Recargar fuentes tras cambio de resolucion/fullscreen
// ============================================
void recargarFuentes(Juego* juego) {
    if (juego->fuentePequena && juego->fuentePequena != juego->fuente)
        TTF_CloseFont(juego->fuentePequena);
    if (juego->fuente)
        TTF_CloseFont(juego->fuente);
    juego->fuente        = nullptr;
    juego->fuentePequena = nullptr;
    cargarFuente(juego);
}

// ============================================
// Helpers de texto — posicion absoluta
// ============================================
void renderizarTexto(Juego* juego, const char* texto, int x, int y, SDL_Color color) {
    SDL_Surface* sup = TTF_RenderText_Solid(juego->fuente, texto, 0, color);
    if (!sup) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(juego->renderer, sup);
    if (tex) {
        SDL_FRect dst = {(float)x, (float)y, (float)sup->w, (float)sup->h};
        SDL_RenderTexture(juego->renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_DestroySurface(sup);
}

void renderizarTextoPequeno(Juego* juego, const char* texto, int x, int y, SDL_Color color) {
    SDL_Surface* sup = TTF_RenderText_Solid(juego->fuentePequena, texto, 0, color);
    if (!sup) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(juego->renderer, sup);
    if (tex) {
        SDL_FRect dst = {(float)x, (float)y, (float)sup->w, (float)sup->h};
        SDL_RenderTexture(juego->renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_DestroySurface(sup);
}

// ============================================
// Helpers de texto — centrado horizontal automatico
// ============================================
void renderizarTextoCentrado(Juego* juego, const char* texto, int y, SDL_Color color) {
    SDL_Surface* sup = TTF_RenderText_Solid(juego->fuente, texto, 0, color);
    if (!sup) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(juego->renderer, sup);
    if (tex) {
        int x = (VW(juego) - sup->w) / 2;
        SDL_FRect dst = {(float)x, (float)y, (float)sup->w, (float)sup->h};
        SDL_RenderTexture(juego->renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_DestroySurface(sup);
}

void renderizarTextoPequenoC(Juego* juego, const char* texto, int y, SDL_Color color) {
    SDL_Surface* sup = TTF_RenderText_Solid(juego->fuentePequena, texto, 0, color);
    if (!sup) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(juego->renderer, sup);
    if (tex) {
        int x = (VW(juego) - sup->w) / 2;
        SDL_FRect dst = {(float)x, (float)y, (float)sup->w, (float)sup->h};
        SDL_RenderTexture(juego->renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_DestroySurface(sup);
}

// ============================================
// Top 5
// ============================================
void renderizarTop5(Juego* juego, int x, int y, int posicionResaltada) {
    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color gris     = {130, 130, 130, 255};
    SDL_Color dorado   = {255, 180,   0, 255};
    SDL_Color verde    = { 80, 255, 120, 255};
    const char* medallas[] = {"#1", "#2", "#3", "#4", "#5"};
    // Espaciado proporcional al alto de ventana
    const int espaciado = SDL_max(18, (int)(VH(juego) * 0.055f));
    renderizarTextoPequeno(juego, "--- TOP 5 ---", x, y, amarillo);
    if (juego->tablaPuntajes.cantidad == 0) {
        renderizarTextoPequeno(juego, "Sin records aun", x, y + espaciado, gris);
        return;
    }
    for (int i = 0; i < MAX_PUNTAJES; i++) {
        char linea[72];
        SDL_Color color;
        if (i < juego->tablaPuntajes.cantidad) {
            if (i == posicionResaltada)   color = verde;
            else if (i == 0)              color = dorado;
            else                          color = blanco;
            SDL_snprintf(linea, sizeof(linea), "%s %-14s %4d",
                medallas[i],
                juego->tablaPuntajes.entradas[i].nombre,
                juego->tablaPuntajes.entradas[i].puntuacion);
        } else {
            color = gris;
            SDL_snprintf(linea, sizeof(linea), "%s ---", medallas[i]);
        }
        renderizarTextoPequeno(juego, linea, x, y + (i + 1) * espaciado, color);
    }
}

// ============================================
// Limpieza de recursos
// ============================================
void limpiarRecursos(Juego* juego) {
    limpiarAudio(juego);
    if (juego->gamepad) SDL_CloseGamepad(juego->gamepad);
    // Spritesheets jugador (los 4 son independientes)
    if (juego->texPlayerRight) SDL_DestroyTexture(juego->texPlayerRight);
    if (juego->texPlayerLeft)  SDL_DestroyTexture(juego->texPlayerLeft);
    if (juego->texPlayerDown)  SDL_DestroyTexture(juego->texPlayerDown);
    if (juego->texPlayerUp)    SDL_DestroyTexture(juego->texPlayerUp);
    // texJugador es alias de texPlayerDown, NO destruir por separado
    if (juego->texEnemigo) SDL_DestroyTexture(juego->texEnemigo);
    if (juego->texMachete) SDL_DestroyTexture(juego->texMachete);
    if (juego->texEnemigo) SDL_DestroyTexture(juego->texEnemigo);
    if (juego->texMachete) SDL_DestroyTexture(juego->texMachete);
    if (juego->texEnemigoRapido     && juego->texEnemigoRapido     != juego->texEnemigo) SDL_DestroyTexture(juego->texEnemigoRapido);
    if (juego->texEnemigoTanque     && juego->texEnemigoTanque     != juego->texEnemigo) SDL_DestroyTexture(juego->texEnemigoTanque);
    if (juego->texEnemigoZigzag     && juego->texEnemigoZigzag     != juego->texEnemigo) SDL_DestroyTexture(juego->texEnemigoZigzag);
    if (juego->texEnemigoBombardero && juego->texEnemigoBombardero != juego->texEnemigo) SDL_DestroyTexture(juego->texEnemigoBombardero);
    if (juego->texEnemigoEspejo     && juego->texEnemigoEspejo     != juego->texEnemigo) SDL_DestroyTexture(juego->texEnemigoEspejo);
    if (juego->texBoss   && juego->texBoss   != juego->texEnemigo) SDL_DestroyTexture(juego->texBoss);
    if (juego->texTrofeo && juego->texTrofeo != juego->texEnemigo) SDL_DestroyTexture(juego->texTrofeo);
    if (juego->texPilar  && juego->texPilar  != juego->texEnemigo) SDL_DestroyTexture(juego->texPilar);
    if (juego->texLlave  && juego->texLlave  != juego->texEnemigo) SDL_DestroyTexture(juego->texLlave);
    if (juego->musicaVictoria) MIX_DestroyAudio(juego->musicaVictoria);
    for (int i = 0; i < 5; i++) if (juego->texFondos[i]) SDL_DestroyTexture(juego->texFondos[i]);
    if (juego->fuentePequena && juego->fuentePequena != juego->fuente) TTF_CloseFont(juego->fuentePequena);
    if (juego->fuente)   TTF_CloseFont(juego->fuente);
    if (juego->renderer) SDL_DestroyRenderer(juego->renderer);
    if (juego->ventana)  SDL_DestroyWindow(juego->ventana);
    TTF_Quit();
    SDL_Quit();
}

// ============================================
// Reinicio
// ============================================
void reiniciarJuego(Juego* juego) {
    inicializarJugador(&juego->jugador);
    inicializarEnemigos(juego);
    inicializarMachete(juego);
    juego->ultimoNivelDificultad = 0;
    juego->posicionNuevoPuntaje  = -1;
    juego->pistaSonando          = PISTA_NINGUNA;
    juego->nivel4Reproducido     = false;
    juego->estadoBoss            = BOSS_INACTIVO;
    juego->bossHP                = 0;
    juego->bossSpawneado         = false;
    juego->trofeoActivo          = false;
    juego->pilaresActivos        = 0;
    for (int p = 0; p < MAX_PILARES; p++) juego->pilares[p].activo = false;
    juego->combo = 0; juego->mejorCombo = 0; juego->multiplicador = 1.0f;
    for (int f = 0; f < MAX_FLOATING_TEXT; f++) juego->floatingTexts[f].activo = false;
    juego->llave.activa     = false;
    juego->llave.pulsoTimer = 0.0f;
    juego->gameOverReproducido = false;
    juego->transicion = {};

    // El machete y la intro se gestionan dentro de iniciarIntro()
    // segun juego->nivelActual, no hay que tocarlo aqui.
    juego->macheteEquipado  = false;
    juego->macheteAparecido = false;
    juego->machete.recogido = false;

    // Lanzar la intro cinematica (el personaje caminando)
    // iniciarIntro() se declara en CountdownScene.h
    iniciarIntro(juego);
}