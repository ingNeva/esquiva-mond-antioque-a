#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <cmath>
#include <cstdio>
#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
#endif

// ============================================
// Constantes del juego
// ============================================
#define MAX_ENEMIGOS              50
#define ANCHO_VENTANA             1360
#define ALTO_VENTANA              768
#define TAMANO_SPRITE             64
#define COOLDOWN_MACHETE          2000
#define RANGO_ATAQUE              150
#define OFFSET_MACHETE_X          40
#define OFFSET_MACHETE_Y          20
#define BARRA_COOLDOWN_ANCHO      200
#define BARRA_COOLDOWN_ALTO       20
#define BARRA_COOLDOWN_X          10
#define BARRA_COOLDOWN_Y          (ALTO_VENTANA - 80)
#define DURACION_ANIMACION_ATAQUE 300
#define RADIO_GIRO_MACHETE        50
#define MAX_PUNTAJES              5
#define RUTA_SAVES                "saves/puntajes.bin"
#define MAX_NOMBRE                32
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================
// Umbrales de nivel
// ============================================
#define UMBRAL_NIVEL_1   0
#define UMBRAL_NIVEL_2   20
#define UMBRAL_NIVEL_3   40
#define UMBRAL_NIVEL_4   80
#define UMBRAL_NIVEL_5   160

// ============================================
// Duracion de la transicion de nivel (ms)
// ============================================
#define DURACION_TRANSICION       2500

// ============================================
// Audio
// ============================================
#define VOLUMEN_MUSICA_DEFAULT  64

#define RUTA_MUSICA_MENU        "musica/Map (basic version).wav"
#define RUTA_MUSICA_PAUSA       "musica/Map.wav"
#define RUTA_MUSICA_NIVELES123  "musica/Mars.wav"
#define RUTA_MUSICA_NIVEL4      "musica/BossIntro.wav"
#define RUTA_MUSICA_NIVEL5      "musica/BossMain.wav"
#define RUTA_MUSICA_GAMEOVER    "musica/Warp Jingle.wav"

enum EstadoPista {
    PISTA_NINGUNA,
    PISTA_MENU,
    PISTA_PAUSA,
    PISTA_NIVELES123,
    PISTA_NIVEL4,
    PISTA_NIVEL5,
    PISTA_GAMEOVER
};

// ============================================
// ESTRUCTURAS DE PUNTAJES
// ============================================
struct EntradaPuntaje {
    char nombre[MAX_NOMBRE];
    int  puntuacion;
};

struct TablaPuntajes {
    EntradaPuntaje entradas[MAX_PUNTAJES];
    int            cantidad;
};

// ============================================
// ESTADOS DEL JUEGO
// ============================================
enum EstadoJuego {
    ESTADO_MENU,
    ESTADO_JUGANDO,
    ESTADO_INSTRUCCIONES,
    ESTADO_INGRESANDO_NOMBRE,
    ESTADO_GAME_OVER,
    ESTADO_PAUSADO,
    ESTADO_TRANSICION_NIVEL,
    ESTADO_CUENTA_REGRESIVA    // NUEVO: pantalla 3-2-1 antes de jugar
};

// ============================================
// ESTRUCTURAS DEL JUEGO
// ============================================
struct Jugador {
    SDL_FRect rect;
    int velocidad;
};

struct Enemigo {
    SDL_FRect rect;
    float velX;
    float velY;
};

struct Machete {
    SDL_FRect rect;
    bool   recogido;
    bool   activo;
    Uint64 ultimoUso;
    bool   animandoAtaque;
    Uint64 inicioAnimacion;
    float  anguloActual;
};

struct TransicionNivel {
    bool   activa;
    int    nivelNuevo;
    Uint64 inicio;
    EstadoJuego estadoAnterior;
};

// Rutas de los fondos por nivel (1-5)
#define RUTA_FONDO_NIVEL1  "imagenes/bg_nivel1.png"
#define RUTA_FONDO_NIVEL2  "imagenes/bg_nivel2.png"
#define RUTA_FONDO_NIVEL3  "imagenes/bg_nivel3.png"
#define RUTA_FONDO_NIVEL4  "imagenes/bg_nivel4.png"
#define RUTA_FONDO_NIVEL5  "imagenes/bg_nivel5.png"

struct Juego {
    SDL_Window*    ventana;
    SDL_Renderer*  renderer;
    SDL_Texture*   texFondos[5];
    SDL_Texture*   texJugador;
    SDL_Texture*   texEnemigo;
    SDL_Texture*   texMachete;
    TTF_Font*      fuente;
    TTF_Font*      fuentePequena;
    Jugador        jugador;
    Enemigo        enemigos[MAX_ENEMIGOS];
    int            enemigosActivos;
    int            puntuacion;
    bool           ejecutando;
    Machete        machete;
    bool           macheteEquipado;
    EstadoJuego    estado;
    int            opcionMenuSeleccionada;
    bool           macheteAparecido;
    int            ultimoNivelDificultad;
    SDL_Gamepad*   gamepad;
    TablaPuntajes  tablaPuntajes;
    char           nombreIngresado[MAX_NOMBRE];
    int            longitudNombre;
    int            posicionNuevoPuntaje;

    // --- Audio ---
    MIX_Mixer*   mixer;
    MIX_Track*   trackMusica;
    MIX_Audio*   musicaMenu;
    MIX_Audio*   musicaPausa;
    MIX_Audio*   musicaNiveles123;
    MIX_Audio*   musicaNivel4;
    MIX_Audio*   musicaNivel5;
    MIX_Audio*   musicaGameOver;
    bool         musicaActiva;
    int          volumenMusica;
    EstadoPista  pistaSonando;

    bool         nivel4Reproducido;
    bool         gameOverReproducido;

    // Transicion de nivel
    TransicionNivel transicion;

    // --- NUEVOS CAMPOS ---
    Uint64 inicioCuentaRegresiva;
    int    opcionGameOverSel;    // 0=jugar de nuevo, 1=menu, 2=salir
    int    opcionPausaSel;       // 0=continuar, 1=menu, 2=salir
};

// ============================================
// DECLARACION DE FUNCIONES
// ============================================
bool  inicializarSDL(Juego* juego);
bool  cargarTexturas(Juego* juego);
bool  cargarFuente(Juego* juego);
bool  inicializarAudio(Juego* juego);
void  reproducirMusica(Juego* juego, EstadoPista pista);
void  toggleMusicaMute(Juego* juego);
void  ajustarVolumen(Juego* juego, int delta);
void  limpiarAudio(Juego* juego);
EstadoPista pistaSegunEstadoJuego(Juego* juego);
int   nivelActual(int puntuacion);
void  renderizarTexto(Juego* juego, const char* texto, int x, int y, SDL_Color color);
void  renderizarTextoPequeno(Juego* juego, const char* texto, int x, int y, SDL_Color color);
void  renderizarTextoCentrado(Juego* juego, TTF_Font* fuente, const char* texto, int y, SDL_Color color);
void  renderizarTextoCentradoPequeno(Juego* juego, const char* texto, int y, SDL_Color color);
bool  mouseEnRect(int mx, int my, int x, int y, int w, int h);
void  dibujarBoton(Juego* juego, const char* texto, int x, int y, int w, int h, bool seleccionado, SDL_Color colorNormal, SDL_Color colorHover);
void  mostrarPuntuacionPantalla(Juego* juego);
void  inicializarJugador(Jugador* jugador);
void  inicializarMachete(Juego* juego);
void  aparecerMachete(Juego* juego);
void  verificarRecogidaMachete(Juego* juego);
void  usarMachete(Juego* juego);
float calcularProgresoCooldown(Juego* juego);
void  renderizarBarraCooldown(Juego* juego);
void  actualizarPosicionMacheteEquipado(Juego* juego);
void  inicializarEnemigos(Juego* juego);
void  generarEnemigo(Enemigo* enemigo);
void  manejarEventos(Juego* juego);
void  actualizarJugador(Jugador* jugador, SDL_Gamepad* gamepad);
void  actualizarEnemigos(Juego* juego);
bool  verificarColision(SDL_FRect* a, SDL_FRect* b);
void  dibujarJuego(Juego* juego);
void  renderizar(Juego* juego);
void  limpiarRecursos(Juego* juego);
void  actualizarAnimacionAtaque(Juego* juego);
void  calcularPosicionMacheteGirando(Juego* juego, float* posX, float* posY);
void  reiniciarJuego(Juego* juego);
void  iniciarCuentaRegresiva(Juego* juego);
void  renderizarCuentaRegresiva(Juego* juego);
void  renderizarMenu(Juego* juego);
void  manejarEventosMenu(Juego* juego);
void  renderizarInstrucciones(Juego* juego);
void  renderizarGameOver(Juego* juego);
void  renderizarPausa(Juego* juego);
void  crearDirectorioSaves();
void  cargarPuntajes(TablaPuntajes* tabla);
void  guardarPuntajes(const TablaPuntajes* tabla);
bool  calificaParaTop5(const TablaPuntajes* tabla, int puntuacion);
int   insertarPuntaje(TablaPuntajes* tabla, const char* nombre, int puntuacion);
void  renderizarTop5(Juego* juego, int x, int y, int posicionResaltada);
void  iniciarIngresoNombre(Juego* juego);
void  renderizarIngresoNombre(Juego* juego);
void  iniciarTransicionNivel(Juego* juego, int nivelNuevo);
void  actualizarTransicionNivel(Juego* juego);
void  renderizarTransicionNivel(Juego* juego);

// ============================================
// FUNCION PRINCIPAL
// ============================================
int main(int argc, char* argv[]) {
    FILE* log = fopen("log.txt", "w");
    if (log) { fprintf(log, "Iniciando...\n"); fclose(log); }

    const char* basePath = SDL_GetBasePath();
    if (basePath) {
#ifdef _WIN32
        SetCurrentDirectoryA(basePath);
#else
        chdir(basePath);
#endif
        log = fopen("log.txt", "a");
        if (log) { fprintf(log, "BasePath: %s\n", basePath); fclose(log); }
    }

    srand((unsigned int)time(NULL));

    Juego juego = {};
    juego.estado                 = ESTADO_MENU;
    juego.opcionMenuSeleccionada = 0;
    juego.posicionNuevoPuntaje   = -1;
    juego.nivel4Reproducido      = false;
    juego.gameOverReproducido    = false;
    juego.transicion             = {};
    juego.opcionGameOverSel      = 0;
    juego.opcionPausaSel         = 0;
    juego.inicioCuentaRegresiva  = 0;

    crearDirectorioSaves();
    cargarPuntajes(&juego.tablaPuntajes);

    if (!inicializarSDL(&juego)) {
        log = fopen("log.txt", "a");
        if (log) { fprintf(log, "FALLO: inicializarSDL\n"); fclose(log); }
        return 1;
    }
    if (!cargarTexturas(&juego)) {
        log = fopen("log.txt", "a");
        if (log) { fprintf(log, "FALLO: cargarTexturas\n"); fclose(log); }
        limpiarRecursos(&juego); return 1;
    }
    if (!cargarFuente(&juego)) {
        log = fopen("log.txt", "a");
        if (log) { fprintf(log, "FALLO: cargarFuente\n"); fclose(log); }
        limpiarRecursos(&juego); return 1;
    }

    log = fopen("log.txt", "a");
    if (log) { fprintf(log, "Todo OK, entrando al game loop\n"); fclose(log); }

    inicializarJugador(&juego.jugador);
    inicializarEnemigos(&juego);
    inicializarMachete(&juego);
    juego.macheteAparecido      = false;
    juego.ultimoNivelDificultad = 0;
    juego.ejecutando            = true;

    while (juego.ejecutando) {
        if (!juego.transicion.activa)
            reproducirMusica(&juego, pistaSegunEstadoJuego(&juego));

        switch (juego.estado) {
            case ESTADO_MENU:
                manejarEventosMenu(&juego);
                if (juego.ejecutando) renderizarMenu(&juego);
                break;
            case ESTADO_INSTRUCCIONES:
                renderizarInstrucciones(&juego);
                break;
            case ESTADO_CUENTA_REGRESIVA:
                renderizarCuentaRegresiva(&juego);
                break;
            case ESTADO_JUGANDO:
                manejarEventos(&juego);
                actualizarJugador(&juego.jugador, juego.gamepad);
                actualizarAnimacionAtaque(&juego);
                actualizarPosicionMacheteEquipado(&juego);
                actualizarEnemigos(&juego);
                if (!juego.ejecutando) {
                    juego.ejecutando = true;
                    if (calificaParaTop5(&juego.tablaPuntajes, juego.puntuacion)) {
                        iniciarIngresoNombre(&juego);
                        juego.estado = ESTADO_INGRESANDO_NOMBRE;
                    } else {
                        juego.opcionGameOverSel = 0;
                        juego.estado = ESTADO_GAME_OVER;
                    }
                }
                renderizar(&juego);
                break;
            case ESTADO_INGRESANDO_NOMBRE:
                renderizarIngresoNombre(&juego);
                break;
            case ESTADO_PAUSADO:
                renderizarPausa(&juego);
                break;
            case ESTADO_GAME_OVER:
                renderizarGameOver(&juego);
                break;
            case ESTADO_TRANSICION_NIVEL:
                actualizarTransicionNivel(&juego);
                renderizarTransicionNivel(&juego);
                break;
        }
        SDL_Delay(16);
    }

    limpiarRecursos(&juego);
    return 0;
}

// ============================================
// NIVEL ACTUAL
// ============================================
int nivelActual(int puntuacion) {
    if (puntuacion >= UMBRAL_NIVEL_5) return 5;
    if (puntuacion >= UMBRAL_NIVEL_4) return 4;
    if (puntuacion >= UMBRAL_NIVEL_3) return 3;
    if (puntuacion >= UMBRAL_NIVEL_2) return 2;
    return 1;
}

// ============================================
// TRANSICION DE NIVEL
// ============================================
void iniciarTransicionNivel(Juego* juego, int nivelNuevo) {
    juego->transicion.activa         = true;
    juego->transicion.nivelNuevo     = nivelNuevo;
    juego->transicion.inicio         = SDL_GetTicks();
    juego->transicion.estadoAnterior = ESTADO_JUGANDO;

    if (juego->musicaActiva) {
        MIX_StopTrack(juego->trackMusica, 0);
        juego->pistaSonando = PISTA_NINGUNA;
    }

    juego->estado = ESTADO_TRANSICION_NIVEL;
    SDL_Log("Iniciando transicion al nivel %d", nivelNuevo);
}

void actualizarTransicionNivel(Juego* juego) {
    Uint64 elapsed = SDL_GetTicks() - juego->transicion.inicio;
    if (elapsed >= DURACION_TRANSICION) {
        juego->transicion.activa = false;
        juego->estado = ESTADO_JUGANDO;
        juego->pistaSonando = PISTA_NINGUNA;
    }
}

void renderizarTransicionNivel(Juego* juego) {
    Uint64 elapsed  = SDL_GetTicks() - juego->transicion.inicio;
    float  progreso = (float)elapsed / (float)DURACION_TRANSICION;
    int    nivel    = juego->transicion.nivelNuevo;

    SDL_RenderClear(juego->renderer);
    int idxDestino = SDL_clamp(nivel - 1, 0, 4);
    int idxOrigen  = SDL_clamp(nivel - 2, 0, 4);

    if (juego->texFondos[idxOrigen])
        SDL_RenderTexture(juego->renderer, juego->texFondos[idxOrigen], NULL, NULL);

    if (juego->texFondos[idxDestino]) {
        float alphaNew = SDL_clamp((progreso - 0.3f) / 0.5f, 0.0f, 1.0f);
        SDL_SetTextureAlphaMod(juego->texFondos[idxDestino], (Uint8)(alphaNew * 255.0f));
        SDL_RenderTexture(juego->renderer, juego->texFondos[idxDestino], NULL, NULL);
        SDL_SetTextureAlphaMod(juego->texFondos[idxDestino], 255);
    }

    float fase = sinf(progreso * (float)M_PI * 8.0f) * 0.5f + 0.5f;
    Uint8 ov_r, ov_g, ov_b;
    Uint8 barR, barG, barB;
    if (nivel == 4) {
        ov_r = (Uint8)(255*fase + 200*(1.0f-fase));
        ov_g = (Uint8)(100*fase +  40*(1.0f-fase));
        ov_b = 0;
        barR=200; barG=80;  barB=0;
    } else {
        ov_r = (Uint8)(180*fase + 255*(1.0f-fase));
        ov_g = 0;
        ov_b = (Uint8)(255*fase + 180*(1.0f-fase));
        barR=120; barG=0;   barB=220;
    }
    float envolvente = sinf(progreso * (float)M_PI);
    Uint8 ov_alpha   = (Uint8)(envolvente * 160.0f);

    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(juego->renderer, ov_r, ov_g, ov_b, ov_alpha);
    SDL_FRect overlay = {0.0f, 0.0f, (float)ANCHO_VENTANA, (float)ALTO_VENTANA};
    SDL_RenderFillRect(juego->renderer, &overlay);

    if (progreso < 0.15f) {
        float flashAlpha = (0.15f - progreso) / 0.15f;
        SDL_SetRenderDrawColor(juego->renderer, 255, 255, 255, (Uint8)(flashAlpha * 200.0f));
        SDL_RenderFillRect(juego->renderer, &overlay);
    }
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);

    if (progreso > 0.2f && progreso < 0.85f) {
        SDL_Color colorTitulo, colorSubtitulo;
        if (nivel == 4) {
            colorTitulo    = {255, 220,  40, 255};
            colorSubtitulo = {255, 140,   0, 255};
        } else {
            colorTitulo    = {220, 100, 255, 255};
            colorSubtitulo = {180,  40, 255, 255};
        }

        const char* tituloNivel = (nivel == 4) ? "NIVEL 4" : "NIVEL 5";
        const char* subtitulo   = (nivel == 4) ? "!EL JEFE SE ACERCA!" : "!MAXIMA DIFICULTAD!";

        int txW = (int)SDL_strlen(tituloNivel) * 28;
        renderizarTexto(juego, tituloNivel,
            ANCHO_VENTANA / 2 - txW / 2, ALTO_VENTANA / 2 - 56, colorTitulo);

        int stW = (int)SDL_strlen(subtitulo) * 18;
        renderizarTextoPequeno(juego, subtitulo,
            ANCHO_VENTANA / 2 - stW / 2, ALTO_VENTANA / 2 + 8, colorSubtitulo);

        const int barW = 400, barH = 8;
        const int barX = ANCHO_VENTANA / 2 - barW / 2;
        const int barY = ALTO_VENTANA  / 2 + 70;

        SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(juego->renderer, 30, 30, 30, 180);
        SDL_FRect barFondo   = {(float)barX, (float)barY, (float)barW, (float)barH};
        SDL_RenderFillRect(juego->renderer, &barFondo);
        SDL_SetRenderDrawColor(juego->renderer, barR, barG, barB, 255);
        SDL_FRect barRelleno = {(float)barX, (float)barY, barW * progreso, (float)barH};
        SDL_RenderFillRect(juego->renderer, &barRelleno);
        SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);
    }

    SDL_RenderPresent(juego->renderer);
}

// ============================================
// AUDIO
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

    if (!juego->musicaMenu)       SDL_Log("Audio: no se cargo %s", RUTA_MUSICA_MENU);
    if (!juego->musicaPausa)      SDL_Log("Audio: no se cargo %s", RUTA_MUSICA_PAUSA);
    if (!juego->musicaNiveles123) SDL_Log("Audio: no se cargo %s", RUTA_MUSICA_NIVELES123);
    if (!juego->musicaNivel4)     SDL_Log("Audio: no se cargo %s", RUTA_MUSICA_NIVEL4);
    if (!juego->musicaNivel5)     SDL_Log("Audio: no se cargo %s", RUTA_MUSICA_NIVEL5);
    if (!juego->musicaGameOver)   SDL_Log("Audio: no se cargo %s", RUTA_MUSICA_GAMEOVER);

    return true;
}

EstadoPista pistaSegunEstadoJuego(Juego* juego) {
    switch (juego->estado) {
        case ESTADO_MENU:
        case ESTADO_INSTRUCCIONES:
        case ESTADO_INGRESANDO_NOMBRE:
        case ESTADO_CUENTA_REGRESIVA:   // cuenta regresiva usa musica del menu
            return PISTA_MENU;

        case ESTADO_PAUSADO:
            return PISTA_PAUSA;

        case ESTADO_GAME_OVER:
            if (juego->gameOverReproducido)
                return PISTA_MENU;
            return PISTA_GAMEOVER;

        case ESTADO_JUGANDO: {
            int nivel = nivelActual(juego->puntuacion);
            if (nivel <= 3) return PISTA_NIVELES123;
            if (nivel == 4) {
                if (!juego->nivel4Reproducido)
                    return PISTA_NIVEL4;
                return PISTA_NIVEL5;
            }
            return PISTA_NIVEL5;
        }

        case ESTADO_TRANSICION_NIVEL:
            return PISTA_NINGUNA;

        default:
            return PISTA_MENU;
    }
}

void reproducirMusica(Juego* juego, EstadoPista pista) {
    if (!juego->musicaActiva) return;
    if (juego->pistaSonando == pista) return;

    MIX_Audio* objetivo = nullptr;
    bool loopInfinito   = true;

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
        if (pista == PISTA_NIVEL4)   juego->nivel4Reproducido   = true;
        if (pista == PISTA_GAMEOVER) juego->gameOverReproducido  = true;
        return;
    }

    MIX_StopTrack(juego->trackMusica, 0);
    MIX_SetTrackAudio(juego->trackMusica, objetivo);

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, loopInfinito ? -1 : 0);
    MIX_PlayTrack(juego->trackMusica, props);
    SDL_DestroyProperties(props);

    juego->pistaSonando = pista;

    if (pista == PISTA_NIVEL4)   juego->nivel4Reproducido   = true;
    if (pista == PISTA_GAMEOVER) juego->gameOverReproducido  = true;
}

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

// ============================================
// SDL E INICIALIZACION
// ============================================
bool inicializarSDL(Juego* juego) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
        SDL_Log("Error SDL: %s", SDL_GetError());
        return false;
    }
    juego->ventana = SDL_CreateWindow("Esquivar Botellas", ANCHO_VENTANA, ALTO_VENTANA, 0);
    if (!juego->ventana) { SDL_Log("Error ventana: %s", SDL_GetError()); return false; }

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
                if (juego->gamepad) {
                    SDL_Log("Gamepad: %s", SDL_GetGamepadName(juego->gamepad));
                    break;
                }
            }
        }
        SDL_free(ids);
    }
    return true;
}

bool cargarTexturas(Juego* juego) {
    const char* rutasFondos[5] = {
        RUTA_FONDO_NIVEL1, RUTA_FONDO_NIVEL2, RUTA_FONDO_NIVEL3,
        RUTA_FONDO_NIVEL4, RUTA_FONDO_NIVEL5
    };
    for (int i = 0; i < 5; i++) {
        juego->texFondos[i] = IMG_LoadTexture(juego->renderer, rutasFondos[i]);
        if (!juego->texFondos[i])
            SDL_Log("Advertencia: no se cargo fondo nivel %d (%s): %s",
                    i+1, rutasFondos[i], SDL_GetError());
    }

    juego->texJugador = IMG_LoadTexture(juego->renderer, "imagenes/player.png");
    juego->texEnemigo = IMG_LoadTexture(juego->renderer, "imagenes/enemy_roja.png");
    juego->texMachete = IMG_LoadTexture(juego->renderer, "imagenes/machete.png");

    FILE* log = fopen("log.txt", "a");
    if (log) {
        for (int i = 0; i < 5; i++)
            fprintf(log, "texFondo[%d]:%p\n", i+1, (void*)juego->texFondos[i]);
        fprintf(log, "texJugador:%p texEnemigo:%p texMachete:%p\n",
            (void*)juego->texJugador, (void*)juego->texEnemigo, (void*)juego->texMachete);
        fclose(log);
    }
    return (juego->texJugador && juego->texEnemigo && juego->texMachete);
}

bool cargarFuente(Juego* juego) {
    juego->fuente = TTF_OpenFont("Arial Black.ttf", 36);
    if (!juego->fuente) { SDL_Log("Error fuente: %s", SDL_GetError()); return false; }

    juego->fuentePequena = TTF_OpenFont("Arial Black.ttf", 22);
    if (!juego->fuentePequena)
        juego->fuentePequena = juego->fuente;

    return true;
}

// ============================================
// RENDERIZADO DE TEXTO
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

// Renderiza texto centrado horizontalmente usando TTF_GetStringSize para precision real
void renderizarTextoCentrado(Juego* juego, TTF_Font* fuente, const char* texto, int y, SDL_Color color) {
    int w = 0, h = 0;
    TTF_GetStringSize(fuente, texto, 0, &w, &h);
    int x = (ANCHO_VENTANA - w) / 2;
    SDL_Surface* sup = TTF_RenderText_Solid(fuente, texto, 0, color);
    if (!sup) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(juego->renderer, sup);
    if (tex) {
        SDL_FRect dst = {(float)x, (float)y, (float)sup->w, (float)sup->h};
        SDL_RenderTexture(juego->renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_DestroySurface(sup);
}

void renderizarTextoCentradoPequeno(Juego* juego, const char* texto, int y, SDL_Color color) {
    renderizarTextoCentrado(juego, juego->fuentePequena, texto, y, color);
}

// ============================================
// HELPERS DE UI (boton y mouse)
// ============================================
bool mouseEnRect(int mx, int my, int x, int y, int w, int h) {
    return mx >= x && mx <= x + w && my >= y && my <= y + h;
}

void dibujarBoton(Juego* juego, const char* texto, int x, int y, int w, int h,
                  bool seleccionado, SDL_Color colorNormal, SDL_Color colorHover) {
    // Fondo del boton
    SDL_Color colorFondo = seleccionado
        ? (SDL_Color){60, 60, 80, 220}
        : (SDL_Color){30, 30, 40, 180};
    SDL_Color colorBorde = seleccionado ? colorHover : colorNormal;

    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(juego->renderer,
        colorFondo.r, colorFondo.g, colorFondo.b, colorFondo.a);
    SDL_FRect rect = {(float)x, (float)y, (float)w, (float)h};
    SDL_RenderFillRect(juego->renderer, &rect);
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);

    // Borde
    SDL_SetRenderDrawColor(juego->renderer,
        colorBorde.r, colorBorde.g, colorBorde.b, 255);
    SDL_RenderRect(juego->renderer, &rect);

    // Texto centrado en el boton
    int tw = 0, th = 0;
    TTF_GetStringSize(juego->fuentePequena, texto, 0, &tw, &th);
    int tx = x + (w - tw) / 2;
    int ty = y + (h - th) / 2;
    renderizarTextoPequeno(juego, texto, tx, ty,
        seleccionado ? colorHover : colorNormal);
}

// ============================================
// SISTEMA DE PUNTAJES
// ============================================
void crearDirectorioSaves() {
#ifdef _WIN32
    _mkdir("saves");
#else
    mkdir("saves", 0755);
#endif
}

void cargarPuntajes(TablaPuntajes* tabla) {
    tabla->cantidad = 0;
    for (int i = 0; i < MAX_PUNTAJES; i++) {
        tabla->entradas[i].puntuacion = 0;
        tabla->entradas[i].nombre[0]  = '\0';
    }
    FILE* f = fopen(RUTA_SAVES, "rb");
    if (!f) return;

    fread(&tabla->cantidad, sizeof(int), 1, f);
    if (tabla->cantidad < 0 || tabla->cantidad > MAX_PUNTAJES)
        tabla->cantidad = 0;

    for (int i = 0; i < tabla->cantidad; i++)
        fread(&tabla->entradas[i], sizeof(EntradaPuntaje), 1, f);

    fclose(f);
}

void guardarPuntajes(const TablaPuntajes* tabla) {
    crearDirectorioSaves();
    FILE* f = fopen(RUTA_SAVES, "wb");
    if (!f) { SDL_Log("Error: no se pudo guardar puntajes."); return; }
    fwrite(&tabla->cantidad, sizeof(int), 1, f);
    for (int i = 0; i < tabla->cantidad; i++)
        fwrite(&tabla->entradas[i], sizeof(EntradaPuntaje), 1, f);
    fclose(f);
}

bool calificaParaTop5(const TablaPuntajes* tabla, int puntuacion) {
    if (puntuacion <= 0) return false;
    if (tabla->cantidad < MAX_PUNTAJES) return true;
    return puntuacion > tabla->entradas[tabla->cantidad - 1].puntuacion;
}

int insertarPuntaje(TablaPuntajes* tabla, const char* nombre, int puntuacion) {
    int pos = tabla->cantidad;
    for (int i = 0; i < tabla->cantidad; i++) {
        if (puntuacion > tabla->entradas[i].puntuacion) { pos = i; break; }
    }
    int hasta = (tabla->cantidad < MAX_PUNTAJES) ? tabla->cantidad : MAX_PUNTAJES - 1;
    for (int i = hasta; i > pos; i--)
        tabla->entradas[i] = tabla->entradas[i - 1];

    SDL_snprintf(tabla->entradas[pos].nombre, MAX_NOMBRE, "%s", nombre);
    tabla->entradas[pos].puntuacion = puntuacion;
    if (tabla->cantidad < MAX_PUNTAJES) tabla->cantidad++;

    guardarPuntajes(tabla);
    return pos;
}

void renderizarTop5(Juego* juego, int x, int y, int posicionResaltada) {
    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color gris     = {130, 130, 130, 255};
    SDL_Color dorado   = {255, 180,   0, 255};
    SDL_Color verde    = { 80, 255, 120, 255};

    const char* medallas[] = {"#1", "#2", "#3", "#4", "#5"};
    const int espaciado = 44;

    renderizarTextoPequeno(juego, "--- TOP 5 ---", x, y, amarillo);

    if (juego->tablaPuntajes.cantidad == 0) {
        renderizarTextoPequeno(juego, "Sin records aun", x, y + espaciado, gris);
        return;
    }

    for (int i = 0; i < MAX_PUNTAJES; i++) {
        char linea[72];
        SDL_Color color;

        if (i < juego->tablaPuntajes.cantidad) {
            if (i == posicionResaltada)
                color = verde;
            else if (i == 0)
                color = dorado;
            else
                color = blanco;

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
// INGRESO DE NOMBRE
// ============================================
void iniciarIngresoNombre(Juego* juego) {
    SDL_memset(juego->nombreIngresado, 0, MAX_NOMBRE);
    juego->longitudNombre       = 0;
    juego->posicionNuevoPuntaje = -1;
    SDL_StartTextInput(juego->ventana);
}

void renderizarIngresoNombre(Juego* juego) {
    SDL_RenderClear(juego->renderer);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color verde    = { 80, 255, 120, 255};
    SDL_Color gris     = {130, 130, 130, 255};

    renderizarTextoCentrado(juego, juego->fuente, "NUEVO RECORD!", 100, amarillo);

    char msgPuntaje[64];
    SDL_snprintf(msgPuntaje, sizeof(msgPuntaje), "Puntuacion: %d", juego->puntuacion);
    renderizarTextoCentrado(juego, juego->fuente, msgPuntaje, 180, blanco);

    renderizarTextoCentradoPequeno(juego,
        "Ingresa tu nombre (max 31 caracteres) y presiona Enter", 265, gris);

    SDL_SetRenderDrawColor(juego->renderer, 50, 50, 50, 255);
    SDL_FRect caja = {(float)(ANCHO_VENTANA / 2 - 220), 310.0f, 440.0f, 54.0f};
    SDL_RenderFillRect(juego->renderer, &caja);
    SDL_SetRenderDrawColor(juego->renderer, 255, 220, 0, 255);
    SDL_RenderRect(juego->renderer, &caja);

    char textoMostrado[MAX_NOMBRE + 2];
    bool cursorVisible = (SDL_GetTicks() / 500) % 2 == 0;
    SDL_snprintf(textoMostrado, sizeof(textoMostrado), "%s%s",
        juego->nombreIngresado, cursorVisible ? "_" : " ");
    renderizarTexto(juego, textoMostrado, ANCHO_VENTANA / 2 - 205, 318, verde);

    renderizarTop5(juego, ANCHO_VENTANA - 430, 100, -1);

    renderizarTextoCentradoPequeno(juego,
        "Enter: confirmar    ESC: cancelar (puntaje no guardado)",
        ALTO_VENTANA - 55, gris);

    SDL_RenderPresent(juego->renderer);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            SDL_StopTextInput(juego->ventana);
            juego->ejecutando = false;
            return;
        }
        if (e.type == SDL_EVENT_TEXT_INPUT) {
            int espacio = MAX_NOMBRE - 1 - juego->longitudNombre;
            if (espacio > 0) {
                int len    = (int)SDL_strlen(e.text.text);
                int copiar = (len < espacio) ? len : espacio;
                SDL_strlcat(juego->nombreIngresado, e.text.text, MAX_NOMBRE);
                juego->longitudNombre += copiar;
            }
        }
        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_BACKSPACE && juego->longitudNombre > 0) {
                juego->longitudNombre--;
                juego->nombreIngresado[juego->longitudNombre] = '\0';
            }
            if ((e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER)
                && juego->longitudNombre > 0) {
                SDL_StopTextInput(juego->ventana);
                juego->posicionNuevoPuntaje = insertarPuntaje(
                    &juego->tablaPuntajes,
                    juego->nombreIngresado,
                    juego->puntuacion);
                juego->opcionGameOverSel = 0;
                juego->estado = ESTADO_GAME_OVER;
            }
            if (e.key.key == SDLK_ESCAPE) {
                SDL_StopTextInput(juego->ventana);
                juego->posicionNuevoPuntaje = -1;
                juego->opcionGameOverSel    = 0;
                juego->estado = ESTADO_GAME_OVER;
            }
        }
    }
}

// ============================================
// JUGADOR
// ============================================
void mostrarPuntuacionPantalla(Juego* juego) {
    SDL_Color blanco = {255, 255, 255, 255};
    std::string txt = "Score: " + std::to_string(juego->puntuacion);
    renderizarTexto(juego, txt.c_str(), 10, 10, blanco);

    int nivel = nivelActual(juego->puntuacion);
    char txtNivel[32];
    SDL_snprintf(txtNivel, sizeof(txtNivel), "Nivel: %d", nivel);
    SDL_Color colorNivel;
    switch (nivel) {
        case 1: case 2: case 3: colorNivel = {100, 220, 100, 255}; break;
        case 4: colorNivel = {255, 165, 0, 255}; break;
        case 5: colorNivel = {220, 50,  50, 255}; break;
        default: colorNivel = {255, 255, 255, 255}; break;
    }
    renderizarTexto(juego, txtNivel, 10, 52, colorNivel);

    SDL_Color colorAudio = juego->musicaActiva
        ? (SDL_Color){80, 255, 120, 255}
        : (SDL_Color){180, 180, 180, 255};
    char textoAudio[32];
    if (juego->musicaActiva)
        SDL_snprintf(textoAudio, sizeof(textoAudio), "Vol:%d%%",
            juego->volumenMusica * 100 / 128);
    else
        SDL_snprintf(textoAudio, sizeof(textoAudio), "SIN AUDIO");
    renderizarTextoPequeno(juego, textoAudio, ANCHO_VENTANA - 130, 10, colorAudio);
}

void inicializarJugador(Jugador* jugador) {
    jugador->rect.x    = (float)(ANCHO_VENTANA - TAMANO_SPRITE) / 2;
    jugador->rect.y    = (float)(ALTO_VENTANA  - TAMANO_SPRITE) / 2;
    jugador->rect.w    = (float)TAMANO_SPRITE;
    jugador->rect.h    = (float)TAMANO_SPRITE;
    jugador->velocidad = 4;
}

// ============================================
// MACHETE
// ============================================
void inicializarMachete(Juego* juego) {
    juego->machete.recogido        = false;
    juego->machete.activo          = false;
    juego->machete.ultimoUso       = 0;
    juego->machete.rect.w          = (float)TAMANO_SPRITE;
    juego->machete.rect.h          = (float)TAMANO_SPRITE;
    juego->macheteEquipado         = false;
    juego->machete.animandoAtaque  = false;
    juego->machete.inicioAnimacion = 0;
    juego->machete.anguloActual    = 0.0f;
}

void aparecerMachete(Juego* juego) {
    juego->machete.rect.x   = (float)(rand() % (ANCHO_VENTANA - TAMANO_SPRITE));
    juego->machete.rect.y   = (float)(rand() % (ALTO_VENTANA  - TAMANO_SPRITE));
    juego->machete.recogido = false;
}

void verificarRecogidaMachete(Juego* juego) {
    if (!juego->machete.recogido &&
        verificarColision(&juego->jugador.rect, &juego->machete.rect)) {
        juego->machete.recogido = true;
        juego->macheteEquipado  = true;
        juego->machete.rect.w   = 48.0f;
        juego->machete.rect.h   = 48.0f;
        SDL_Log("Machete equipado!");
    }
}

void usarMachete(Juego* juego) {
    Uint64 tiempoActual = SDL_GetTicks();
    if (tiempoActual - juego->machete.ultimoUso < COOLDOWN_MACHETE) return;

    juego->machete.activo          = true;
    juego->machete.ultimoUso       = tiempoActual;
    juego->machete.animandoAtaque  = true;
    juego->machete.inicioAnimacion = tiempoActual;
    juego->machete.anguloActual    = 0.0f;

    float cx = juego->jugador.rect.x + TAMANO_SPRITE / 2.0f;
    float cy = juego->jugador.rect.y + TAMANO_SPRITE / 2.0f;
    int   destruidos = 0;

    for (int i = 0; i < juego->enemigosActivos; i++) {
        float dx  = (juego->enemigos[i].rect.x + TAMANO_SPRITE / 2.0f) - cx;
        float dy  = (juego->enemigos[i].rect.y + TAMANO_SPRITE / 2.0f) - cy;
        float dis = sqrtf(dx*dx + dy*dy);
        if (dis <= RANGO_ATAQUE) {
            juego->puntuacion++;
            generarEnemigo(&juego->enemigos[i]);
            destruidos++;
        }
    }
    if (destruidos > 0) SDL_Log("Machete: %d destruidos.", destruidos);
}

float calcularProgresoCooldown(Juego* juego) {
    if (juego->machete.ultimoUso == 0) return 1.0f;
    Uint64 t = SDL_GetTicks() - juego->machete.ultimoUso;
    if (t >= COOLDOWN_MACHETE) return 1.0f;
    return (float)t / (float)COOLDOWN_MACHETE;
}

void renderizarBarraCooldown(Juego* juego) {
    if (!juego->macheteEquipado) return;

    float progreso = calcularProgresoCooldown(juego);

    const int barX    = BARRA_COOLDOWN_X;
    const int barY    = BARRA_COOLDOWN_Y;
    const int barW    = BARRA_COOLDOWN_ANCHO;
    const int barH    = BARRA_COOLDOWN_ALTO;
    const int offsetX = 34;

    SDL_FRect icono = {(float)barX, (float)(barY - 4), 28.0f, 28.0f};
    SDL_RenderTexture(juego->renderer, juego->texMachete, NULL, &icono);

    SDL_SetRenderDrawColor(juego->renderer, 0, 0, 0, 255);
    SDL_FRect borde = {
        (float)(barX + offsetX - 2), (float)(barY - 2),
        (float)(barW + 4), (float)(barH + 4)
    };
    SDL_RenderFillRect(juego->renderer, &borde);

    SDL_SetRenderDrawColor(juego->renderer, 35, 35, 35, 255);
    SDL_FRect fondo = {(float)(barX + offsetX), (float)barY, (float)barW, (float)barH};
    SDL_RenderFillRect(juego->renderer, &fondo);

    Uint8 r, g, b;
    if (progreso < 0.5f) {
        r = 255; g = (Uint8)(progreso * 2.0f * 200); b = 0;
    } else {
        r = (Uint8)((1.0f - (progreso - 0.5f) * 2.0f) * 255); g = 200; b = 0;
    }
    SDL_SetRenderDrawColor(juego->renderer, r, g, b, 255);
    SDL_FRect relleno = {
        (float)(barX + offsetX), (float)barY,
        barW * progreso, (float)barH
    };
    SDL_RenderFillRect(juego->renderer, &relleno);

    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(juego->renderer, 255, 255, 255, 55);
    SDL_FRect brillo = {
        (float)(barX + offsetX), (float)barY,
        barW * progreso, (float)(barH / 2)
    };
    SDL_RenderFillRect(juego->renderer, &brillo);
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);

    char etiqueta[48];
    SDL_Color colorEtiqueta;
    if (progreso >= 1.0f) {
        SDL_snprintf(etiqueta, sizeof(etiqueta), "LISTO  [ESPACIO]");
        colorEtiqueta = {0, 230, 80, 255};
    } else {
        float restante = (COOLDOWN_MACHETE
            - (float)(SDL_GetTicks() - juego->machete.ultimoUso)) / 1000.0f;
        SDL_snprintf(etiqueta, sizeof(etiqueta), "%.1fs", restante);
        colorEtiqueta = {200, 200, 200, 255};
    }
    renderizarTextoPequeno(juego, etiqueta,
        barX + offsetX + barW + 8, barY - 1, colorEtiqueta);
}

void calcularPosicionMacheteGirando(Juego* juego, float* posX, float* posY) {
    float cx  = juego->jugador.rect.x + TAMANO_SPRITE / 2.0f;
    float cy  = juego->jugador.rect.y + TAMANO_SPRITE / 2.0f;
    float rad = juego->machete.anguloActual * (float)M_PI / 180.0f;
    *posX = cx + cosf(rad) * RADIO_GIRO_MACHETE - 24.0f;
    *posY = cy + sinf(rad) * RADIO_GIRO_MACHETE - 24.0f;
}

void actualizarPosicionMacheteEquipado(Juego* juego) {
    if (!juego->macheteEquipado) return;
    if (juego->machete.animandoAtaque) {
        float px, py;
        calcularPosicionMacheteGirando(juego, &px, &py);
        juego->machete.rect.x = px;
        juego->machete.rect.y = py;
    } else {
        juego->machete.rect.x = juego->jugador.rect.x + OFFSET_MACHETE_X;
        juego->machete.rect.y = juego->jugador.rect.y + OFFSET_MACHETE_Y;
    }
}

// ============================================
// ENEMIGOS
// ============================================
void inicializarEnemigos(Juego* juego) {
    juego->enemigosActivos = 1;
    juego->puntuacion      = 0;
    generarEnemigo(&juego->enemigos[0]);
}

void actualizarAnimacionAtaque(Juego* juego) {
    if (!juego->machete.animandoAtaque) return;
    Uint64 t = SDL_GetTicks() - juego->machete.inicioAnimacion;
    if (t >= DURACION_ANIMACION_ATAQUE) {
        juego->machete.animandoAtaque = false;
        juego->machete.anguloActual   = 0.0f;
    } else {
        juego->machete.anguloActual = ((float)t / DURACION_ANIMACION_ATAQUE) * 360.0f;
    }
}

void generarEnemigo(Enemigo* enemigo) {
    int lado = rand() % 4;
    enemigo->rect.w = (float)TAMANO_SPRITE;
    enemigo->rect.h = (float)TAMANO_SPRITE;
    if (lado == 0) {
        enemigo->rect.x = -(float)TAMANO_SPRITE;
        enemigo->rect.y = (float)(rand() % (ALTO_VENTANA - TAMANO_SPRITE));
        enemigo->velX = 5.0f; enemigo->velY = 0.0f;
    } else if (lado == 1) {
        enemigo->rect.x = (float)ANCHO_VENTANA;
        enemigo->rect.y = (float)(rand() % (ALTO_VENTANA - TAMANO_SPRITE));
        enemigo->velX = -5.0f; enemigo->velY = 0.0f;
    } else if (lado == 2) {
        enemigo->rect.x = (float)(rand() % (ANCHO_VENTANA - TAMANO_SPRITE));
        enemigo->rect.y = -(float)TAMANO_SPRITE;
        enemigo->velX = 0.0f; enemigo->velY = 5.0f;
    } else {
        enemigo->rect.x = (float)(rand() % (ANCHO_VENTANA - TAMANO_SPRITE));
        enemigo->rect.y = (float)ALTO_VENTANA;
        enemigo->velX = 0.0f; enemigo->velY = -5.0f;
    }
}

// ============================================
// EVENTOS Y ACTUALIZACION
// ============================================
void manejarEventos(Juego* juego) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }

        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_SPACE && juego->macheteEquipado)
                usarMachete(juego);
            if (e.key.key == SDLK_ESCAPE) {
                juego->opcionPausaSel = 0;
                juego->estado = ESTADO_PAUSADO;
            }
            if (e.key.key == SDLK_M)
                toggleMusicaMute(juego);
            if (e.key.key == SDLK_EQUALS || e.key.key == SDLK_PLUS)
                ajustarVolumen(juego, 16);
            if (e.key.key == SDLK_MINUS)
                ajustarVolumen(juego, -16);
        }

        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH && juego->macheteEquipado)
                usarMachete(juego);
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_START) {
                juego->opcionPausaSel = 0;
                juego->estado = ESTADO_PAUSADO;
            }
        }

        if (e.type == SDL_EVENT_GAMEPAD_ADDED && !juego->gamepad)
            juego->gamepad = SDL_OpenGamepad(e.gdevice.which);
        if (e.type == SDL_EVENT_GAMEPAD_REMOVED && juego->gamepad) {
            SDL_CloseGamepad(juego->gamepad);
            juego->gamepad = nullptr;
        }
    }
}

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
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP))
            jugador->rect.y -= (float)jugador->velocidad;
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN))
            jugador->rect.y += (float)jugador->velocidad;
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT))
            jugador->rect.x -= (float)jugador->velocidad;
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
            jugador->rect.x += (float)jugador->velocidad;
    }

    if (jugador->rect.x < 0.0f) jugador->rect.x = 0.0f;
    if (jugador->rect.x > ANCHO_VENTANA - TAMANO_SPRITE)
        jugador->rect.x = (float)(ANCHO_VENTANA - TAMANO_SPRITE);
    if (jugador->rect.y < 0.0f) jugador->rect.y = 0.0f;
    if (jugador->rect.y > ALTO_VENTANA - TAMANO_SPRITE)
        jugador->rect.y = (float)(ALTO_VENTANA - TAMANO_SPRITE);
}

bool verificarColision(SDL_FRect* a, SDL_FRect* b) {
    return SDL_HasRectIntersectionFloat(a, b);
}

void actualizarEnemigos(Juego* juego) {
    int nivel = nivelActual(juego->puntuacion);

    if (nivel > juego->ultimoNivelDificultad && juego->enemigosActivos < MAX_ENEMIGOS) {
        generarEnemigo(&juego->enemigos[juego->enemigosActivos]);
        juego->enemigosActivos++;
        SDL_Log("Subio al nivel %d — Enemigos activos: %d", nivel, juego->enemigosActivos);

        if (nivel == 4 || nivel == 5) {
            juego->ultimoNivelDificultad = nivel;
            iniciarTransicionNivel(juego, nivel);
            return;
        }
        juego->ultimoNivelDificultad = nivel;
    }

    if (juego->puntuacion >= UMBRAL_NIVEL_4 && !juego->macheteAparecido && !juego->machete.recogido) {
        aparecerMachete(juego);
        juego->macheteAparecido = true;
        SDL_Log("Machete aparecio! (nivel 4, puntuacion %d)", juego->puntuacion);
    }

    for (int i = 0; i < juego->enemigosActivos; i++) {
        Enemigo* en = &juego->enemigos[i];
        en->rect.x += en->velX;
        en->rect.y += en->velY;

        if (en->rect.x < -70 || en->rect.x > ANCHO_VENTANA + 20 ||
            en->rect.y < -70 || en->rect.y > ALTO_VENTANA  + 20) {
            juego->puntuacion++;
            generarEnemigo(en);
        }

        if (verificarColision(&juego->jugador.rect, &en->rect)) {
            SDL_Log("Colision! Puntuacion: %d", juego->puntuacion);
            juego->ejecutando = false;
            return;
        }
    }

    verificarRecogidaMachete(juego);
    if (juego->machete.activo) juego->machete.activo = false;
}

// ============================================
// RENDERIZADO
// ============================================
void dibujarJuego(Juego* juego) {
    SDL_RenderClear(juego->renderer);

    int nivelIdx = SDL_clamp(nivelActual(juego->puntuacion) - 1, 0, 4);
    SDL_Texture* texFondoActual = juego->texFondos[nivelIdx];

    if (juego->transicion.activa) {
        int nivelDestino = SDL_clamp(juego->transicion.nivelNuevo - 1, 0, 4);
        SDL_Texture* texFondoDestino = juego->texFondos[nivelDestino];
        if (texFondoActual)
            SDL_RenderTexture(juego->renderer, texFondoActual, NULL, NULL);
        if (texFondoDestino) {
            float progreso = (float)(SDL_GetTicks() - juego->transicion.inicio)
                             / (float)DURACION_TRANSICION;
            float alpha = SDL_clamp(progreso * 1.6f - 0.3f, 0.0f, 1.0f);
            SDL_SetTextureAlphaMod(texFondoDestino, (Uint8)(alpha * 255.0f));
            SDL_RenderTexture(juego->renderer, texFondoDestino, NULL, NULL);
            SDL_SetTextureAlphaMod(texFondoDestino, 255);
        }
    } else {
        if (texFondoActual)
            SDL_RenderTexture(juego->renderer, texFondoActual, NULL, NULL);
    }

    SDL_RenderTexture(juego->renderer, juego->texJugador, NULL, &juego->jugador.rect);

    for (int i = 0; i < juego->enemigosActivos; i++)
        SDL_RenderTexture(juego->renderer, juego->texEnemigo, NULL, &juego->enemigos[i].rect);

    if (juego->puntuacion >= UMBRAL_NIVEL_4 || juego->macheteEquipado)
        if (!juego->machete.recogido || juego->macheteEquipado)
            SDL_RenderTexture(juego->renderer, juego->texMachete, NULL, &juego->machete.rect);

    if (juego->machete.activo || juego->machete.animandoAtaque) {
        float cx = juego->jugador.rect.x + TAMANO_SPRITE / 2.0f;
        float cy = juego->jugador.rect.y + TAMANO_SPRITE / 2.0f;

        SDL_SetRenderDrawColor(juego->renderer, 255, 50, 50, 255);
        SDL_FRect rv = {cx - RANGO_ATAQUE, cy - RANGO_ATAQUE,
                        (float)(RANGO_ATAQUE*2), (float)(RANGO_ATAQUE*2)};
        SDL_RenderRect(juego->renderer, &rv);

        if (juego->machete.animandoAtaque) {
            SDL_SetRenderDrawColor(juego->renderer, 255, 255, 0, 255);
            SDL_RenderLine(juego->renderer, cx, cy,
                juego->machete.rect.x + 24.0f, juego->machete.rect.y + 24.0f);
        }
    }

    mostrarPuntuacionPantalla(juego);
    renderizarBarraCooldown(juego);
}

void renderizar(Juego* juego) {
    dibujarJuego(juego);
    SDL_RenderPresent(juego->renderer);
}

// ============================================
// LIMPIEZA
// ============================================
void limpiarRecursos(Juego* juego) {
    limpiarAudio(juego);
    if (juego->gamepad) SDL_CloseGamepad(juego->gamepad);
    if (juego->texJugador) SDL_DestroyTexture(juego->texJugador);
    if (juego->texEnemigo) SDL_DestroyTexture(juego->texEnemigo);
    if (juego->texMachete) SDL_DestroyTexture(juego->texMachete);
    for (int i = 0; i < 5; i++)
        if (juego->texFondos[i]) SDL_DestroyTexture(juego->texFondos[i]);
    if (juego->fuentePequena && juego->fuentePequena != juego->fuente)
        TTF_CloseFont(juego->fuentePequena);
    if (juego->fuente)     TTF_CloseFont(juego->fuente);
    if (juego->renderer)   SDL_DestroyRenderer(juego->renderer);
    if (juego->ventana)    SDL_DestroyWindow(juego->ventana);
    TTF_Quit();
    SDL_Quit();
}

// ============================================
// REINICIO
// ============================================
void reiniciarJuego(Juego* juego) {
    inicializarJugador(&juego->jugador);
    inicializarEnemigos(juego);
    inicializarMachete(juego);
    juego->macheteAparecido      = false;
    juego->ultimoNivelDificultad = 0;
    juego->posicionNuevoPuntaje  = -1;
    juego->pistaSonando          = PISTA_NINGUNA;
    juego->nivel4Reproducido     = false;
    juego->gameOverReproducido   = false;
    juego->transicion            = {};
    juego->opcionGameOverSel     = 0;
    juego->opcionPausaSel        = 0;
    juego->estado                = ESTADO_JUGANDO;
}

// ============================================
// CUENTA REGRESIVA (3-2-1 antes de jugar)
// ============================================
void iniciarCuentaRegresiva(Juego* juego) {
    // Preparar el juego pero sin activar el loop de juego todavia
    inicializarJugador(&juego->jugador);
    inicializarEnemigos(juego);
    inicializarMachete(juego);
    juego->macheteAparecido      = false;
    juego->ultimoNivelDificultad = 0;
    juego->posicionNuevoPuntaje  = -1;
    juego->nivel4Reproducido     = false;
    juego->gameOverReproducido   = false;
    juego->transicion            = {};
    juego->opcionGameOverSel     = 0;
    juego->opcionPausaSel        = 0;
    juego->pistaSonando          = PISTA_NINGUNA;
    juego->inicioCuentaRegresiva = SDL_GetTicks();
    juego->estado                = ESTADO_CUENTA_REGRESIVA;
}

void renderizarCuentaRegresiva(Juego* juego) {
    Uint64 elapsed = SDL_GetTicks() - juego->inicioCuentaRegresiva;

    // Tras 3.2 segundos, pasar a jugar
    if (elapsed >= 3200) {
        juego->estado = ESTADO_JUGANDO;
        return;
    }

    // Fondo del nivel 1 como telón
    if (juego->texFondos[0])
        SDL_RenderTexture(juego->renderer, juego->texFondos[0], NULL, NULL);

    // Overlay semitransparente
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(juego->renderer, 0, 0, 0, 145);
    SDL_FRect overlay = {0, 0, (float)ANCHO_VENTANA, (float)ALTO_VENTANA};
    SDL_RenderFillRect(juego->renderer, &overlay);
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color gris     = {160, 160, 160, 255};
    SDL_Color verde    = { 80, 220, 100, 255};

    // Separador
    SDL_SetRenderDrawColor(juego->renderer, 70, 70, 70, 255);
    SDL_RenderLine(juego->renderer,
        ANCHO_VENTANA/2 - 220, ALTO_VENTANA/2 - 125,
        ANCHO_VENTANA/2 + 220, ALTO_VENTANA/2 - 125);

    // Recordatorio rapido de controles
    renderizarTextoCentradoPequeno(juego,
        "WASD / Flechas: mover   |   ESPACIO: machete   |   ESC: pausar",
        ALTO_VENTANA/2 - 175, gris);
    renderizarTextoCentradoPequeno(juego,
        "Esquiva los enemigos — el machete aparece al nivel 4",
        ALTO_VENTANA/2 - 140, blanco);

    SDL_RenderLine(juego->renderer,
        ANCHO_VENTANA/2 - 220, ALTO_VENTANA/2 - 115,
        ANCHO_VENTANA/2 + 220, ALTO_VENTANA/2 - 115);

    // Numero de cuenta regresiva con efecto de escala pulsante
    int cuenta = 3 - (int)(elapsed / 1000);
    if (cuenta < 1) cuenta = 1;

    char numStr[4];
    SDL_snprintf(numStr, sizeof(numStr), "%d", cuenta);

    // Escala: empieza grande al inicio de cada segundo, se reduce
    float fase = fmodf((float)(elapsed % 1000) / 1000.0f, 1.0f);
    float escala = 1.0f + (1.0f - fase) * 0.5f;

    int tw = 0, th = 0;
    TTF_GetStringSize(juego->fuente, numStr, 0, &tw, &th);
    int bw = (int)(tw * escala);
    int bh = (int)(th * escala);
    int bx = (ANCHO_VENTANA - bw) / 2;
    int by = ALTO_VENTANA/2 - bh/2 - 10;

    SDL_Surface* sup = TTF_RenderText_Solid(juego->fuente, numStr, 0, amarillo);
    if (sup) {
        SDL_Texture* tex = SDL_CreateTextureFromSurface(juego->renderer, sup);
        if (tex) {
            SDL_FRect dst = {(float)bx, (float)by, (float)bw, (float)bh};
            SDL_RenderTexture(juego->renderer, tex, NULL, &dst);
            SDL_DestroyTexture(tex);
        }
        SDL_DestroySurface(sup);
    }

    // Barra de progreso de la cuenta
    const int barW = 320, barH = 6;
    const int barX = (ANCHO_VENTANA - barW) / 2;
    const int barY = ALTO_VENTANA/2 + 90;

    float progreso = (float)elapsed / 3000.0f;
    if (progreso > 1.0f) progreso = 1.0f;

    SDL_SetRenderDrawColor(juego->renderer, 50, 50, 50, 255);
    SDL_FRect barFondo = {(float)barX, (float)barY, (float)barW, (float)barH};
    SDL_RenderFillRect(juego->renderer, &barFondo);

    SDL_SetRenderDrawColor(juego->renderer, 80, 220, 100, 255);
    SDL_FRect barRelleno = {(float)barX, (float)barY, barW * progreso, (float)barH};
    SDL_RenderFillRect(juego->renderer, &barRelleno);

    renderizarTextoCentradoPequeno(juego, "Preparandote...", ALTO_VENTANA/2 + 110, verde);

    SDL_RenderPresent(juego->renderer);

    // ESC cancela y vuelve al menu
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }
        if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)
            juego->estado = ESTADO_MENU;
    }
}

// ============================================
// MENU PRINCIPAL
// ============================================
void renderizarMenu(Juego* juego) {
    SDL_RenderClear(juego->renderer);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color gris     = {130, 130, 130, 255};

    // Titulo centrado en la mitad izquierda
    renderizarTextoCentrado(juego, juego->fuente, "ESQUIVAR BOTELLAS", 80, amarillo);

    // Botones de opciones
    const char* opciones[]  = {"JUGAR", "INSTRUCCIONES", "SALIR"};
    const int totalOpciones = 3;
    const int botonW = 300;
    const int botonH = 52;
    const int botonX = ANCHO_VENTANA / 4 - botonW / 2;
    const int btn0Y  = 210;
    const int btn1Y  = 280;
    const int btn2Y  = 350;
    const int btnY[] = {btn0Y, btn1Y, btn2Y};

    float mx = 0, my = 0;
    SDL_GetMouseState(&mx, &my);

    for (int i = 0; i < totalOpciones; i++) {
        if (mouseEnRect(mx, my, botonX, btnY[i], botonW, botonH))
            juego->opcionMenuSeleccionada = i;
    }

    SDL_Color colorBordes[] = {
        {255, 220, 0, 255},    // JUGAR — amarillo
        {255, 220, 0, 255},    // INSTRUCCIONES — amarillo
        {220,  60, 60, 255}    // SALIR — rojo
    };
    SDL_Color colorTextos[] = {
        blanco,
        blanco,
        {220, 100, 100, 255}
    };

    for (int i = 0; i < totalOpciones; i++) {
        dibujarBoton(juego, opciones[i],
            botonX, btnY[i], botonW, botonH,
            juego->opcionMenuSeleccionada == i,
            colorTextos[i], colorBordes[i]);
    }

    // Info de audio y controles
    SDL_Color colorAudio = juego->musicaActiva
        ? (SDL_Color){80, 255, 120, 255}
        : (SDL_Color){180, 180, 180, 255};
    char textoAudio[40];
    SDL_snprintf(textoAudio, sizeof(textoAudio),
        juego->musicaActiva ? "M: silenciar  |  Vol: %d%%" : "M: activar audio",
        juego->volumenMusica * 100 / 128);
    renderizarTextoPequeno(juego, textoAudio, botonX, ALTO_VENTANA - 80, colorAudio);
    renderizarTextoPequeno(juego,
        "Flechas: navegar  |  Enter: seleccionar  |  + / -: volumen",
        botonX, ALTO_VENTANA - 45, gris);

    // Divisor y Top 5
    SDL_SetRenderDrawColor(juego->renderer, 70, 70, 70, 255);
    SDL_RenderLine(juego->renderer,
        (float)(ANCHO_VENTANA / 2), 60.0f,
        (float)(ANCHO_VENTANA / 2), (float)(ALTO_VENTANA - 30));

    renderizarTop5(juego, ANCHO_VENTANA / 2 + 40, 80, -1);

    SDL_RenderPresent(juego->renderer);
}

void manejarEventosMenu(Juego* juego) {
    const int totalOpciones = 3;
    const int botonW = 300;
    const int botonH = 52;
    const int botonX = ANCHO_VENTANA / 4 - botonW / 2;
    const int btnY[] = {210, 280, 350};

    auto seleccionar = [&](int opcion) {
        switch (opcion) {
            case 0: iniciarCuentaRegresiva(juego); break;
            case 1: juego->estado = ESTADO_INSTRUCCIONES; break;
            case 2: juego->ejecutando = false; break;
        }
    };

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }

        if (e.type == SDL_EVENT_KEY_DOWN) {
            switch (e.key.key) {
                case SDLK_UP:
                    juego->opcionMenuSeleccionada =
                        (juego->opcionMenuSeleccionada - 1 + totalOpciones) % totalOpciones;
                    break;
                case SDLK_DOWN:
                    juego->opcionMenuSeleccionada =
                        (juego->opcionMenuSeleccionada + 1) % totalOpciones;
                    break;
                case SDLK_RETURN: case SDLK_KP_ENTER:
                    seleccionar(juego->opcionMenuSeleccionada);
                    break;
                case SDLK_M:     toggleMusicaMute(juego); break;
                case SDLK_EQUALS: case SDLK_PLUS: ajustarVolumen(juego, 16); break;
                case SDLK_MINUS: ajustarVolumen(juego, -16); break;
                default: break;
            }
        }

        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
            int cx = (int)e.button.x, cy = (int)e.button.y;
            for (int i = 0; i < totalOpciones; i++) {
                if (mouseEnRect(cx, cy, botonX, btnY[i], botonW, botonH))
                    seleccionar(i);
            }
        }

        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            switch (e.gbutton.button) {
                case SDL_GAMEPAD_BUTTON_DPAD_UP:
                    juego->opcionMenuSeleccionada =
                        (juego->opcionMenuSeleccionada - 1 + totalOpciones) % totalOpciones;
                    break;
                case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
                    juego->opcionMenuSeleccionada =
                        (juego->opcionMenuSeleccionada + 1) % totalOpciones;
                    break;
                case SDL_GAMEPAD_BUTTON_SOUTH:
                    seleccionar(juego->opcionMenuSeleccionada);
                    break;
                default: break;
            }
        }

        if (e.type == SDL_EVENT_GAMEPAD_ADDED && !juego->gamepad)
            juego->gamepad = SDL_OpenGamepad(e.gdevice.which);
        if (e.type == SDL_EVENT_GAMEPAD_REMOVED && juego->gamepad) {
            SDL_CloseGamepad(juego->gamepad);
            juego->gamepad = nullptr;
        }
    }
}

// ============================================
// INSTRUCCIONES (dos columnas, mouse en volver)
// ============================================
void renderizarInstrucciones(Juego* juego) {
    SDL_RenderClear(juego->renderer);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color gris     = {130, 130, 130, 255};
    SDL_Color verde    = { 80, 220,  80, 255};
    SDL_Color naranja  = {255, 165,   0, 255};
    SDL_Color rojo     = {220,  50,  50, 255};

    renderizarTextoCentrado(juego, juego->fuente, "INSTRUCCIONES", 36, amarillo);

    // COLUMNA IZQUIERDA — Controles
    const int colIzq = 60;

    renderizarTexto(juego, "TECLADO", colIzq, 110, amarillo);
    renderizarTextoPequeno(juego, "W  A  S  D",            colIzq, 158, blanco);
    renderizarTextoPequeno(juego, "  Mover al jugador",    colIzq, 180, gris);
    renderizarTextoPequeno(juego, "ESPACIO",               colIzq, 210, blanco);
    renderizarTextoPequeno(juego, "  Atacar con machete",  colIzq, 232, gris);
    renderizarTextoPequeno(juego, "ESC",                   colIzq, 262, blanco);
    renderizarTextoPequeno(juego, "  Pausar el juego",     colIzq, 284, gris);
    renderizarTextoPequeno(juego, "M   /   + / -",         colIzq, 314, blanco);
    renderizarTextoPequeno(juego, "  Musica y volumen",    colIzq, 336, gris);

    renderizarTexto(juego, "CONTROL", colIzq, 385, amarillo);
    renderizarTextoPequeno(juego, "Stick izq / D-pad",    colIzq, 432, blanco);
    renderizarTextoPequeno(juego, "  Mover al jugador",   colIzq, 454, gris);
    renderizarTextoPequeno(juego, "Boton Sur (Cruz/A)",   colIzq, 484, blanco);
    renderizarTextoPequeno(juego, "  Atacar con machete", colIzq, 506, gris);
    renderizarTextoPequeno(juego, "START",                colIzq, 536, blanco);
    renderizarTextoPequeno(juego, "  Pausar el juego",    colIzq, 558, gris);

    // Divisor central
    SDL_SetRenderDrawColor(juego->renderer, 70, 70, 70, 255);
    SDL_RenderLine(juego->renderer,
        (float)(ANCHO_VENTANA / 2), 95.0f,
        (float)(ANCHO_VENTANA / 2), (float)(ALTO_VENTANA - 80));

    // COLUMNA DERECHA — Niveles con cajas de color
    const int colDer = ANCHO_VENTANA / 2 + 40;
    renderizarTexto(juego, "NIVELES", colDer, 110, amarillo);

    struct InfoNivel { const char* rango; const char* desc; SDL_Color color; };
    InfoNivel niveles[] = {
        {"Nivel 1-2  (0-39 pts)",  "Musica tranquila",          {100, 220, 100, 255}},
        {"Nivel 3    (40-79 pts)", "Mas enemigos",               {150, 220,  80, 255}},
        {"Nivel 4    (80-159)",    "Machete + intro del jefe",   naranja             },
        {"Nivel 5    (160+)",      "Maxima dificultad",          rojo                },
    };

    int ny = 175;
    for (auto& nv : niveles) {
        SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(juego->renderer,
            nv.color.r/5, nv.color.g/5, nv.color.b/5, 200);
        SDL_FRect nr = {(float)colDer - 10, (float)ny - 6, 340.0f, 62.0f};
        SDL_RenderFillRect(juego->renderer, &nr);
        SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(juego->renderer, nv.color.r, nv.color.g, nv.color.b, 100);
        SDL_RenderRect(juego->renderer, &nr);

        renderizarTextoPequeno(juego, nv.rango, colDer, ny,      nv.color);
        renderizarTextoPequeno(juego, nv.desc,  colDer, ny + 26, blanco);
        ny += 82;
    }

    // Nota del machete
    ny += 8;
    renderizarTextoPequeno(juego, "El machete gira 360 y elimina enemigos",
        colDer, ny, gris);
    renderizarTextoPequeno(juego, "en un radio de 150px — cooldown: 2 seg",
        colDer, ny + 28, gris);

    // Boton VOLVER con soporte de mouse
    const int btnX = (ANCHO_VENTANA - 280) / 2;
    const int btnY = ALTO_VENTANA - 68;
    const int btnW = 280;
    const int btnH = 46;

    float mx = 0, my = 0;
    SDL_GetMouseState(&mx, &my);
    bool hoverVolver = mouseEnRect(mx, my, btnX, btnY, btnW, btnH);

    dibujarBoton(juego, "VOLVER AL MENU  (ESC)",
        btnX, btnY, btnW, btnH, hoverVolver,
        (SDL_Color){200, 200, 200, 255}, amarillo);

    SDL_RenderPresent(juego->renderer);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }
        if (e.type == SDL_EVENT_KEY_DOWN &&
           (e.key.key == SDLK_ESCAPE || e.key.key == SDLK_BACKSPACE ||
            e.key.key == SDLK_RETURN  || e.key.key == SDLK_KP_ENTER))
            juego->estado = ESTADO_MENU;
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT)
            if (mouseEnRect((int)e.button.x, (int)e.button.y, btnX, btnY, btnW, btnH))
                juego->estado = ESTADO_MENU;
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN &&
           (e.gbutton.button == SDL_GAMEPAD_BUTTON_EAST ||
            e.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH))
            juego->estado = ESTADO_MENU;
    }
}

// ============================================
// GAME OVER (3 botones, mouse completo)
// ============================================
void renderizarGameOver(Juego* juego) {
    SDL_RenderClear(juego->renderer);

    SDL_Color rojo     = {220,  50,  50, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color verde    = { 80, 255, 120, 255};
    SDL_Color gris     = {130, 130, 130, 255};

    // Titulo centrado
    renderizarTextoCentrado(juego, juego->fuente, "GAME OVER", 72, rojo);

    // Stats centrados
    char scoreTexto[64];
    SDL_snprintf(scoreTexto, sizeof(scoreTexto), "Puntuacion: %d", juego->puntuacion);
    renderizarTextoCentrado(juego, juego->fuentePequena, scoreTexto, 162, blanco);

    char nivelTexto[64];
    SDL_snprintf(nivelTexto, sizeof(nivelTexto),
        "Nivel alcanzado: %d", nivelActual(juego->puntuacion));
    renderizarTextoCentrado(juego, juego->fuentePequena, nivelTexto, 198, amarillo);

    if (juego->posicionNuevoPuntaje >= 0) {
        char msgPos[64];
        SDL_snprintf(msgPos, sizeof(msgPos),
            "TOP 5!  Puesto #%d", juego->posicionNuevoPuntaje + 1);
        renderizarTextoCentrado(juego, juego->fuentePequena, msgPos, 234, verde);
    }

    // Botones en la mitad izquierda
    const int btnW  = 300;
    const int btnH  = 52;
    const int btnX  = ANCHO_VENTANA / 4 - btnW / 2;
    const int btn0Y = 295;
    const int btn1Y = 365;
    const int btn2Y = 435;

    float mx = 0, my = 0;
    SDL_GetMouseState(&mx, &my);

    if (mouseEnRect(mx, my, btnX, btn0Y, btnW, btnH)) juego->opcionGameOverSel = 0;
    if (mouseEnRect(mx, my, btnX, btn1Y, btnW, btnH)) juego->opcionGameOverSel = 1;
    if (mouseEnRect(mx, my, btnX, btn2Y, btnW, btnH)) juego->opcionGameOverSel = 2;

    dibujarBoton(juego, "JUGAR DE NUEVO",
        btnX, btn0Y, btnW, btnH,
        juego->opcionGameOverSel == 0,
        (SDL_Color){200, 220, 200, 255}, verde);

    dibujarBoton(juego, "MENU PRINCIPAL",
        btnX, btn1Y, btnW, btnH,
        juego->opcionGameOverSel == 1,
        blanco, amarillo);

    dibujarBoton(juego, "SALIR DEL JUEGO",
        btnX, btn2Y, btnW, btnH,
        juego->opcionGameOverSel == 2,
        (SDL_Color){180, 120, 120, 255}, rojo);

    renderizarTextoPequeno(juego,
        "Flechas: navegar  |  Enter: confirmar",
        btnX, ALTO_VENTANA - 44, gris);

    // Divisor y Top 5
    SDL_SetRenderDrawColor(juego->renderer, 70, 70, 70, 255);
    SDL_RenderLine(juego->renderer,
        (float)(ANCHO_VENTANA / 2), 55.0f,
        (float)(ANCHO_VENTANA / 2), (float)(ALTO_VENTANA - 30));

    renderizarTop5(juego, ANCHO_VENTANA / 2 + 40, 72, juego->posicionNuevoPuntaje);

    SDL_RenderPresent(juego->renderer);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }

        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_UP)
                juego->opcionGameOverSel = (juego->opcionGameOverSel - 1 + 3) % 3;
            if (e.key.key == SDLK_DOWN)
                juego->opcionGameOverSel = (juego->opcionGameOverSel + 1) % 3;

            if (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER) {
                switch (juego->opcionGameOverSel) {
                    case 0: iniciarCuentaRegresiva(juego); break;
                    case 1: reiniciarJuego(juego); juego->estado = ESTADO_MENU; break;
                    case 2: juego->ejecutando = false; break;
                }
            }
            if (e.key.key == SDLK_ESCAPE) {
                reiniciarJuego(juego);
                juego->estado = ESTADO_MENU;
            }
        }

        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
            int cx = (int)e.button.x, cy = (int)e.button.y;
            if      (mouseEnRect(cx, cy, btnX, btn0Y, btnW, btnH))
                iniciarCuentaRegresiva(juego);
            else if (mouseEnRect(cx, cy, btnX, btn1Y, btnW, btnH)) {
                reiniciarJuego(juego); juego->estado = ESTADO_MENU;
            } else if (mouseEnRect(cx, cy, btnX, btn2Y, btnW, btnH))
                juego->ejecutando = false;
        }

        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP)
                juego->opcionGameOverSel = (juego->opcionGameOverSel - 1 + 3) % 3;
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN)
                juego->opcionGameOverSel = (juego->opcionGameOverSel + 1) % 3;
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
                switch (juego->opcionGameOverSel) {
                    case 0: iniciarCuentaRegresiva(juego); break;
                    case 1: reiniciarJuego(juego); juego->estado = ESTADO_MENU; break;
                    case 2: juego->ejecutando = false; break;
                }
            }
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_EAST) {
                reiniciarJuego(juego);
                juego->estado = ESTADO_MENU;
            }
        }
    }
}

// ============================================
// PAUSA (botones centrados, mouse completo)
// ============================================
void renderizarPausa(Juego* juego) {
    dibujarJuego(juego);

    // Overlay oscuro
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(juego->renderer, 0, 0, 0, 160);
    SDL_FRect overlay = {0, 0, (float)ANCHO_VENTANA, (float)ALTO_VENTANA};
    SDL_RenderFillRect(juego->renderer, &overlay);
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color rojo     = {220,  50,  50, 255};
    SDL_Color gris     = {130, 130, 130, 255};

    renderizarTextoCentrado(juego, juego->fuente, "PAUSADO", 140, amarillo);

    // Botones centrados
    const int btnW  = 320;
    const int btnH  = 52;
    const int btnX  = (ANCHO_VENTANA - btnW) / 2;
    const int btn0Y = 240;
    const int btn1Y = 312;
    const int btn2Y = 384;

    float mx = 0, my = 0;
    SDL_GetMouseState(&mx, &my);

    if (mouseEnRect(mx, my, btnX, btn0Y, btnW, btnH)) juego->opcionPausaSel = 0;
    if (mouseEnRect(mx, my, btnX, btn1Y, btnW, btnH)) juego->opcionPausaSel = 1;
    if (mouseEnRect(mx, my, btnX, btn2Y, btnW, btnH)) juego->opcionPausaSel = 2;

    dibujarBoton(juego, "CONTINUAR  (ESC)",
        btnX, btn0Y, btnW, btnH,
        juego->opcionPausaSel == 0,
        blanco, amarillo);

    dibujarBoton(juego, "VOLVER AL MENU",
        btnX, btn1Y, btnW, btnH,
        juego->opcionPausaSel == 1,
        (SDL_Color){200, 200, 200, 255}, amarillo);

    dibujarBoton(juego, "SALIR DEL JUEGO  (Q)",
        btnX, btn2Y, btnW, btnH,
        juego->opcionPausaSel == 2,
        (SDL_Color){180, 120, 120, 255}, rojo);

    // Info de audio
    SDL_Color colorAudio = juego->musicaActiva
        ? (SDL_Color){80, 255, 120, 255}
        : (SDL_Color){180, 180, 180, 255};
    char textoAudio[48];
    SDL_snprintf(textoAudio, sizeof(textoAudio),
        juego->musicaActiva ? "Audio ON  |  Vol: %d%%" : "Audio: SILENCIADO",
        juego->volumenMusica * 100 / 128);
    renderizarTextoCentradoPequeno(juego, textoAudio, 464, colorAudio);
    renderizarTextoCentradoPequeno(juego,
        "M: silenciar  |  + / -: volumen  |  Flechas: navegar", 496, gris);

    SDL_RenderPresent(juego->renderer);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }

        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_UP)
                juego->opcionPausaSel = (juego->opcionPausaSel - 1 + 3) % 3;
            if (e.key.key == SDLK_DOWN)
                juego->opcionPausaSel = (juego->opcionPausaSel + 1) % 3;

            if (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER) {
                switch (juego->opcionPausaSel) {
                    case 0: juego->estado = ESTADO_JUGANDO; break;
                    case 1: reiniciarJuego(juego); juego->estado = ESTADO_MENU; break;
                    case 2: juego->ejecutando = false; break;
                }
            }
            if (e.key.key == SDLK_ESCAPE)  juego->estado = ESTADO_JUGANDO;
            if (e.key.key == SDLK_Q)        juego->ejecutando = false;
            if (e.key.key == SDLK_M)        toggleMusicaMute(juego);
            if (e.key.key == SDLK_EQUALS || e.key.key == SDLK_PLUS)
                ajustarVolumen(juego, 16);
            if (e.key.key == SDLK_MINUS)    ajustarVolumen(juego, -16);
        }

        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
            int cx = (int)e.button.x, cy = (int)e.button.y;
            if      (mouseEnRect(cx, cy, btnX, btn0Y, btnW, btnH))
                juego->estado = ESTADO_JUGANDO;
            else if (mouseEnRect(cx, cy, btnX, btn1Y, btnW, btnH)) {
                reiniciarJuego(juego); juego->estado = ESTADO_MENU;
            } else if (mouseEnRect(cx, cy, btnX, btn2Y, btnW, btnH))
                juego->ejecutando = false;
        }

        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP)
                juego->opcionPausaSel = (juego->opcionPausaSel - 1 + 3) % 3;
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN)
                juego->opcionPausaSel = (juego->opcionPausaSel + 1) % 3;
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_START)
                juego->estado = ESTADO_JUGANDO;
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
                switch (juego->opcionPausaSel) {
                    case 0: juego->estado = ESTADO_JUGANDO; break;
                    case 1: reiniciarJuego(juego); juego->estado = ESTADO_MENU; break;
                    case 2: juego->ejecutando = false; break;
                }
            }
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_EAST) {
                reiniciarJuego(juego);
                juego->estado = ESTADO_MENU;
            }
        }
    }
}